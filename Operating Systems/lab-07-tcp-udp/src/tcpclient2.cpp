#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

struct Message {
    char phone1[13];         // Первый номер телефона. 13, а не 12, чтобы влез '\0'.
    char phone2[13];         // Второй номер телефона.
    unsigned char hour;      // Часы.
    unsigned char minute;    // Минуты.
    unsigned char second;    // Секунды.
    std::string text;        // Текст сообщения любой длины.
};

static int init_winsock() {
    WSADATA data;
    int err = WSAStartup(MAKEWORD(2, 2), &data);
    if (err != 0) {
        printf("WSAStartup failed: %d\n", err);
        return 0;
    }
    return 1;
}

static void cleanup_socket(SOCKET s) {
    // Общее завершение: закрываем сокет и выгружаем WinSock.
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    WSACleanup();
}

static int parse_server_address(const char *arg, sockaddr_in *addr, std::string *server_prefix) {
    // Разбираем строку вида "IP:port" и готовим sockaddr_in для connect.
    // Дополнительно собираем строку "IP:port ", которая потом пойдёт в начало
    // каждой строки выходного файла.
    std::string addr_port;
    size_t colon_pos;
    std::string ip_part;
    std::string port_part;
    long port_value;
    char *end = NULL;
    unsigned long ip_value;

    if (arg == NULL || addr == NULL) {
        return 0;
    }

    addr_port = arg;
    colon_pos = addr_port.find(':');
    if (colon_pos == std::string::npos) {
        return 0;
    }

    ip_part = addr_port.substr(0, colon_pos);
    port_part = addr_port.substr(colon_pos + 1);
    if (ip_part.empty() || port_part.empty()) {
        return 0;
    }

    port_value = strtol(port_part.c_str(), &end, 10);
    if (*end != '\0' || port_value <= 0 || port_value > 65535) {
        return 0;
    }

    ip_value = inet_addr(ip_part.c_str());
    if (ip_value == INADDR_NONE) {
        return 0;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons((unsigned short)port_value);
    addr->sin_addr.s_addr = ip_value;

    if (server_prefix != NULL) {
        *server_prefix = ip_part + ":" + port_part + " ";
    }

    return 1;
}

static int connect_with_retry(SOCKET s, const sockaddr_in *addr) {
    int attempt;

    // По условию делаем до 10 попыток подключения с паузой 100 мс.
    for (attempt = 0; attempt < 10; ++attempt) {
        if (connect(s, (const sockaddr *)addr, sizeof(*addr)) != SOCKET_ERROR) {
            return 1;
        }
        Sleep(100);
    }

    printf("Failed to connect\n");
    return 0;
}

static int set_socket_timeouts(SOCKET s) {
    // Таймауты защищают от бесконечного зависания на send/recv.
    int timeout_ms = 5000;

    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms)) == SOCKET_ERROR) {
        return 0;
    }
    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout_ms, sizeof(timeout_ms)) == SOCKET_ERROR) {
        return 0;
    }

    return 1;
}

static int send_all(SOCKET s, const char *data, int size) {
    int total = 0;

    // send может отправить только часть буфера, поэтому досылаем в цикле.
    while (total < size) {
        int sent = send(s, data + total, size - total, 0);
        if (sent == SOCKET_ERROR || sent == 0) {
            printf("send failed: %d\n", WSAGetLastError());
            return 0;
        }
        total += sent;
    }

    return 1;
}

static int recv_all(SOCKET s, char *buffer, int size) {
    int total = 0;

    // recv тоже может вернуть не все байты сразу, поэтому дочитываем до конца.
    while (total < size) {
        int received = recv(s, buffer + total, size - total, 0);
        if (received == SOCKET_ERROR || received == 0) {
            return 0;
        }
        total += received;
    }

    return 1;
}

static int recv_message(SOCKET s, Message *msg) {
    // Сервер при get отправляет сообщение в бинарном формате:
    // 4 байта index, 12 байт phone1, 12 байт phone2, 3 байта времени,
    // затем текст до '\0'.
    unsigned int net_index;
    char time_bytes[3];
    char ch;

    if (!recv_all(s, (char *)&net_index, 4)) {
        return 0;
    }

    if (!recv_all(s, msg->phone1, 12)) {
        return 0;
    }
    msg->phone1[12] = '\0';

    if (!recv_all(s, msg->phone2, 12)) {
        return 0;
    }
    msg->phone2[12] = '\0';

    if (!recv_all(s, time_bytes, 3)) {
        return 0;
    }

    msg->hour = (unsigned char)time_bytes[0];
    msg->minute = (unsigned char)time_bytes[1];
    msg->second = (unsigned char)time_bytes[2];
    msg->text.clear();

    // Текст заранее неизвестной длины, поэтому читаем его посимвольно
    // до завершающего нулевого байта.
    while (1) {
        int received = recv(s, &ch, 1, 0);
        if (received == SOCKET_ERROR || received == 0) {
            return 0;
        }
        if (ch == '\0') {
            break;
        }
        msg->text.push_back(ch);
    }

    return 1;
}

static void write_message(FILE *file, const std::string &server_prefix, const Message *msg) {
    // В выходной файл номер сообщения не записывается.
    // Сохраняем только адрес сервера, телефоны, время и текст.
    fprintf(file,
            "%s%s %s %02u:%02u:%02u %s\n",
            server_prefix.c_str(),
            msg->phone1,
            msg->phone2,
            (unsigned int)msg->hour,
            (unsigned int)msg->minute,
            (unsigned int)msg->second,
            msg->text.c_str());
}

int main(int argc, char *argv[]) {
    sockaddr_in server_addr;     // Сетевой адрес сервера.
    std::string server_prefix;   // Строка вида "IP:port " для записи в файл.
    SOCKET s = INVALID_SOCKET;   // TCP-сокет клиента.
    FILE *file = NULL;           // Выходной файл, куда сохраняем полученные сообщения.
    Message msg;                 // Очередное сообщение, принятое от сервера.

    if (argc < 4) {
        printf("Invalid args\n");
        return 1;
    }

    // Второй аргумент должен быть ровно "get".
    if (strcmp(argv[2], "get") != 0) {
        printf("Invalid command\n");
        return 1;
    }

    if (!parse_server_address(argv[1], &server_addr, &server_prefix)) {
        printf("Invalid server address\n");
        return 1;
    }

    // Сначала просто проверяем, что выходной файл можно создать/очистить.
    file = fopen(argv[3], "wb");
    if (file == NULL) {
        printf("Cannot open file\n");
        return 1;
    }
    fclose(file);

    if (!init_winsock()) {
        return 1;
    }

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        cleanup_socket(s);
        return 1;
    }

    if (!set_socket_timeouts(s)) {
        printf("cannot set socket timeouts\n");
        cleanup_socket(s);
        return 1;
    }

    if (!connect_with_retry(s, &server_addr)) {
        cleanup_socket(s);
        return 1;
    }

    // Команда "get" говорит серверу, что нужно вернуть все сообщения из msg.txt.
    if (!send_all(s, "get", 3)) {
        cleanup_socket(s);
        return 1;
    }

    // Открываем файл повторно уже в режиме добавления и начинаем сохранять данные.
    file = fopen(argv[3], "ab");
    if (file == NULL) {
        printf("Cannot open file\n");
        cleanup_socket(s);
        return 1;
    }

    // Сервер сам закроет соединение после передачи последнего сообщения.
    // Пока сообщения читаются успешно, сохраняем их в выходной файл.
    while (recv_message(s, &msg)) {
        write_message(file, server_prefix, &msg);
    }

    fclose(file);
    cleanup_socket(s);
    return 0;
}
