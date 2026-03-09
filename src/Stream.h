#pragma once
#include "Bytearray.h"
#include <memory>


namespace DYX{

class Stream{
public:
    virtual ~Stream(){}
    virtual int read(void *buf,size_t len) = 0;
    virtual int read(ByteArray::Ptr ba,size_t len) = 0;
    virtual int write(const void *buf,size_t len) = 0;
    virtual int write(ByteArray::Ptr ba,size_t len) = 0;
    virtual void close() = 0;

    int readfixsize(void *buf,size_t len);
    int readfixsize(ByteArray::Ptr ba,size_t len);
    int writefixsize(const void *buf,size_t len);
    int writefixsize(ByteArray::Ptr ba,size_t len);
};
}

