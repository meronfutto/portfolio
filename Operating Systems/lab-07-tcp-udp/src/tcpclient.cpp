#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

struct Message {
    unsigned int index;         // Номер сообщения в файле: 0, 1, 2, ...
    char phone1[13];            // Первый телефон. 13, а не 12, чтобы добавить '\0' в конец.
    char phone2[13];            // Второй телефон.
    unsigned char hour;         // Часы 0..23
    unsigned char minute;       // Минуты 0..59
    unsigned char second;       // Секунды 0..59
};

// Проверяем, что телефон имеет вид +7xxxxxxxxxx.
int CheckPhone(const char *phone) {
    int i;

    if (phone == NULL) return 0;
    if (strlen(phone) != 12) return 0;
    if (phone[0] != '+' || phone[1] != '7') return 0;

    for (i = 2; i < 12; ++i) {
        if (phone[i] < '0' || phone[i] > '9') return 0;
    }

    return 1;
}

// Проверяем корректность времени.
int CheckTime(int hh, int mm, int ss) {
    if (hh < 0 || hh > 23) return 0;
    if (mm < 0 || mm > 59) return 0;
    if (ss < 0 || ss > 59) return 0;
    return 1;
}

// В тексте не должно быть пустой строки и переводов строки.
int CheckText(const char *text) {
    int i;

    if (text == NULL || text[0] == '\0') return 0;

    for (i = 0; text[i] != '\0'; ++i) {
        if (text[i] == '\n' || text[i] == '\r') return 0;
    }

    return 1;
}

// send может отправить не все байты сразу, поэтому досылаем в цикле.
int SendAll(SOCKET s, const char *data, int size) {
    int total = 0; // Сколько байт уже удалось отправить.

    while (total < size) {
        int sent = send(s, data + total, size - total, 0);
        if (sent <= 0) {
            printf("send failed: %d\n", WSAGetLastError());
            return 0;
        }
        total += sent;
    }

    return 1;
}

// recv тоже может вернуть только часть данных, поэтому дочитываем до конца.
int RecvAll(SOCKET s, char *data, int size) {
    int total = 0; // Сколько байт уже получили.

    while (total < size) {
        int received = recv(s, data + total, size - total, 0);
        if (received == SOCKET_ERROR) {
            printf("recv failed: %d\n", WSAGetLastError());
            return 0;
        }
        if (received == 0) {
            printf("Connection closed by server\n");
            return 0;
        }
        total += received;
    }

    return 1;
}

// Собираем одно полное сообщение по протоколу и отправляем его целиком.
int SendMessage(SOCKET s, const Message *msg, const char *text) {
    size_t text_len = strlen(text);
    size_t msg_size = 4 + 12 + 12 + 3 + text_len + 1;
    char *sendbuf = (char *)malloc(msg_size);
    unsigned int net_index;
    size_t pos = 0;
    int result = 0;

    if (sendbuf == NULL) {
        printf("memory allocation failed\n");
        return 0;
    }

    net_index = htonl(msg->index);

    memcpy(sendbuf + pos, &net_index, 4);
    pos += 4;

    memcpy(sendbuf + pos, msg->phone1, 12);
    pos += 12;

    memcpy(sendbuf + pos, msg->phone2, 12);
    pos += 12;

    sendbuf[pos++] = (char)msg->hour;
    sendbuf[pos++] = (char)msg->minute;
    sendbuf[pos++] = (char)msg->second;

    memcpy(sendbuf + pos, text, text_len);
    pos += text_len;

    sendbuf[pos] = '\0';

    result = SendAll(s, sendbuf, (int)msg_size);
    free(sendbuf);
    return result;
}

// Читаем одну строку файла целиком, даже если она длиннее обычного буфера.
int ReadLongLine(FILE *f, char **line_out) {
    char part[4096];
    char *line = NULL;
    size_t used = 0;

    *line_out = NULL;

    while (fgets(part, sizeof(part), f) != NULL) {
        size_t part_len = strlen(part);
        char *new_line = (char *)realloc(line, used + part_len + 1);

        if (new_line == NULL) {
            free(line);
            return 0;
        }

        line = new_line;
        memcpy(line + used, part, part_len + 1);
        used += part_len;

        if (used > 0 && line[used - 1] == '\n') {
            break;
        }
    }

    if (line == NULL) {
        return 0;
    }

    *line_out = line;
    return 1;
}

// Общее завершение клиента: закрываем все ресурсы и возвращаем код выхода.
int Finish(FILE *f, SOCKET s, int code) {
    if (s != INVALID_SOCKET) {
        closesocket(s);
    }
    if (f != NULL) {
        fclose(f);
    }
    WSACleanup();
    return code;
}

