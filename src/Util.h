#pragma once
#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <typeinfo>
#include <vector>
#include <string>
#include "Log.h"
#include "Fiber.h"
namespace DYX{
    //获取内核级别的线程id
    pid_t GetThreadId();
    //获取协程id
    uint32_t GetFiberId();

    //获取类型名
    template<typename T>
    const char* TypeToName(){
         // 定义一个静态局部变量 s_name，只在第一次调用时初始化，之后所有调用都返回这个地址
        static const char *name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);//abi::__cxa_demangle 内部\
        会 malloc 一块内存，而 static 指针存下来避免了悬空指针的问题。
        // typeid 是 RTTI（运行时类型识别）工具，typeid(T).name() 返回一个编译器相关的字符串，表示类型 T 的名字。
        // 但这个名字通常是被“mangled”过的，比如：
        // int 可能返回 "i"
        // std::string 可能返回 "NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE"
        // abi::__cxa_demangle
        // 这是 GCC/Clang 提供的函数，用于把上面“乱七八糟”的 mangled 名字**解码（demangle）**成可读的字符串。
        // 比如：
        // 输入 "i" → 输出 "int"
        // 输入 "NSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE" → 输出 "std::string"

        return name;
    }
    
    void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);//获取调用栈

    /**
     * @brief 获取当前栈信息的字符串
     * @param[in] size 栈的最大层数
     * @param[in] skip 跳过栈顶的层数
     * @param[in] prefix 栈信息前输出的内容
     */
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");//将调用栈转换成字符串


}    