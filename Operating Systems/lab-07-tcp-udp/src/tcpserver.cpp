#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>

#define MAX_CLIENTS 1024     // сколько максимум клиентов можем держать
#define RECV_CHUNK_SIZE 4096 // сколько байт читаем за один recv
#define FIXED_PART_SIZE 31   // размер обязательной части сообщения

enum ClientMode {
    MODE_WAIT_COMMAND = 0,
    MODE_PUT = 1,
    MODE_GET = 2
};

struct Client {
    int fd;                    // сокет клиента
    sockaddr_in addr;          // адрес клиента
    int mode;                  // put/get или уже работаем
    int close_after_send;      // надо ли закрыть после отправки (для get)
    int stop_after_send;       // надо ли завершить сервер после отправки
    int peer_closed;           // клиент закрыл соединение(для put)
    std::vector<char> in_buf;  // Всё, что пришло от клиента
    std::vector<char> out_buf; // Всё, что сервер хочет отправить клиенту( для get)
    size_t out_sent;           // сколько из out_buf удалось отправить
};

static int CheckPort(const char *text, int *port_out) {  // Проверяем порт и если он верный то возвращаем его
    char *end = NULL;
    long port = 0;

    if (text == NULL || *text == '\0') return 0;

    port = strtol(text, &end, 10);
    if (*end != '\0') return 0;
    if (port <= 0 || port > 65535) return 0;

    *port_out = (int)port;
    return 1;
}

static int SetNonBlocking(int fd) {
    // fcntl — это системная функция Linux для работы с настройками дескриптора.
    int flags = fcntl(fd, F_GETFL, 0); // F_GETFL - получить текущее состояние флагов этого дескриптора 
    if (flags < 0) return 0;
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) return 0; // Устанавливаем неблокирующий режим - если операция сейчас невозможна, программа не ждёт, а сразу возвращается
    return 1;
}

static void InitClient(Client *c) {         // Инициализация клиента
    c->fd = -1;
    memset(&c->addr, 0, sizeof(c->addr));
    c->mode = MODE_WAIT_COMMAND;
    c->close_after_send = 0;
    c->stop_after_send = 0;
    c->peer_closed = 0;
    c->in_buf.clear();
    c->out_buf.clear();
    c->out_sent = 0;
}

static void CloseClient(Client *c) {
    if (c->fd >= 0) {
        close(c->fd);
    }
    InitClient(c);
}

static int FlushOutput(Client *c) { // Сервер отправляет клиенту то , что лежит в out_buf например "ok"
    while (c->out_sent < c->out_buf.size()) {
        ssize_t sent = send(c->fd,
                            &c->out_buf[c->out_sent],
                            c->out_buf.size() - c->out_sent,
                            0);
        if (sent > 0) {
            c->out_sent += (size_t)sent;
            continue;
        }
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 1;
        }
        return 0;
    }

    if (c->out_sent >= c->out_buf.size()) {
        c->out_buf.clear();
        c->out_sent = 0;
    }
    return 1;
}

static int QueueOk(Client *c) {
    c->out_buf.push_back('o');
    c->out_buf.push_back('k');
    return 1;
}

static int ValidText(const char *text, size_t len) {
    size_t i;
    if (len == 0) return 0;
    for (i = 0; i < len; ++i) {
        if (text[i] == '\n' || text[i] == '\r') return 0;
    }
    return 1;
}

static int WriteMessageToFile(const Client *c,
                              const char *phone1,
                              const char *phone2,
                              unsigned char hh,
                              unsigned char mm,
                              unsigned char ss,
                              const char *text,
                              size_t text_len) {
    FILE *f = fopen("msg.txt", "ab");
    if (f == NULL) {
        printf("Error: cannot open msg.txt\n");
        return 0;
    }

    fprintf(f,
            "%s:%u %.*s %.*s %02u:%02u:%02u ",
            inet_ntoa(c->addr.sin_addr),
            (unsigned int)ntohs(c->addr.sin_port),
            12,
            phone1,
            12,
            phone2,
            (unsigned int)hh,
            (unsigned int)mm,
            (unsigned int)ss);

    if (text_len > 0) {
        fwrite(text, 1, text_len, f);
    }
    fputc('\n', f);
    fclose(f);
    return 1;
}

