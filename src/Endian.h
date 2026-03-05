#pragma once
#include <byteswap.h>
#include <stdint.h>
#include <type_traits>
//字节序操作函数（大端小端转换）

#define MY_LITTLE_ENDIAN 1
#define MY_BIG_ENDIAN 2

namespace DYX{

//8byte
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value) {
    return (T)bswap_64((uint64_t)value);
}

//4byte
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

//2byte
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
} 

#if BYTE_ORDER == BIG_ENDIAN
#define MY_BYTE_ORDER MY_BIG_ENDIAN
#else
#define MY_BYTE_ORDER MY_LITTLE_ENDIAN
#endif


#if MY_BYTE_ORDER == MY_BIG_ENDIAN
//只在小端机器上执行byteswap, 在大端机器上什么都不做，当前是大端直接返回原值
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}
//只在大端机器上执行byteswap, 在小端机器上什么都不做，当前是大端需要反转字节序
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}

#else
//只在小端机器上执行byteswap, 在大端机器上什么都不做，当前是小端需要反转字节序
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}
//只在大端机器上执行byteswap, 在小端机器上什么都不做，当前是小端直接返回原值
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}

#endif


}

