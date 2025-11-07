#include "Util.h"

namespace DYX{
    inline DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");//确保全工程以 C++17（或更高）编译；inline 变量允许跨 TU 定义一次“合并”为同一实体。   

    pid_t GetThreadId(){//获取内核级别的线程id
        return syscall(SYS_gettid);
    }
    uint32_t GetFiberId(){
        return Fiber::GetFiberId();
    }

    static std::string demangle(const char *str){//去除函数名的编译器修饰
        size_t size = 0;
        int status = 0;
        std::string rt;
        rt.resize(256);
        // 1从符号字符串中提取函数名（用 sscanf）
        if(1 == sscanf(str, "%*[^(]%*[^_]%255[^)+]", &rt[0])) {
	        // 2调用 __cxa_demangle() 解码 C++ 函数名
            char* v = abi::__cxa_demangle(&rt[0], nullptr, &size, &status);
            if(v) {
                // 3成功则返回解码后的结果
                std::string result(v);
                free(v);
                return result;
            }
        }
        // 4否则尝试简单提取函数名
        if(1 == sscanf(str, "%255s", &rt[0])) {
            return rt;
        }
        // 5最后实在不行，返回原始字符串 
        return str;
    }

    void Backtrace(std::vector<std::string>& bt, int size, int skip)//获取调用栈
    {
        void** array = (void**)malloc(sizeof(void*)*size);
        size_t s = ::backtrace(array, size);//获取调用栈地址(返回地址数量)
        char** strings = backtrace_symbols(array, s);//将地址转换成符号字符串
        if(strings == NULL) {
            STREAM_LOG_ERROR(g_logger) << "backtrace_synbols error";
            return;
        }
        for(size_t i = skip; i < s; ++i) {
            bt.push_back(demangle(strings[i]));
        }
        free(strings);
        free(array);
    }
    std::string BacktraceToString(int size, int skip, const std::string& prefix)//将调用栈转换成字符串
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for(size_t i = 0; i < bt.size(); ++i) {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

}