#include "Log.h"
#include "Config.h"
#include "Logdefine.h"
#include <tuple>
#include <unordered_map>
#include <functional>
namespace DYX{
    //日志等级相关操作函数
    std::string GetlevelStr(Loglevel level)//日志级别转字符串
    {
        switch(level)
        {
            case Loglevel::DEBUG:
                return "DEBUG";
            case Loglevel::INFO:
                return "INFO";
            case Loglevel::WARN:
                return "WARN";
            case Loglevel::ERROR:
                return "ERROR";
            case Loglevel::FATAL:
                return "FATAL";
            default:
                return "UNKNOWN";
        }
    }
    Loglevel GetlevelInt(const std::string &levelStr)//字符串转日志级别
    {
        //全部转为大写,忽略大小写问题
        std::string str = levelStr; // 先深拷贝一份
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        if(str == "DEBUG"){
            return Loglevel::DEBUG;
        }else if(str == "INFO"){
            return Loglevel::INFO;
        }else if(str == "WARN"){
            return Loglevel::WARN;
        }else if(str == "ERROR"){   
            return Loglevel::ERROR;
        }else if(str == "FATAL"){
            return Loglevel::FATAL;
        }else{
            return Loglevel::UNKNOWN;
        }
    }
    ///LogEvent类实现
    void LogEvent::format(const char *fmt,...)
    {
            va_list al;
            va_start(al, fmt);
            format(fmt, al);
            va_end(al);
    }
    void LogEvent::format(const char *fmt, va_list al)
    {
            char* buf = nullptr;
            int len = vasprintf(&buf, fmt, al);//传入 buf 的地址。vasprintf 会自己调用 malloc（或类似函数）为生成的字符串动态分配足够大小的内存，\
                                    并让 buf 指向这块内存。调用者负责后续用 free 释放这块内存
            if(len != -1) {
                _ss << std::string(buf, len);
                free(buf);
            }
    }
    //LogEventWrapper类实现
    LogEventWrapper::~LogEventWrapper() {
        _event->_logger->Log(_event->_level,_event);
        // std::cout<<GetlevelStr(_event->_logger->GetLevel())<<std::endl;
        // std::cout<<"~Wrapper"<<std::endl;
    }

    ///FormatItem子类实现
    class MesgFormatItem : public LogFormatter::FormatItem {//日志内容
    public:
        MesgFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_ss.str();
        }
    };
    class LevelFormatItem : public LogFormatter::FormatItem {//日志等级
    public:
        LevelFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            // os<<GetlevelStr(level);
            os<<GetlevelStr(event->_level);
            // std::cout<<"event level: "<<GetlevelStr(event->_level)<<std::endl;
            // std::cout<<"logger level: "<<GetlevelStr(logger->GetLevel())<<std::endl;

        }
    };
    class ElapseFormatItem : public LogFormatter::FormatItem {//程序从开始到现在累计毫秒数
    public:
        ElapseFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_elapse;
        }
    };
    class LoggerNameFormatItem : public LogFormatter::FormatItem {//日志器名称
    public:
        LoggerNameFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_logger->GetName();
        }
    };
    class NewLineFormatItem : public LogFormatter::FormatItem {//换行
    public:
        NewLineFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<std::endl;
        }
    };
    class LineFormatItem : public LogFormatter::FormatItem {//行号
    public:
        LineFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_line;
        }
    };
    class DateTimeFormatItem : public LogFormatter::FormatItem {//日期时间
    public:
        DateTimeFormatItem(const std::string &str = "%Y-%m-%d %H:%M:%S")
            :_format(str)
        {
            // if(_format.empty()){
            //     _format = "%Y-%m-%d %H:%M:%S";
            // }
        }
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            struct tm tm_time;
            time_t time = event->_time;
            localtime_r(&time, &tm_time);
            char time_str[64] = {0};
            strftime(time_str, sizeof(time_str), _format.c_str(), &tm_time);
            os<<time_str;
        }
    private:
        std::string _format;
    };
    class FilenameFormatItem : public LogFormatter::FormatItem {//文件名
    public:
        FilenameFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_filename;
        }
    };
    class TabFormatItem : public LogFormatter::FormatItem {//Tab键
    public:
        TabFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<"\t";
        }
    };
    class StringFormatItem : public LogFormatter::FormatItem {//字符串
    public:
        StringFormatItem(const std::string &str = ""):_str(str){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<_str;
        }
    private:
        std::string _str;
    };
    class FiberIdFormatItem : public LogFormatter::FormatItem {//协程id
    public:
        FiberIdFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_fiber_id;
        }
    };
    class ThreadIdFormatItem : public LogFormatter::FormatItem {//线程id
    public:
        ThreadIdFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_thread_id;
        }
    };
    class ThreadNameFormatItem : public LogFormatter::FormatItem {//线程名称
    public:
        ThreadNameFormatItem(const std::string &str = ""){}
        void Format_Item(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) override{
            os<<event->_thread_name;
        }
    };