static int QueueGetMessage(Client *c,
                           uint32_t idx,
                           const char *phone1,
                           const char *phone2,
                           unsigned char hh,
                           unsigned char mm,
                           unsigned char ss,
                           const char *text) {
    uint32_t net_idx = htonl(idx);
    char p1[12];
    char p2[12];
    size_t len1 = strlen(phone1);
    size_t len2 = strlen(phone2);

    memset(p1, ' ', sizeof(p1));
    memset(p2, ' ', sizeof(p2));

    if (len1 > 12) len1 = 12;
    if (len2 > 12) len2 = 12;
    memcpy(p1, phone1, len1);
    memcpy(p2, phone2, len2);

    c->out_buf.insert(c->out_buf.end(), (char *)&net_idx, (char *)&net_idx + 4);
    c->out_buf.insert(c->out_buf.end(), p1, p1 + 12);
    c->out_buf.insert(c->out_buf.end(), p2, p2 + 12);
    c->out_buf.push_back((char)hh);
    c->out_buf.push_back((char)mm);
    c->out_buf.push_back((char)ss);
    c->out_buf.insert(c->out_buf.end(), text, text + strlen(text));
    c->out_buf.push_back('\0');
    return 1;
}

static int LoadHistoryForGet(Client *c) {
    FILE *f = fopen("msg.txt", "rb");
    char line[65536];
    uint32_t idx = 0;

    if (f == NULL) return 1;

    while (fgets(line, sizeof(line), f) != NULL) {
        char *nl = strchr(line, '\n');
        char peer_ignored[128];
        char phone1[64];
        char phone2[64];
        char tstr[64];
        char text[65000];
        int hh, mm, ss;

        if (nl != NULL) *nl = '\0';
        text[0] = '\0';

        if (sscanf(line, "%127s %63s %63s %63s %64999[^\n]",
                   peer_ignored, phone1, phone2, tstr, text) < 4) {
            continue;
        }

        if (sscanf(tstr, "%d:%d:%d", &hh, &mm, &ss) != 3) continue;
        if (hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 59) continue;

        QueueGetMessage(c,
                        idx,
                        phone1,
                        phone2,
                        (unsigned char)hh,
                        (unsigned char)mm,
                        (unsigned char)ss,
                        text);
        ++idx;
    }

    fclose(f);
    return 1;
}

