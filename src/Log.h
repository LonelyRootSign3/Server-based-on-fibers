#ifndef __LOG_H__
#define __LOG_H__

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <cstdarg>
#include <yaml-cpp/yaml.h>
#include "Util.h"
#include "Singleton.h"
#include "Mutex.h"
#include "Thread.h"
//使用流式方式将日志级别level的日志写入到logger
#define STREAM_LOG_LEVEL(logger, level) \
    if(logger->GetLevel() <= level) \
        DYX::LogEventWrapper(std::make_shared<DYX::LogEvent>(logger, level, __FILE__, __LINE__, \
        0 , DYX::GetThreadId(), DYX::GetFiberId(),  time(0),DYX::Thread::getCurName())).GetStream()

#define STREAM_LOG_DEBUG(logger) STREAM_LOG_LEVEL(logger, DYX::Loglevel::DEBUG)
#define STREAM_LOG_INFO(logger) STREAM_LOG_LEVEL(logger, DYX::Loglevel::INFO)
#define STREAM_LOG_WARN(logger) STREAM_LOG_LEVEL(logger, DYX::Loglevel::WARN)
#define STREAM_LOG_ERROR(logger) STREAM_LOG_LEVEL(logger, DYX::Loglevel::ERROR)
#define STREAM_LOG_FATAL(logger) STREAM_LOG_LEVEL(logger, DYX::Loglevel::FATAL)

// 使用格式化方式将日志级别level的日志写入到logger
#define FORMAT_LOG_LEVEL(logger, level, fmt ,...) \
        if(logger->GetLevel() <= level) \
            DYX::LogEventWrapper(std::make_shared<DYX::LogEvent>(logger, level, __FILE__, __LINE__, \
            0 , DYX::GetThreadId(), DYX::GetFiberId(),  \
            time(0), DYX::Thread::getCurName())).GetEvent()->format(fmt,__VA_ARGS__)

#define FORMAT_LOG_DEBUG(logger,fmt, ...) FORMAT_LOG_LEVEL(logger, DYX::Loglevel::DEBUG, fmt , __VA_ARGS__)
#define FORMAT_LOG_INFO(logger, fmt,...) FORMAT_LOG_LEVEL(logger, DYX::Loglevel::INFO,fmt , __VA_ARGS__)
#define FORMAT_LOG_WARN(logger,fmt ,...) FORMAT_LOG_LEVEL(logger, DYX::Loglevel::WARN,fmt , __VA_ARGS__)
#define FORMAT_LOG_ERROR(logger,fmt, ...) FORMAT_LOG_LEVEL(logger, DYX::Loglevel::ERROR,fmt , __VA_ARGS__)
#define FORMAT_LOG_FATAL(logger, fmt,...) FORMAT_LOG_LEVEL(logger, DYX::Loglevel::FATAL,fmt , __VA_ARGS__)



//获取主日志器
#define GET_ROOT() DYX::SingleLoggerManager::GetInstance()->GetRootLogger()

//获取指定的日志器
#define GET_NAME_LOGGER(name) DYX::SingleLoggerManager ::GetInstance()->GetLogger(name)

namespace DYX {
    //日志级别
    enum class Loglevel{
        DEBUG = 1,
        INFO,
        WARN,
        ERROR,
        FATAL,
        UNKNOWN
    };
    
    std::string GetlevelStr(Loglevel level);//日志级别转字符串
    Loglevel GetlevelInt(const std::string &levelStr);//字符串转日志级别

    class Logger;
    //日志事件
    class LogEvent{
    public:
        LogEvent(std::shared_ptr<Logger> logger, Loglevel level
            ,const char *file, int32_t line, uint64_t elapse
            ,uint32_t thread_id, uint32_t fiber_id, uint64_t time
            ,const std::string& thread_name)
        : _logger(logger)
        , _level(level)
        , _filename(file)
        , _line(line)
        , _elapse(elapse)
        , _thread_id(thread_id)
        , _fiber_id(fiber_id)
        , _time(time)
        , _thread_name(thread_name)
        {}
        using Ptr = std::shared_ptr<LogEvent>;
        //格式化写入日志内容
        void format(const char *fmt,...);
        void format(const char *fmt, va_list al);
    public:
        const char *_filename = nullptr;  //文件名
        uint64_t _time = 0;            //日志时间戳
        uint64_t _elapse = 0;        // 程序启动开始到现在的毫秒数
        uint32_t _line = 0;         //行号
        uint32_t _thread_id  = 0;   //线程ID
        std::string _thread_name; //线程名称
        uint32_t _fiber_id  = 0;    //协程ID

