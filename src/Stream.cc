#include "Stream.h"

namespace DYX{

int Stream::readfixsize(void *buf,size_t len){
    size_t offset = 0;
    int64_t left = len;
    while(left > 0){
        int64_t lent = read((char *)buf+offset,left);
        if(lent <= 0) return lent;
        offset+=lent;
        left-=lent;
    }
    return len;
}

int Stream::readfixsize(ByteArray::Ptr ba,size_t len){
    int64_t left = len;
    while(left > 0){
        int64_t lent = read(ba,left);
        if(lent <= 0) return lent;
        left-=lent;
    }
    return len;
}

int Stream::writefixsize(const void *buf,size_t len){
    size_t offset = 0;
    int64_t left = len;
    while(left > 0){
        int64_t lent = write((const char *)buf+offset,left);
        if(lent <= 0) return lent;
        offset+=lent;
        left-=lent;
    }
    return len;
}

int Stream::writefixsize(ByteArray::Ptr ba,size_t len){
    int64_t left = len;
    while(left > 0){
        int64_t lent = write(ba,left);
        if(lent <= 0) return lent;
        left-=lent;
    }
    return len;
}

}
