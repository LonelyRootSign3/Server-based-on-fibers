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
    bool Unlink(const std::string& filename, bool exist){
        if(!exist && __lstat(filename.c_str())) {
            return true;
        }
        return ::unlink(filename.c_str()) == 0;
    }

    std::string Trim(const std::string& str, const std::string& delimit){
        size_t start = str.find_first_not_of(delimit);
        if(start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(delimit);
        return str.substr(start, end - start + 1);
    }
    std::string TrimLeft(const std::string& str, const std::string& delimit){
        size_t start = str.find_first_not_of(delimit);
        if(start == std::string::npos) {
            return "";
        }
        return str.substr(start);
    }
    std::string TrimRight(const std::string& str, const std::string& delimit){
        size_t end = str.find_last_not_of(delimit);
        if(end == std::string::npos) {
            return "";
        }
        return str.substr(0, end + 1);
    }

    static const char uri_chars[256] = {//需要进行url编码的字符（下标对应ASCII码）
        /* 0 */
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 1, 1, 0,
        1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 0, 0, 0, 1, 0, 0,
        /* 64 */
        0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
        0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1, 
        1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,
        /* 128 */
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        /* 192 */
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
    };

    static const char xdigit_chars[256] = {// 把 字符型16进制数 所对应的ASCII码值转化成16进制 的表
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,
        0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };

#define IS_URL_CHAR(c) (uri_chars[(unsigned char)(c)] == 1)

    std::string UrlEncode(const std::string& str, bool space_as_plus){
        static const char *hex = "0123456789ABCDEF";
        size_t len = str.size();
        size_t pos = 0;
        while(pos < len && IS_URL_CHAR(str[pos])){
            pos++;
        }
        if(pos == len) return str;
        std::string ret;
        ret.reserve(str.size() * 1.2);//先预开辟1.2倍空间
        ret.append(str, 0, pos);//将pos之前的字符直接append到ret中
        for(; pos < len; pos++){
            if(IS_URL_CHAR(str[pos])){
                ret += str[pos];
            }else{
                if(str[pos] == ' ' && space_as_plus){
                    ret += '+';
                }else{
                    ret += '%';
                    ret += hex[(unsigned char)(str[pos]) >> 4];
                    ret += hex[(unsigned char)(str[pos]) & 0x0F];
                }
            }
        }
        return ret;
    }

    std::string UrlDecode(const std::string& str, bool space_as_plus){
        size_t len = str.size();
        size_t pos = 0;
        while(pos < len){
            if(str[pos] == '%' || (str[pos] == '+' && space_as_plus)) break;
            pos++;
        }
        if(pos == len) return str;
        std::string ret;
        ret.reserve(str.size());//先预开辟空间
        ret.append(str, 0, pos);//将pos之前的字符直接append到ret中

        while(pos < len){
            if(str[pos] == '%' && (pos + 2) < len 
                        && std::isxdigit(static_cast<unsigned char>(str[pos + 1]) 
                        && std::isxdigit(static_cast<unsigned char>(str[pos + 2])))){
                // 使用查表法合并高低位
                char high = xdigit_chars[static_cast<unsigned char>(str[pos + 1])];
                char low  = xdigit_chars[static_cast<unsigned char>(str[pos + 2])];
                ret.push_back(static_cast<char>((high << 4) | low));
                pos += 3; // 跳过已处理的 "%XX"
            }else if(str[pos] == '+' && space_as_plus){
                ret.push_back(' ');
                pos++;
            }else{
                ret += str[pos];
                pos++;
            }
        }
        return ret;

    }


}