        std::shared_ptr<Logger> _logger; //日志器
        std::stringstream _ss;          //日志内容流
        Loglevel _level;                //日志级别
    };

    //日志包装器,用于在定义宏函数时自动析构然后输出日志事件
    class LogEventWrapper{
    public:
        LogEventWrapper(LogEvent::Ptr event) : _event(event) { /*std::cout<<"Wrapper"<<std::endl;*/}
        ~LogEventWrapper();

        //获取日志事件
        LogEvent::Ptr GetEvent() { return _event; }
        
        //获取日志内容流
        std::stringstream &GetStream() { return _event->_ss; }
    private:
        LogEvent::Ptr _event;
    };


    //日志格式器
    /*************************************
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
    //日志格式项
    
    class LogFormatter{
    public:
        LogFormatter(const std::string &pattern);
        using Ptr = std::shared_ptr<LogFormatter>;
        
        
        std::string Format(std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event);
        std::ostream &Format(std::ostream &os, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event);
        
        void Init();//初始化,解析日志模板
        std::string GetPattern() const { return _pattern; } //获取日志格式模板
        bool IsError() const { return _has_error; } //格式是否有错误
    public:
        class FormatItem{//日志内容项格式化基类
        public:
            using Ptr = std::shared_ptr<FormatItem>;
            FormatItem(){}
            virtual ~FormatItem() {}
            virtual void Format_Item(std::ostream &ofs, std::shared_ptr<Logger> logger , Loglevel level , LogEvent::Ptr event) = 0;
        };
    private:
        /// 日志格式模板
        std::string _pattern;
        /// 日志格式解析后的各项格式
        std::vector<FormatItem::Ptr> _format_items;
        /// 是否有错误
        bool _has_error = false;
    };

    //日志记录（输出）器
    class LogAppender{
    public:
        LogAppender(){}
        virtual ~LogAppender(){}//作为基类，析构函数需要虚函数
        using Ptr = std::shared_ptr<LogAppender>;
        using MutexType = DYX::SpinMutex;

        //日志输出,由子类实现
        virtual void Log(std::shared_ptr<Logger> logger , Loglevel LV, LogEvent::Ptr event) = 0;
        void SetFormatter(const std::string &pattern); //设置日志格式模板
        void SetFormatter(LogFormatter::Ptr formatter){ MutexType::Lock lock(_mutex); _formatter = formatter; } //设置日志格式器  
        LogFormatter::Ptr GetFormatter() const {  MutexType::Lock lock(_mutex) ;return _formatter; } //获取日志格式器  
        void SetHasFormatter(bool has_formatter) { _has_formatter = has_formatter; } //设置是否有自己的日志格式器  
        void SetLevel(Loglevel level) { _append_level = level; } //设置日志级别  
        Loglevel GetLevel() const { return _append_level; } //获取日志级别  

        virtual std::string toYamlString() = 0; //转yaml格式字符串

    protected:
        bool _has_formatter = false;//是否有自己的日志格式器
        Loglevel _append_level = Loglevel::DEBUG; //日志级别
        LogFormatter::Ptr _formatter = nullptr; //日志格式器

        mutable MutexType _mutex;      //互斥量 保证多线程下日志安全
    };

    //日志器
    class Logger : public std::enable_shared_from_this<Logger>{
    public:
       //m:消息
       //p:日志级别
       //r:累计毫秒数
       //c:日志名称
       //t:线程id
       //n:换行
       //d:时间
       //f:文件名
       //l:行号
       //T:Tab
       //F:协程id
       //N:线程名称
        Logger(const std::string &name = "root") 
            : _name(name) 
            , _formatter(std::make_shared<LogFormatter>("%d{[%Y-%m-%d %H:%M:%S]}%T[%t]%T[%N]%T[%F]%T[%p]%T[%c]%T[%f:%l]%T%m%n"))//默认日志格式 
            {}
        
        using Ptr = std::shared_ptr<Logger>;
        using MutexType = DYX::SpinMutex;

        void Log(Loglevel LV, LogEvent::Ptr event);//日志记录

        void deug(LogEvent::Ptr event) { Log(Loglevel::DEBUG, event);}
        void info(LogEvent::Ptr event) { Log(Loglevel::INFO, event);}
        void warn(LogEvent::Ptr event) { Log(Loglevel::WARN, event);}
        void error(LogEvent::Ptr event) { Log(Loglevel::ERROR, event);}
        void fatal(LogEvent::Ptr event) { Log(Loglevel::FATAL, event);}


        std::string GetName() const { return _name; }
        void SetLevel(Loglevel level) { _logger_level = level; }
        Loglevel GetLevel() const { return _logger_level; }

        void SetFormatter(LogFormatter::Ptr formatter){
            if(formatter->IsError()){
                std::cout<<"LogFormatter has error"<<std::endl;
                return;
            }
            MutexType::Lock lock(_mutex);
            _formatter = formatter; 
        }
        void SetFormatter(const std::string &pattern);
        LogFormatter::Ptr GetFormatter() const { return _formatter; }

        void AddAppender(LogAppender::Ptr appender);
        bool DelAppender(LogAppender::Ptr appender);
        void ClearAppenders();

        void SetRoot(Logger::Ptr root) { MutexType::Lock lock(_mutex);   _root = root; }
        Logger::Ptr GetRoot() const { return _root; }

        std::string toYamlString(); //转yaml格式字符串（调试用的）

        size_t AppendSize() const { return _appenders.size(); } //日志输出器数量

        
    private:
        Logger::Ptr _root;//根日志器
        LogFormatter::Ptr _formatter; //日志格式器
        std::string _name;//日志器名称
        Loglevel _logger_level = Loglevel::DEBUG; //日志级别
        std::list<LogAppender::Ptr> _appenders;//日志输出器列表

        mutable MutexType _mutex;      //互斥量 保证多线程下日志安全
    };

    //控制台日志输出器(标准输出)
    class ConsoleLogAppender : public LogAppender{
    public: 
        using Ptr = std::shared_ptr<ConsoleLogAppender>;
        void Log(std::shared_ptr<Logger> logger , Loglevel LV, LogEvent::Ptr event) override;
        std::string toYamlString() override;
    private:
        
    };

    //文件日志输出器
    class FileLogAppender : public LogAppender{
    public:
        using Ptr = std::shared_ptr<FileLogAppender>;
        FileLogAppender(const std::string &filename);
        void Log(std::shared_ptr<Logger> logger , Loglevel LV, LogEvent::Ptr event) override;
        std::string toYamlString() override;
        bool Reopen();//重新打开文件
    private:
        std::string _filename; //日志文件名
        std::ofstream _ofs; //日志文件输出流(写文件)
        uint64_t _last_time; //上一次打开时间
    };

    //日志管理器
    class LogManager{
    public:
        LogManager();
        using MutexType = DYX::SpinMutex;

        void Init();
        Logger::Ptr GetLogger(const std::string &name);
        Logger::Ptr GetRootLogger() {return _root_logger;}
        std::string toYamlString(); //转yaml格式字符串（调试用的）
        void ShowLoggersName();
    private:
        Logger::Ptr _root_logger; //根日志器
        std::unordered_map<std::string, Logger::Ptr> _loggers; //日志器列表
        
        MutexType _mutex; 
    };

    //日志管理器单例
    using SingleLoggerManager = DYX::SingletonPtr<LogManager>;
    // typedef SingletonPtr<LogManager> LoggerManager;
}

#endif
