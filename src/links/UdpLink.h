#pragma once
#include "Link.h"
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class UdpLink : public Link {
public:
    UdpLink(int port) {
        this->port = port;
        this->sock = -1;
    }

    bool openLink() override {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("socket failed");
            return false;
        }

        sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(port);

        if (bind(sock, (sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind failed");
            close(sock);
            sock = -1;
            return false;
        }
        return true;
    }

    int readBytes(uint8_t* buffer, int maxBytes) override {
        ssize_t bytesReceived = recvfrom(sock, buffer, maxBytes, 0, nullptr, nullptr);
        return (int)bytesReceived;
    }

    void closeLink() override {
        if (sock >= 0) {
            close(sock);
            sock = -1;
        }
    }

private:
    int port;
    int sock;
};