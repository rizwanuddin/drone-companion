#pragma once
#include <cstdint>

class Link {
public:
    virtual ~Link() {}
    virtual bool openLink() = 0;
    virtual int readBytes(uint8_t* buffer, int maxBytes) = 0;
    virtual void closeLink() = 0;
};