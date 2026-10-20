#include <arpa/inet.h>   // inet_pton, htons, htonl, ntohl
#include <netinet/in.h>  // sockaddr_in
#include <sys/socket.h>  // socket, sendto, recvfrom
#include <sys/select.h>  // select
#include <unistd.h>      // close

#include <iostream>
#include <cstring>       // strchr, strlen, memcpy
#include <cstdlib>       // strtol, malloc, realloc, free
#include <cstdio>
#include <cstdint>
#include <sys/types.h>

struct Message {
    uint32_t index;    // Номер сообщения во входном файле.
    char phone1[13];   // Первый телефон. 13, а не 12, чтобы добавить '\0'.
    char phone2[13];   // Второй телефон.
    uint8_t hour;      // Часы.
    uint8_t minute;    // Минуты.
    uint8_t second;    // Секунды.
};

struct PreparedMessage {
    uint32_t index;           // Номер сообщения, которому соответствует дейтаграмма.
    unsigned char* datagram;  // Уже собранный бинарный пакет.
    size_t datagram_len;      // Длина пакета в байтах.
};

bool BuildDatagram(const Message* msg, const char* text, unsigned char** out_buf, size_t* out_len) {
    // Собираем одну UDP-дейтаграмму в формате протокола:
    // index(4) + phone1(12) + phone2(12) + hh(1) + mm(1) + ss(1) + text + '\0'
    if (text == nullptr) return false;

    size_t text_len = strlen(text);
    size_t packet_len = 4 + 12 + 12 + 3 + text_len + 1;
    unsigned char* packet = (unsigned char*)malloc(packet_len);
    if (packet == nullptr) return false;

    size_t offset = 0;
    uint32_t net_index = htonl(msg->index);

    memcpy(packet + offset, &net_index, 4);
    offset += 4;

    memcpy(packet + offset, msg->phone1, 12);
    offset += 12;

    memcpy(packet + offset, msg->phone2, 12);
    offset += 12;

    packet[offset++] = msg->hour;
    packet[offset++] = msg->minute;
    packet[offset++] = msg->second;

    memcpy(packet + offset, text, text_len);
    offset += text_len;

    packet[offset++] = '\0';

    *out_buf = packet;
    *out_len = offset;
    return true;
}