int main(int argc, char *argv[]) {
    FILE *f = NULL;             // Входной файл с текстовыми сообщениями.
    SOCKET s = INVALID_SOCKET;  // Сокет клиента для связи с сервером.
    unsigned int index = 0;     // Номер текущего сообщения по протоколу.

    if (argc != 3) {
        printf("Usage: tcpclient <ip:port> <file>\n");
        return 1;
    }

    f = fopen(argv[2], "rb");
    if (f == NULL) {
        printf("Cannot open file\n");
        return 1;
    }

    {
        WSADATA wsa; // Служебная структура WinSock.
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            printf("WSAStartup failed\n");
            return Finish(f, s, 1);
        }
    }

    {
        char server_text[256];   // Строка вида 127.0.0.1:9000
        char *colon = NULL;      // Указатель на символ ':' внутри строки адреса.
        char *end = NULL;        // Нужен для strtol, чтобы проверить корректность порта.
        long port = 0;           // Числовое значение порта после разбора строки.
        sockaddr_in server_addr; // Структура адреса сервера для connect.
        int attempt;             // Номер попытки подключения.

        // Копируем адрес сервера в свой буфер, чтобы можно было безопасно менять строку.
        strncpy(server_text, argv[1], sizeof(server_text) - 1);
        server_text[sizeof(server_text) - 1] = '\0';

        // Ищем двоеточие между IP и портом.
        colon = strchr(server_text, ':');
        if (colon == NULL) {
            printf("Invalid server address\n");
            return Finish(f, s, 1);
        }

        // Разрезаем строку на две части:
        // server_text -> IP
        // colon      -> строка порта
        *colon = '\0';
        ++colon;

        if (server_text[0] == '\0' || colon[0] == '\0') {
            printf("Invalid server address\n");
            return Finish(f, s, 1);
        }

        // Преобразуем порт из текста в число.
        port = strtol(colon, &end, 10);
        if (*end != '\0' || port <= 0 || port > 65535) {
            printf("Invalid server address\n");
            return Finish(f, s, 1);
        }

        // Заполняем адрес сервера для будущего connect.
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons((unsigned short)port);
        server_addr.sin_addr.s_addr = inet_addr(server_text);
        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
            printf("Invalid server address\n");
            return Finish(f, s, 1);
        }

        // По условию делаем до 10 попыток подключения с паузой 100 мс.
        // Каждый раз создаём новый сокет и пробуем connect.
        for (attempt = 0; attempt < 10; ++attempt) {
            s = socket(AF_INET, SOCK_STREAM, 0);
            if (s == INVALID_SOCKET) {
                Sleep(100);
                continue;
            }

            if (connect(s, (sockaddr *)&server_addr, sizeof(server_addr)) == 0) {
                break;
            }

            closesocket(s);
            s = INVALID_SOCKET;
            Sleep(100);
        }

        if (s == INVALID_SOCKET) {
            printf("connect failed\n");
            return Finish(f, s, 1);
        }
    }

    // По протоколу сначала отправляем команду "put":
    // это говорит серверу, что дальше пойдут сообщения.
    if (!SendAll(s, "put", 3)) {
        return Finish(f, s, 1);
    }

    while (1) {
        Message msg;            // Разобранное сообщение в удобном виде.
        char *line = NULL;      // Одна строка из входного файла любой длины.
        const char *text;       // Указатель на начало текстовой части сообщения.
        int hh;                 // Часы как обычное число int.
        int mm;                 // Минуты как обычное число int.
        int ss;                 // Секунды как обычное число int.
        char ok[2];             // Сюда читаем ответ сервера: 'o' 'k'.
        size_t len;             // Длина строки line.

        // Читаем очередную строку из файла. Если файл закончился, выходим из цикла.
        if (!ReadLongLine(f, &line)) break;
        if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
            free(line);
            continue;
        }

        // Убираем перевод строки в конце, если он есть.
        len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            --len;
        }

        // Минимально проверяем формат:
        // +7xxxxxxxxxx +7xxxxxxxxxx hh:mm:ss text
        if (line[0] == '\0') {
            free(line);
            continue;
        }
        if (strlen(line) < 36) {
            free(line);
            continue;
        }
        if (line[12] != ' ' || line[25] != ' ' || line[34] != ' ') {
            free(line);
            continue;
        }

        // Копируем телефоны из строки в структуру сообщения.
        memcpy(msg.phone1, line, 12);
        memcpy(msg.phone2, line + 13, 12);
        msg.phone1[12] = '\0';
        msg.phone2[12] = '\0';

        if (!CheckPhone(msg.phone1) || !CheckPhone(msg.phone2)) {
            free(line);
            continue;
        }

        // Проверяем, что время действительно записано цифрами и двоеточиями.
        if (line[28] != ':' || line[31] != ':') {
            free(line);
            continue;
        }
        if (line[26] < '0' || line[26] > '9') {
            free(line);
            continue;
        }
        if (line[27] < '0' || line[27] > '9') {
            free(line);
            continue;
        }
        if (line[29] < '0' || line[29] > '9') {
            free(line);
            continue;
        }
        if (line[30] < '0' || line[30] > '9') {
            free(line);
            continue;
        }
        if (line[32] < '0' || line[32] > '9') {
            free(line);
            continue;
        }
        if (line[33] < '0' || line[33] > '9') {
            free(line);
            continue;
        }

        // Переводим время из текстового вида "hh:mm:ss" в обычные числа.
        hh = (line[26] - '0') * 10 + (line[27] - '0');
        mm = (line[29] - '0') * 10 + (line[30] - '0');
        ss = (line[32] - '0') * 10 + (line[33] - '0');

        if (!CheckTime(hh, mm, ss)) {
            free(line);
            continue;
        }

        // Копируем разобранные поля в структуру сообщения.
        msg.hour = (unsigned char)hh;
        msg.minute = (unsigned char)mm;
        msg.second = (unsigned char)ss;
        text = line + 35;

        if (!CheckText(text)) {
            free(line);
            continue;
        }

        msg.index = index;

        // Как и в Ruby-эмуляторе клиента, собираем всё сообщение в один буфер
        // и только потом отправляем его на сервер.
        if (!SendMessage(s, &msg, text)) {
            free(line);
            return Finish(f, s, 1);
        }

        // После каждого сообщения ждём от сервера ответ "ok".
        if (!RecvAll(s, ok, 2)) {
            free(line);
            return Finish(f, s, 1);
        }

        if (ok[0] != 'o' || ok[1] != 'k') {
            printf("Invalid confirmation\n");
            free(line);
            return Finish(f, s, 1);
        }

        free(line);
        ++index;
    }

    return Finish(f, s, 0);
}
