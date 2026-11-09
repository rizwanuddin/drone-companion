#pragma once
#include "Link.h"
#include <string>
#include <cstdio>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

class SerialLink : public Link {
public:
    SerialLink(std::string devicePath) {
        this->devicePath = devicePath;
        this->fd = -1;
    }

    bool openLink() override {
        fd = open(devicePath.c_str(), O_RDWR | O_NOCTTY);
        if (fd < 0) {
            perror("open serial port failed");
            return false;
        }

        termios settings;
        if (tcgetattr(fd, &settings) != 0) {
            perror("reading serial settings failed");
            close(fd);
            fd = -1;
            return false;
        }

        cfmakeraw(&settings);
        settings.c_cflag |= (CLOCAL | CREAD);
        cfsetispeed(&settings, B115200);
        cfsetospeed(&settings, B115200);
        settings.c_cc[VMIN] = 1;
        settings.c_cc[VTIME] = 0;

        if (tcsetattr(fd, TCSANOW, &settings) != 0) {
            perror("applying serial settings failed");
            close(fd);
            fd = -1;
            return false;
        }
        return true;
    }

    int readBytes(uint8_t* buffer, int maxBytes) override {
        ssize_t bytesReceived = read(fd, buffer, maxBytes);
        return (int)bytesReceived;
    }

    int writeBytes(const uint8_t* buffer, int numBytes) override {
        ssize_t bytesSent = write(fd, buffer, numBytes);
        return (int)bytesSent;
    }

    void closeLink() override {
        if (fd >= 0) {
            close(fd);
            fd = -1;
        }
    }

private:
    std::string devicePath;
    int fd;
};