bool ParseServerAddress(const char* s, sockaddr_in* out_addr) {
    // Разбираем строку вида "IP:port" и заполняем sockaddr_in для sendto.
    if (s == nullptr || out_addr == nullptr) {
        return false;
    }

    const char* pos = strchr(s, ':');
    if (pos == nullptr) {
        return false;
    }

    char ip[INET_ADDRSTRLEN];
    size_t ip_len = pos - s;
    if (ip_len == 0 || ip_len >= INET_ADDRSTRLEN) {
        return false;
    }
    memcpy(ip, s, ip_len);
    ip[ip_len] = '\0';

    char* end = nullptr;
    long port = strtol(pos + 1, &end, 10);
    if (end == pos + 1 || *end != '\0' || port < 1 || port > 65535) {
        return false;
    }

    if (inet_pton(AF_INET, ip, &out_addr->sin_addr) != 1) {
        return false;
    }

    out_addr->sin_family = AF_INET;
    out_addr->sin_port = htons((uint16_t)port);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: udpclient <ip:port> <file>\n";
        return 1;
    }

    sockaddr_in serv_addr{}; // Адрес сервера, куда будут отправляться все дейтаграммы.

    const char* arg_addr = argv[1];
    if (!ParseServerAddress(arg_addr, &serv_addr)) {
        std::cerr << "Invalid server address\n";
        return 1;
    }

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        std::cerr << "Error creation client socket\n";
        return 1;
    }

    FILE* f = fopen(argv[2], "r");
    if (f == nullptr) {
        std::cerr << "Cannot open file\n";
        close(s);
        return 1;
    }

    char* line = nullptr;   // Сюда getline будет класть очередную строку файла.
    size_t capacity = 0;    // Текущий размер буфера line.
    ssize_t read_len = 0;   // Длина считанной строки.

    uint32_t current_index = 0; // Номер текущего сообщения по протоколу.

    PreparedMessage* msgs = NULL; // Массив всех подготовленных дейтаграмм.
    size_t msg_count = 0;         // Сколько сообщений удалось подготовить.

    while ((read_len = getline(&line, &capacity, f)) != -1) {
        // Убираем перевод строки в конце, чтобы потом не отправлять его серверу.
        while (read_len > 0 && (line[read_len - 1] == '\n' || line[read_len - 1] == '\r')) {
            line[--read_len] = '\0';
        }

        // Пустые и слишком короткие строки считаем некорректными и пропускаем.
        if (read_len == 0) continue;
        if (read_len < 35) continue;

        Message msg{};
        msg.index = current_index;

        // Копируем телефоны из входной строки.
        memcpy(msg.phone1, line, 12);
        msg.phone1[12] = '\0';

        memcpy(msg.phone2, line + 13, 12);
        msg.phone2[12] = '\0';

        // Переводим время из текстового вида "hh:mm:ss" в три отдельных числа.
        msg.hour = (line[26] - '0') * 10 + (line[27] - '0');
        msg.minute = (line[29] - '0') * 10 + (line[30] - '0');
        msg.second = (line[32] - '0') * 10 + (line[33] - '0');

        const char* text_ptr = line + 35;   // Текст начинается после "phone1 phone2 hh:mm:ss ".
        unsigned char* datagram = nullptr;  // Готовый бинарный пакет.
        size_t datagram_len = 0;            // Его длина.
        if (!BuildDatagram(&msg, text_ptr, &datagram, &datagram_len)) {
            continue;
        }

        // Добавляем собранную дейтаграмму в общий массив сообщений.
        PreparedMessage* new_msgs = (PreparedMessage*)realloc(msgs, (msg_count + 1) * sizeof(PreparedMessage));
        if (new_msgs == NULL) {
            free(datagram);
            break;
        }
        msgs = new_msgs;

        msgs[msg_count].index = msg.index;
        msgs[msg_count].datagram = datagram;
        msgs[msg_count].datagram_len = datagram_len;
        msg_count++;

        current_index++;
    }

    // Сервер в ответ возвращает не больше 20 последних id, поэтому клиент ждёт
    // подтверждения либо по всем сообщениям, либо максимум по 20.
    size_t target;
    if (msg_count < 20) {
        target = msg_count;
    } else {
        target = 20;
    }

    unsigned char* acked = (unsigned char*)calloc(msg_count, 1); // Флаг подтверждения для каждого сообщения.
    size_t acked_count = 0;                                       // Сколько сообщений уже подтверждено.

    // Сначала один раз отправляем все подготовленные сообщения.
    for (size_t i = 0; i < msg_count; ++i) {
        if (acked[i]) continue;

        sendto(s, msgs[i].datagram, msgs[i].datagram_len,
               0, (sockaddr*)&serv_addr, sizeof(serv_addr));
    }

    // Дальше работаем по схеме:
    // ждём ответ 100 мс, если ответа нет — повторно отправляем всё неподтверждённое.
    while (acked_count < target) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(s, &rfds);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 мс

        int rc = select(s + 1, &rfds, NULL, NULL, &tv);

        if (rc == 0) {
            // Таймаут означает, что ответ от сервера не пришёл.
            // Повторяем отправку всех сообщений, по которым ещё нет подтверждения.
            for (size_t i = 0; i < msg_count; ++i) {
                if (acked[i]) continue;

                sendto(s, msgs[i].datagram, msgs[i].datagram_len,
                       0, (sockaddr*)&serv_addr, sizeof(serv_addr));
            }
        }

        if (rc > 0 && FD_ISSET(s, &rfds)) {
            unsigned char recvbuf[65535]; // Сюда приходит ответ сервера.
            sockaddr_in from{};
            socklen_t from_len = sizeof(from);
            ssize_t r = recvfrom(s, recvbuf, sizeof(recvbuf), 0, (sockaddr*)&from, &from_len);

            if (r > 0 && (r % 4 == 0)) {
                // Ответ сервера — это набор msg_id, по 4 байта на каждый.
                for (ssize_t off = 0; off + 4 <= r; off += 4) {
                    uint32_t net_idx;
                    memcpy(&net_idx, recvbuf + off, 4);
                    uint32_t idx = ntohl(net_idx);

                    // Если такой id ещё не был отмечен, считаем сообщение подтверждённым.
                    if (idx < msg_count && !acked[idx]) {
                        acked[idx] = 1;
                        acked_count++;
                    }
                }
            }
        }
    }

    free(acked);
    for (size_t i = 0; i < msg_count; ++i) {
        free(msgs[i].datagram);
    }
    free(msgs);
    free(line);
    fclose(f);
    close(s);
    return 0;
}
