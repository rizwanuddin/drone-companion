#pragma once
#include "Link.h"
#include <string>
#include <cstring>
#include <cstdio>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class UdpLink : public Link {
public:
    // LISTEN mode: wait on a port for data (SITL via MAVProxy)
    UdpLink(int listenPort) {
        this->isClient = false;
        this->port = listenPort;
        this->remoteIp = "";
        this->sock = -1;
        this->hasSender = false;
        memset(&lastSender, 0, sizeof(lastSender));
    }

    // CLIENT mode: talk first to a server (mavlink-router on the Pi)
    UdpLink(std::string remoteIp, int remotePort) {
        this->isClient = true;
        this->port = remotePort;
        this->remoteIp = remoteIp;
        this->sock = -1;
        this->hasSender = false;
        memset(&lastSender, 0, sizeof(lastSender));
    }

    bool openLink() override {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            perror("socket failed");
            return false;
        }

        // Give up waiting for data after 1 second, so the program can send heartbeats
        timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        if (isClient == true) {
            // We know the server's address up front, so we can send immediately
            lastSender.sin_family = AF_INET;
            lastSender.sin_addr.s_addr = inet_addr(remoteIp.c_str());
            lastSender.sin_port = htons(port);
            hasSender = true;
            return true;
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
        sockaddr_in sender;
        socklen_t senderLength = sizeof(sender);
        ssize_t bytesReceived = recvfrom(sock, buffer, maxBytes, 0, (sockaddr*)&sender, &senderLength);

        if (bytesReceived > 0) {
            lastSender = sender;
            hasSender = true;
        }
        return (int)bytesReceived;
    }

    int writeBytes(const uint8_t* buffer, int numBytes) override {
        if (hasSender == false) {
            return 0;
        }
        ssize_t bytesSent = sendto(sock, buffer, numBytes, 0, (sockaddr*)&lastSender, sizeof(lastSender));
        return (int)bytesSent;
    }

    void closeLink() override {
        if (sock >= 0) {
            close(sock);
            sock = -1;
        }
    }

private:
    bool isClient;
    int port;
    std::string remoteIp;
    int sock;
    sockaddr_in lastSender;
    bool hasSender;
};