/***********************************
     * LogFormatter类实现
     * 内部类FormatItem派生类的实现
     * 
     * XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, LoggerNameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FilenameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
        XX(N, ThreadNameFormatItem),        //N:线程名称
     ************************/

    LogFormatter::LogFormatter(const std::string &pattern) : _pattern(pattern) { Init();}

    std::string LogFormatter::Format(std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event)
    {
        std::stringstream ss;
        for(auto &item:_format_items){
            item->Format_Item(ss, logger, level, event);
        }
        return ss.str();
    }
    std::ostream &LogFormatter::Format(std::ostream &ofs, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event)
    {
        for(auto &item:_format_items){
            item->Format_Item(ofs, logger, level, event);
        }
        return ofs;
    }

    /**   Init()函数实现
     * 简单的状态机判断，提取pattern中的常规字符和模式字符
     *  
     * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
     * 
     * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
     * 
     * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这个状态需要特殊处理
     * 
     * 一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
     * 
     * @see LogFormatter::LogFormatter
     */
    void LogFormatter::Init()
    {
        std::vector<std::tuple<std::string, std::string, int> > vec;
        std::string nstr;
        for(size_t i = 0; i < _pattern.size(); ++i) {
            if(_pattern[i] != '%') {
                nstr.append(1, _pattern[i]);
                continue;
            }

            if((i + 1) < _pattern.size()) {
                if(_pattern[i + 1] == '%') {
                    nstr.append(1, '%');
                    continue;
                }
            }

            size_t n = i + 1;
            int fmt_status = 0;
            size_t fmt_begin = 0;

            std::string str;
            std::string fmt;
            while(n < _pattern.size()) {
                if(!fmt_status && (!isalpha(_pattern[n]) && _pattern[n] != '{'
                        && _pattern[n] != '}')) {
                    str = _pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if(fmt_status == 0) {
                    if(_pattern[n] == '{') {
                        str = _pattern.substr(i + 1, n - i - 1);
                        //std::cout << "*" << str << std::endl;
                        fmt_status = 1; //解析格式
                        fmt_begin = n;
                        ++n;
                        continue;
                    }
                } else if(fmt_status == 1) {
                    if(_pattern[n] == '}') {
                        fmt = _pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        //std::cout << "#" << fmt << std::endl;
                        fmt_status = 0;
                        ++n;
                        break;
                    }
                }
                ++n;
                if(n == _pattern.size()) {
                    if(str.empty()) {
                        str = _pattern.substr(i + 1);
                    }
                }
            }

            if(fmt_status == 0) {
                if(!nstr.empty()) {
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                vec.push_back(std::make_tuple(str, fmt, 1));
                i = n - 1;
            } else if(fmt_status == 1) {
                std::cout << "pattern parse error: " << _pattern << " - " << _pattern.substr(i) << std::endl;
                _has_error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }

        if(!nstr.empty()) {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }
        static std::unordered_map<std::string, std::function<FormatItem::Ptr(const std::string& str)> > s_format_items = {
    #define XX(str, C) \
            {#str, [](const std::string& fmt) { return FormatItem::Ptr(new C(fmt));}}

            XX(m, MesgFormatItem),           //m:消息
            XX(p, LevelFormatItem),             //p:日志级别
            XX(r, ElapseFormatItem),            //r:累计毫秒数
            XX(c, LoggerNameFormatItem),              //c:日志名称
            XX(t, ThreadIdFormatItem),          //t:线程id
            XX(n, NewLineFormatItem),           //n:换行
            XX(d, DateTimeFormatItem),          //d:时间
            XX(f, FilenameFormatItem),          //f:文件名
            XX(l, LineFormatItem),              //l:行号
            XX(T, TabFormatItem),               //T:Tab
            XX(F, FiberIdFormatItem),           //F:协程id
            XX(N, ThreadNameFormatItem),        //N:线程名称
    #undef XX
        };

        for(auto& i : vec) {
            if(std::get<2>(i) == 0) {
                _format_items.push_back(FormatItem::Ptr(new StringFormatItem(std::get<0>(i))));
            } else {
                auto it = s_format_items.find(std::get<0>(i));
                if(it == s_format_items.end()) {
                    _format_items.push_back(FormatItem::Ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    _has_error = true;
                } else {
                    _format_items.push_back(it->second(std::get<1>(i)));
                }
            }
        }
    }

    // void LogFormatter::Init()
    // {
    //     //存放类别
    //     enum class Type{
    //         NORMAL = 0,//常规字符
    //         ESCAPE     //转义字符
    //     };
    //     //解析状态
    //     enum class Status{
    //         ITEM = 0,//正在解析格式化项名称（如 d, p 等）
    //         FORMAT//正在解析格式参数（如 %Y-%m-%d）
    //     };
    //     // 按顺序存储解析到的pattern项
    //     //item, format, type  --每个pattern包括一个格式化项名称（如%Y, %m, %d, %H, %M, %S）,\
    //     一个格式化字符串(如%Y-%%m-%%d %%H:%%M:%%S),一个类型(常规字符或转义字符)
    //     //创建vector存储解析结果，每个元素是三元组：(字符串内容, 格式参数, 类型)
    //     std::vector<std::tuple<std::string,std::string, Type>> patterns;

    //     std::string normal_str;//缓冲区，用于存储普通字符
        
    //     std::cout<<_pattern<<std::endl;


    //     for(int i = 0;i<_pattern.size();++i){
    //         //普通字符
    //         if(_pattern[i] != '%'){//如果不是%，则直接添加到缓冲区
    //             normal_str += _pattern[i];
    //             continue;
    //         }
    //         //%%转义字符->%
    //         if(i+1 < _pattern.size() && _pattern[i+1] == '%'){
    //             //如果是%%，则添加一个普通字符%
    //             normal_str += '%';
    //             continue;
    //         }

    //         //%后面的字符解析(使用状态机)
    //         size_t n = i + 1;//%后面第一个字符的位置，也是开始解析的位置
    //         Status format_status = Status::ITEM;//解析状态，默认为常规字符
    //         size_t format_start = 0;//格式参数开始位置

    //         std::string item_str;//格式化项名称（如%Y, %m, %d, %H, %M, %S）
    //         std::string format_str;//格式化参数（如%Y-%%m-%%d %%H:%%M:%%S）  %d{%Y-%m-%d} → 提取 "%Y-%m-%d" {}内部的就是格式化参数
            
            
    //         // 解析格式化项的名称和可选格式参数
    //         while(n < _pattern.size()){
    //             //1、遇到非字母且非{}的字符（状态0）
    //             if(format_status == Status::ITEM 
    //                 && _pattern[n] != '{' 
    //                 && _pattern[n] != '}'){
    //                     item_str = _pattern.substr(i+1, n-i-1);//提取格式化项名称  exp:   %d abc → 遇到空格，提取 "d" 作为名称
    //                     break;
    //             }
    //             //2、遇到{（状态0→状态1）
    //             if(_pattern[n] == '{' && format_status == Status::ITEM){
    //                 item_str = _pattern.substr(i+1, n-i-1);     //提取格式化项名称  exp : %d{%Y → 提取 "d" 作为名称，开始解析格式参数
    //                 format_status = Status::FORMAT;     //进入格式参数解析状态
    //                 format_start = n;            //格式参数开始位置  -> 遇到{，记录位置，进入格式参数解析状态
    //                 ++n;
    //                 continue;
    //             }
    //             //3、遇到}（状态1→状态0）
    //             else if(_pattern[n] == '}' && format_status == Status::FORMAT){
    //                 //format_start指向的是{  ,所以起始位置需要+1 
    //                 format_str = _pattern.substr(format_start+1, n-(format_start+1)); //提取格式参数  exp : %d{%Y-%m-%d} → 提取 "%Y-%m-%d" 作为格式参数
    //                 format_status = Status::ITEM; //退出格式参数解析状态, 回到格式化项名称解析状态
    //                 ++n;   //跳过}
    //                 break;
    //             }
    //             //4、到达字符串末尾，状态还是0，则提取剩余的字符作为名称
    //             ++n;//->判断是否到达字符串末尾，如果是，则提取剩余的字符作为名称
    //             if(n == _pattern.size()){//如果到达字符串末   exp : %d（字符串结束）→ 提取 "d" 作为名称
    //                 item_str = _pattern.substr(i+1);//从i+1开始到字符串末，作为名称
    //             }
    //         }

    //         if(format_status == Status::ITEM){
    //             if(!normal_str.empty()){//先处理普通字符
    //                 patterns.push_back(std::make_tuple(normal_str, "", Type::NORMAL));
    //                 normal_str.clear();
    //             }
    //             //将上述的转移字符添加到patterns中
    //             patterns.push_back(std::make_tuple(item_str,format_str, Type::ESCAPE));
    //             i = n - 1;
    //             // 解析过程：
    //             // 原始字符串: A B C % d { Y - m - d } E F G
    //             // 索引:       0 1 2 3 4 5 6 7 8 9 10 11 12 13

    //             // 解析过程：
    //             // - i=3：遇到 %，开始解析
    //             // - n 从4开始，解析到11（}后面）
    //             // - 设置 i = 11 - 1 = 10
    //             // - 循环结束，i++ → i=11
    //             // - 下一次循环从索引11开始（E的位置）

    //         }else if(format_status == Status::FORMAT){//出错了，格式参数未闭合（按理来说这一步应该不会出现，若出现则出了问题）
    //             std::cout<<"Error: patterns format error"<<std::endl;
    //             _has_error = true;
    //             patterns.emplace_back("<<pattern error>>",format_str, Type::NORMAL);
    //         }
            

    //     }
    //     if(!normal_str.empty()){//处理最后的普通字符
    //         patterns.push_back(std::make_tuple(normal_str, "", Type::NORMAL));
    //     }
    //     static std::unordered_map<std::string ,\
    //             std::function<FormatItem::Ptr(const std::string&)>> str_to_items = {
    //     #define XX(str, item) \
    //             {#str,[](const std::string &format){return std::make_shared<item>(format);}}

    //         XX(m, MesgFormatItem),           //m:消息
    //         XX(p, LevelFormatItem),             //p:日志级别
    //         XX(r, ElapseFormatItem),            //r:累计毫秒数
    //         XX(c, LoggerNameFormatItem),              //c:日志名称
    //         XX(t, ThreadIdFormatItem),          //t:线程id
    //         XX(n, NewLineFormatItem),           //n:换行
    //         XX(d, DateTimeFormatItem),          //d:时间
    //         XX(f, FilenameFormatItem),          //f:文件名
    //         XX(l, LineFormatItem),              //l:行号
    //         XX(T, TabFormatItem),               //T:Tab
    //         XX(F, FiberIdFormatItem),           //F:协程id
    //         XX(N, ThreadNameFormatItem),        //N:线程名称
    //     #undef XX
    //     };
    //     for(auto &pattern : patterns){
    //         if(std::get<2>(pattern) == Type::NORMAL){//如果是常规字符，则直接输出
    //             _format_items.push_back(std::make_shared<StringFormatItem>(std::get<0>(pattern)));
    //         }else if(std::get<2>(pattern) == Type::ESCAPE){//如果是转义字符，则根据名称创建对应的格式化项
    //             auto it = str_to_items.find(std::get<0>(pattern));
    //             if(it == str_to_items.end()){//如果没有找到对应的格式化项，则
    //                 _format_items.push_back(std::make_shared<StringFormatItem>("<<pattern error " + std::get<0>(pattern) + ">>"));
    //                 _has_error = true;
    //             }else{
    //                 _format_items.push_back(it->second(std::get<1>(pattern)));
    //             }
    //         }
    //     }

    // }

/********************************************************
 * Logger类实现
 * 
 **************/
    //参考
    // void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
    //     if(level >= m_level) {
    //         auto self = shared_from_this();
    //         MutexType::Lock lock(m_mutex);
    //         if(!m_appenders.empty()) {
    //             for(auto& i : m_appenders) {
    //                 i->log(self, level, event);
    //             }
    //         } else if(m_root) {
    //             m_root->log(level, event);
    //         }
    //     }
    // }
    void Logger::Log(Loglevel LV, LogEvent::Ptr event)//日志记录
    {
        event->_level = LV;
        if(LV >= _logger_level){
            auto logger_self = shared_from_this(); //获取智能指针
            MutexType::Lock lock(_mutex);//加锁，保证多线程安全
            if(!_appenders.empty()){
                for(auto &appender : _appenders){
                    // if(appender->GetLevel() < _logger_level){//如果appender的日志级别小于logger的日志级别，则设置为logger的日志级别
                    //     appender->SetLevel(_logger_level);
                    // }
                    appender->Log(logger_self,LV, event);
                }
            }else if(_root){//如果没有appender，则将日志传递给根日志器
                _root->Log(LV, event);
            }
        }
    }
    void Logger::SetFormatter(const std::string &pattern){
        auto new_val = std::make_shared<LogFormatter>(pattern);
        if(new_val->IsError()){
            std::cout<<"Logger setFormatter name="<<_name<<" value="<<pattern<<" invalid formatter"<<std::endl;
            return;
        }
        SetFormatter(new_val);
    }

    void Logger::AddAppender(LogAppender::Ptr appender){
        MutexType::Lock lock(_mutex);
        if(!appender->GetFormatter()){//如果没有设置格式化器，则设置默认格式化器
            appender->SetFormatter(_formatter);
            appender->SetHasFormatter(true);//设置格式化器标志位

        }
        _appenders.push_back(appender);
    }

    bool Logger::DelAppender(LogAppender::Ptr appender)
    {
        MutexType::Lock lock(_mutex);
        for(auto it : _appenders){
            if(it.get() == appender.get()){
                _appenders.remove(it);
                return true;
            }
        }
        return false;
    }
    
    void Logger::ClearAppenders()  {
        MutexType::Lock lock(_mutex);
        _appenders.clear();
    }
    std::string Logger::toYamlString() //转yaml格式字符串
    {
        YAML::Node node;

        MutexType::Lock lock(_mutex);    
        node["name"] = _name;
        if(_logger_level != Loglevel::UNKNOWN) {
            node["level"] = GetlevelStr(_logger_level);
        }
        if(_formatter) {
            node["formatter"] = _formatter->GetPattern();
        }
        for(auto& appender : _appenders) {
            node["appenders"].push_back(YAML::Load(appender->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    /*************************************************
     * ConsoleAppender类实现
     * 
     * FileAppender类实现
     * 
     ****************************/
    void LogAppender::SetFormatter(const std::string &pattern) //设置日志格式模板
    {
        MutexType::Lock lock(_mutex);
        auto new_val = std::make_shared<LogFormatter>(pattern);
        if(new_val->IsError()){
            std::cout<<"LogAppender setFormatter name="<<" value="<<pattern<<" invalid formatter"<<std::endl;
            return;
        }
        _formatter = new_val;
        _has_formatter = true;
    }


    void ConsoleLogAppender::Log(std::shared_ptr<Logger> logger , Loglevel LV, LogEvent::Ptr event)
    {
        if( !_has_formatter ) {
            std::cout<<"ConsoleLogAppender has no formatter"<<std::endl;
            return;
        }
        if(LV >= _append_level){
            MutexType::Lock lock(_mutex);
            _formatter->Format(std::cout, logger, LV, event);
        }
    }

    std::string ConsoleLogAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "ConsoleLogAppender";
        if(_append_level != Loglevel::UNKNOWN) {
            node["level"] = GetlevelStr(_append_level);
        }
        if(_has_formatter && _formatter) {
            node["formatter"] = _formatter->GetPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &filename){
        _filename = filename;
        Reopen();
    }
    void FileLogAppender::Log(std::shared_ptr<Logger> logger , Loglevel LV, LogEvent::Ptr event)
    {
        if(LV >= _append_level){
            uint64_t now = event->_time;
            if(now >= (_last_time + 3)){//每隔3秒重新打开一次文件（防止日志文件被意外删除然后文件流出错）
                Reopen();
                _last_time = now;
            }
            
            MutexType::Lock lock(_mutex);//保护 _ofs 与本 appender 的 formatter

            if(!_formatter){ std::cout<<"FileLogAppender missing formatter"<<std::endl; return; }
            if(!_formatter->Format(_ofs, logger, LV, event)){
                std::cout<<"error"<<std::endl;
            }
        }
    }
    bool FileLogAppender::Reopen()//重新打开文件
    {
        MutexType::Lock lock(_mutex);//保护 reopen 并发
        if(_ofs.is_open()){
            _ofs.close();
        }
        _ofs.open(_filename);
        return _ofs.is_open();
    }
    std::string FileLogAppender::toYamlString()
    {
        YAML::Node node;
        node["type"] = "FileLogAppender";
        if(_append_level != Loglevel::UNKNOWN) {
            node["level"] = GetlevelStr(_append_level);
        }
        if(_has_formatter && _formatter) {
            node["formatter"] = _formatter->GetPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

//Logmanager类实现

    LogManager::LogManager(){
        _root_logger.reset(new Logger("root"));
        _root_logger->AddAppender(std::make_shared<ConsoleLogAppender>());
        {
            MutexType::Lock lock(_mutex);
            _loggers[_root_logger->GetName()] = _root_logger;
        }
        Init();
    }
    void LogManager::Init(){}

    Logger::Ptr LogManager::GetLogger(const std::string &name)
    {
        MutexType::Lock lock(_mutex);

        auto it = _loggers.find(name);
        if(it != _loggers.end())
            return it->second;
        Logger::Ptr ret = std::make_shared<Logger>(name);
        ret->SetRoot(_root_logger);
        ret->AddAppender(std::make_shared<ConsoleLogAppender>());//默认添加控制台输出
        _loggers[name] = ret;
        return ret;
    }
    std::string LogManager::toYamlString() //转yaml格式字符串
    {
        YAML::Node node;
        
        {
            MutexType::Lock lock(_mutex);
            for(auto& i : _loggers) {
                node.push_back(YAML::Load(i.second->toYamlString()));
            }
        }

        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void LogManager::ShowLoggersName()
    {
        MutexType::Lock lock(_mutex);
        for(auto& i: _loggers){
            std::cout<<i.first<<std::endl;
        }
    }



//先约定一个全局的日志配置项
DYX::ConfigVar<std::set<LogDefine> >::Ptr g_log_defines = \
    DYX::ConfigManager::Lookup("logs", std::set<LogDefine>(), "logs config");

//基于观察者模式实现日志配置的热更新
LogIniter::LogIniter()
{
    auto func = [](const std::set<LogDefine> &oldvalue ,const std::set<LogDefine> &newvalue){
    // 对 new_value（最新的 LogDefine 集合）逐条检查：若原集合 old_value 中不存在同名配置，则视为新建\
    若存在但内容不完全相同，则视为修改；完全一致则跳过。
        STREAM_LOG_INFO(GET_ROOT())<<"on_logger_conf_changed";
        //遍历新值然后对比旧值
        // STREAM_LOG_INFO(GET_ROOT())<<"oldsize : "<<oldvalue.size()
        //                             <<", newsize : "<<newvalue.size();

        // for(auto &v:newvalue){
        //     STREAM_LOG_INFO(GET_ROOT())<<"new logdefine: "<<DYX::LexicalCast<LogDefine, std::string>()(v);
        // }
        // for(auto &v:oldvalue){
        //     STREAM_LOG_INFO(GET_ROOT())<<"old logdefine: "<<DYX::LexicalCast<LogDefine, std::string>()(v);
        // }
        for(auto &v:newvalue){
            auto it = oldvalue.find(v);//这里的find是用的重载的<运算符，只比较name
            Logger::Ptr logger;
            if(it == oldvalue.end()){//旧的没有，则新增logger

                logger = GET_NAME_LOGGER(v.name);
                logger->SetLevel(v.level);//设置日志级别
                if(!v.formatter.empty()){//设置格式化器
                    logger->SetFormatter(v.formatter);
                }
            }else{
                //旧的有，继续判断是否完全相同
                if(*it == v) continue;//相同则跳过
                else{
                    logger = GET_NAME_LOGGER(v.name);
                    STREAM_LOG_DEBUG(GET_ROOT())<<"logger name="<<v.name<<" changed.";  
                    logger->SetLevel(v.level);//更新日志级别
                    if(!v.formatter.empty()){//更新格式化器
                        logger->SetFormatter(v.formatter);
                    }
                }
            }
            //更新logger的appender
            logger->ClearAppenders();//清空默认添加的appender
            for(auto &a : v.appenders){//更新附加器
                LogAppender::Ptr ap;
                if(a.type == 1){//文件
                    ap.reset(new FileLogAppender(a.file));
                }else if(a.type == 2){//控制台
                    ap.reset(new ConsoleLogAppender());
                }
                if(ap){
                    // std::cout<<" appender level="<<GetlevelStr(a.level);
                    if(a.level != Loglevel::UNKNOWN) 
                        ap->SetLevel(a.level);
                    else{
                        ap->SetLevel(v.level);//使用logger的日志级别
                    }
                    if(!a.formatter.empty()){
                        // std::cout<<" appender formatter="<<a.formatter;
                        ap->SetFormatter(a.formatter);
                        ap->SetHasFormatter(true);
                    }else{
                        // std::cout<<" appender don't has formatter ";
                        if(!v.formatter.empty()){//使用logger的格式化器
                            ap->SetFormatter(v.formatter);
                        }
                    }
                    // std::cout<<"添加appender到logger: "<<v.name<<std::endl;
                    logger->AddAppender(ap);
                }
            }
        }
        //遍历旧值，删除已经不存在的logger(清除appender并将日志级别设置为UNKNOWN)
        for(auto &v:oldvalue){
            auto it = newvalue.find(v);
            if(it == newvalue.end()){//新的没有,则删除logger
                auto logger = GET_NAME_LOGGER(v.name);
                logger->SetLevel(Loglevel::UNKNOWN);
                logger->ClearAppenders();
            }
        }

    };
    g_log_defines->addChangeCallback(func);//注册回调函数(监听日志配置的变化)
    // std::cout<<"-------------------log init------------------------------"<<std::endl;

}
static LogIniter __log_init;//创建一个全局静态实例，保证程序启动时构造函数被执行，从而提前完成上述监听器的安装，实现日志配置的热更新。



}
