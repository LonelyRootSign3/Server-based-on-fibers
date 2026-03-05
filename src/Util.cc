#include "Util.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <cstring>
namespace DYX{
    static DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");  

    pid_t GetThreadId(){//获取内核级别的线程id
        return syscall(SYS_gettid);
    }
    int GetFiberId(){
        return Fiber::GetId();
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

    uint64_t GetCurrentMS(){//获取当前时间的毫秒
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000 + tv.tv_usec / 1000;
    }
    uint64_t GetCurrentUS(){//获取当前时间的微秒
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    }
    std::string Time2Str(time_t ts, const std::string& format){
        struct tm tm;
        localtime_r(&ts, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), format.c_str(), &tm);
        return buf;
    }
    time_t Str2Time(const char* str, const char* format){
        struct tm t;
        memset(&t, 0, sizeof(t));
        if(!strptime(str, format, &t)) {
            return 0;
        }
        return mktime(&t);
    }


    static int __lstat(const char* file, struct stat* st = nullptr) {
        struct stat lst;
        int ret = lstat(file, &lst);
        if(st) {
            *st = lst;
        }
        return ret;
    }
    bool Unlink(const std::string& filename, bool exist = false){
        if(!exist && __lstat(filename.c_str())) {
            return true;
        }
        return ::unlink(filename.c_str()) == 0;
    }

}