int main(int argc, char *argv[]) {  // tcpserver <port>
    int port = 0;
    int listen_fd = -1;             // дескриптор серверного сокета
    Client clients[MAX_CLIENTS];    // сколько клиентов может подключиться к серверу
    int shutdown_requested = 0;     // Нужно ли останавливать сервер? 0 - значит нет пока
    int i;

    if (argc < 2) {
        printf("Error: port is not specified\n");
        return 1;
    }
    if (!CheckPort(argv[1], &port)) {
        printf("Error: invalid port\n");
        return 1;
    }

    listen_fd = socket(AF_INET, SOCK_STREAM, 0); // Создаём tcp-сокет
    if (listen_fd < 0) {
        printf("Error: cannot create socket\n");
        return 1;
    }

    
    int reuse = 1;
    sockaddr_in saddr;

    // SO_REUSEADDR опция разрешает сокету повторно использовать адрес/порт быстрее
    // SOL_SOCKET - общие опции сокета, для настройки SO_REUSEADDR
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); 

    if (!SetNonBlocking(listen_fd)) {
        printf("Error: cannot set nonblocking mode\n");
        close(listen_fd);
        return 1;
    }

    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;                 // используем семейство адресов IPv4
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);  // любой адрес слушаем (в сетевом порядке байт)
    saddr.sin_port = htons((uint16_t)port);     // порт, к-ый сервер будет слушать
     
    if (bind(listen_fd, (sockaddr *)&saddr, sizeof(saddr)) < 0) {           // привязываем сокет к адресу и порту
        printf("Error: cannot bind port %d: %s\n", port, strerror(errno));
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {     // делаем серверный сокет слушащим, SOMAXCONN - макс длина очереди ожидающих подключений
        printf("Error: cannot listen\n");
        close(listen_fd);
        return 1;
    }
    

    for (i = 0; i < MAX_CLIENTS; ++i) {    // инициализируем весь массив клиентов
        InitClient(&clients[i]);
    }

    while (!shutdown_requested) {
        fd_set read_set;  // сокеты из к-ых можно читать
        fd_set write_set; // сокеты в к-ые можно писать(для get)
        int max_fd = listen_fd; // максимальный номер дескриптора
        int ready = 0;

        FD_ZERO(&read_set); // очищаем множество read_set и write_set
        FD_ZERO(&write_set);
        FD_SET(listen_fd, &read_set); // listen_fd в read_set 

        for (i = 0; i < MAX_CLIENTS; ++i) {
            /* в каждом элементе массива сидит либо реально подключённый клиент,
               либо слот пустой */
            if (clients[i].fd < 0) continue;
            FD_SET(clients[i].fd, &read_set);
            if (clients[i].out_sent < clients[i].out_buf.size()) {
                FD_SET(clients[i].fd, &write_set);
            }

            if (clients[i].fd > max_fd) { // это нужно для select
                max_fd = clients[i].fd;
            }
        }

        ready = select(max_fd + 1, &read_set, &write_set, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) continue;
            printf("Error: select failed\n");
            break;
        }

        if (FD_ISSET(listen_fd, &read_set)) { // сервер принимает новые подключения клиентов
            while (1) { // новых подключений может быть несколько
                sockaddr_in caddr; // подготовка адреса клиента
                socklen_t caddr_len = sizeof(caddr);
                int cfd = accept(listen_fd, (sockaddr *)&caddr, &caddr_len); // сервер принимает 1 подключение

                if (cfd < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break; // новых подключений больше нет
                    printf("Error: accept failed\n");
                    break;
                }

                if (!SetNonBlocking(cfd)) {
                    close(cfd);
                    continue;
                }

                for (i = 0; i < MAX_CLIENTS; ++i) {
                    if (clients[i].fd < 0) {
                        clients[i].fd = cfd;
                        clients[i].addr = caddr;
                        clients[i].mode = MODE_WAIT_COMMAND;
                        clients[i].close_after_send = 0;
                        clients[i].stop_after_send = 0;
                        clients[i].peer_closed = 0;
                        break;
                    }
                }

                if (i == MAX_CLIENTS) { // свободного места для клиента не найдено
                    close(cfd);
                }
            }
        }

        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd < 0) continue;

            if (FD_ISSET(clients[i].fd, &read_set)) {
                while (1) { // пока из клиентского сокета можно читать данные 
                    char temp[RECV_CHUNK_SIZE]; // порция данных пришедших от клиента
                    ssize_t r = recv(clients[i].fd, temp, sizeof(temp), 0); // читаем данные от клиента

                    if (r > 0) {
                        clients[i].in_buf.insert(clients[i].in_buf.end(), temp, temp + r);
                        continue;
                    }
                    if (r == 0) { // клиент закрыл соединение
                        clients[i].peer_closed = 1;
                        break;
                    }
                    if (errno == EAGAIN || errno == EWOULDBLOCK) { // на данный момент данных больше нет
                        break;
                    }
                    CloseClient(&clients[i]); // если другая ошибка, закрываем клиента.
                    break;
                }

                if (clients[i].fd < 0) continue;

                if (clients[i].mode == MODE_WAIT_COMMAND && clients[i].in_buf.size() >= 3) { // Разбираем первую команду клиента
                    if (memcmp(&clients[i].in_buf[0], "put", 3) == 0) {
                        clients[i].mode = MODE_PUT;
                    } else if (memcmp(&clients[i].in_buf[0], "get", 3) == 0) {
                        clients[i].mode = MODE_GET;
                    } else {
                        printf("Invalid command\n");
                        CloseClient(&clients[i]);
                        continue;
                    }
                    clients[i].in_buf.erase(clients[i].in_buf.begin(), clients[i].in_buf.begin() + 3);

                    if (clients[i].mode == MODE_GET) { // Если режим GET
                        if (!LoadHistoryForGet(&clients[i])) {
                            CloseClient(&clients[i]);
                            continue;
                        }
                        clients[i].close_after_send = 1;
                    }
                }

                if (clients[i].fd < 0) continue;

                if (clients[i].mode == MODE_PUT) {
                    while (1) { // пока во входном буфере есть полные сообщения, разбирать их одно за другим
                        size_t zero_pos = FIXED_PART_SIZE; // конец текста сообщения, позиция
                        char phone1[13];
                        char phone2[13];
                        unsigned char hh;
                        unsigned char mm;
                        unsigned char ss;
                        const char *text_ptr;
                        size_t text_len;

                        if (clients[i].in_buf.size() < FIXED_PART_SIZE) break; // некорректное сообщение

                        while (zero_pos < clients[i].in_buf.size() && // ищем конец текста
                               clients[i].in_buf[zero_pos] != '\0') {
                            ++zero_pos;
                        }

                        if (zero_pos == clients[i].in_buf.size()) break; // не нашли терминальный нуль

                        memcpy(phone1, &clients[i].in_buf[4], 12);
                        memcpy(phone2, &clients[i].in_buf[16], 12);
                        phone1[12] = '\0';
                        phone2[12] = '\0';
                        hh = (unsigned char)clients[i].in_buf[28];
                        mm = (unsigned char)clients[i].in_buf[29];
                        ss = (unsigned char)clients[i].in_buf[30];
                        text_ptr = &clients[i].in_buf[FIXED_PART_SIZE];
                        text_len = zero_pos - FIXED_PART_SIZE;

                        if (hh <= 23 && mm <= 59 && ss <= 59 && ValidText(text_ptr, text_len)) {
                            if (!WriteMessageToFile(&clients[i], phone1, phone2, hh, mm, ss, text_ptr, text_len)) {
                                CloseClient(&clients[i]);
                                break;
                            }
                            QueueOk(&clients[i]);
                            if (text_len == 4 && memcmp(text_ptr, "stop", 4) == 0) {
                                clients[i].stop_after_send = 1;
                            }
                        }

                        clients[i].in_buf.erase(clients[i].in_buf.begin(),
                                                clients[i].in_buf.begin() + zero_pos + 1);
                    }
                }
            }

            if (clients[i].fd >= 0 && FD_ISSET(clients[i].fd, &write_set)) {
                if (!FlushOutput(&clients[i])) {
                    CloseClient(&clients[i]);
                    continue;
                }
            }

            if (clients[i].fd >= 0 &&
                clients[i].stop_after_send &&
                clients[i].out_buf.empty()) {
                shutdown_requested = 1;
            }

            if (clients[i].fd >= 0 &&
                clients[i].close_after_send &&
                clients[i].out_buf.empty()) {
                CloseClient(&clients[i]);
                continue;
            }

            if (clients[i].fd >= 0 &&
                clients[i].peer_closed &&
                clients[i].out_buf.empty()) {
                CloseClient(&clients[i]);
            }
        }
    }

    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd >= 0) {
            CloseClient(&clients[i]);
        }
    }

    close(listen_fd);
    return 0;
}
