#pragma once
#include "Log.h"
#include "Util.h"
#include <assert.h>
//常用宏方法的定义

// __builtin_expect(exp, c) 是 GCC/Clang 的内建函数，用于告诉编译器：
// exp 的结果很可能是 c。

#if defined __GNUC__ || defined __llvm__ //在 GNU C/C++ 编译器或 LLVM 编译器下
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#   define MY_LIKELY(x)       __builtin_expect(!!(x), 1) //!!(x) —— 强制转换为布尔值（避免非 0 逻辑错误） 1 —— 期待表达式结果为 true

/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#   define MY_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define MY_LIKELY(x)      (x)
#   define MY_UNLIKELY(x)      (x)
#endif

#define MY_ASSERT(x) \
    if(MY_UNLIKELY(!(x))) { \
        STREAM_LOG_ERROR(GET_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << DYX::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }
#define MY_ASSERT2(x, w) \
    if(MY_UNLIKELY(!(x))) { \
        STREAM_LOG_ERROR(GET_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << DYX::BacktraceToString(100, 2, "    "); \
        assert(x); \
    }


