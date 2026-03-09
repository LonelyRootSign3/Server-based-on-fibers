#pragma once
#include "Log.h"
#include "Config.h"
namespace DYX{

//-----------------Log日志和Config配置模块的整合-------------------------------------

struct LogAppenderDefine {
    int type = 0;   //1 File, 2 Console
    Loglevel level = Loglevel::UNKNOWN;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const {
        return type == oth.type
            && level == oth.level
            && formatter == oth.formatter
            && file == oth.file;
    }
};

struct LogDefine {
    std::string name;
    Loglevel level = Loglevel::UNKNOWN;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const{
        return name == oth.name
            && level == oth.level
            && formatter == oth.formatter
            && appenders == oth.appenders;
    }

    bool operator<(const LogDefine& oth) const {
        return name < oth.name;
    }

    bool isValid() const {
        return !name.empty();
    }
};

//string转LogDefine
template<>
class LexicalCast<std::string, LogDefine> {
public:
    LogDefine operator()(const std::string &v){
        YAML::Node node = YAML::Load(v);
        LogDefine ret;//返回值
        if(!node["name"].IsDefined()){
            std::cout<<"Log config error: name is null, "<<node<<std::endl;
            throw std::logic_error("Log config name is null");
        }
        ret.name = node["name"].as<std::string>();//日志名称

        ret.level = GetlevelInt(node["level"].IsDefined() ? node["level"].as<std::string>() : "UNKNOWN");//日志级别
        
        
        if(node["formatter"].IsDefined()){//格式化器,可选
            ret.formatter = node["formatter"].as<std::string>();
        }

        if(!node["appenders"].IsDefined()){//没有appender
            std::cout<<"Log config name="<<ret.name<<" appenders is null"<<std::endl;
            throw std::logic_error("Log config appenders is null");
        }else{
            for(size_t i = 0;i<node["appenders"].size();++i){
                auto a = node["appenders"][i];
                if(!a["type"].IsDefined()){
                    std::cout<<"Log config error: appender type is null, "<<a<<std::endl;
                    continue;
                }
                LogAppenderDefine lad;
                std::string type = a["type"].as<std::string>();
                if(type == "FileLogAppender"){
                    lad.type = 1;
                    if(!a["file"].IsDefined()){
                        std::cout<<"Log config error: fileappender file is null, "<<a<<std::endl;
                        continue;
                    }
                    lad.file = a["file"].as<std::string>();
                    if(lad.file.empty()){
                        std::cout<<"Log config error: fileappender file is empty, "<<a<<std::endl;
                        continue;
                    }
                }else if(type == "ConsoleLogAppender"){
                    lad.type = 2;
                }else{
                    std::cout<<"Log config error: appender type is invalid, "<<a<<std::endl;
                    continue;
                }
                lad.level = GetlevelInt(a["level"].IsDefined() ? a["level"].as<std::string>() : "UNKNOWN");
                if(a["formatter"].IsDefined()){
                    lad.formatter = a["formatter"].as<std::string>();
                }
                ret.appenders.push_back(lad);
            }
        }
        return ret;
    }
};


//LogDefine转string
template<>
class LexicalCast<LogDefine, std::string> {
public:
    std::string operator()(const LogDefine &log){
        YAML::Node node;
        node["name"] = log.name;
        if(log.level != Loglevel::UNKNOWN){
            node["level"] = GetlevelStr(log.level);
            // std::cout<<"log level is "<< GetlevelStr(log.level)<<std::endl;
            STREAM_LOG_DEBUG(GET_ROOT())<<"log level is "<< GetlevelStr(log.level);
        }else{
            STREAM_LOG_DEBUG(GET_ROOT())<<"log level is "<< GetlevelStr(log.level);
        }
        if(!log.formatter.empty()){
            node["formatter"] = log.formatter;
        }
        for(auto &a : log.appenders){
            YAML::Node na;
            if(a.type == 1){
                na["type"] = "FileLogAppender";
                na["file"] = a.file;
            }else if(a.type == 2){
                na["type"] = "ConsoleLogAppender";
            }else{
                continue;
            }
            if(a.level != Loglevel::UNKNOWN){
                na["level"] = GetlevelStr(a.level);
            }
            if(!a.formatter.empty()){
                na["formatter"] = a.formatter;
            }
            node["appenders"].push_back(na);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

//先约定一个全局的日志配置项
extern ConfigVar<std::set<LogDefine>>::Ptr g_log_defines;  // 只声明，不定义  
//不能在头文件定义，每个包含这个头文件的 .cc文件都会各自生成一个独立的全局变量拷贝。\
//这样做是为了让所有模块共享同一变量

    
// 一个结构体，其构造函数通过监听日志配置的变化，\
动态更新日志器的属性（如日志级别、格式化器和附加器）。\
它支持新增、修改和删除日志器，并根据配置动态调整日志输出行为。

//基于观察者模式实现日志配置的热更新
struct LogIniter{
    LogIniter();
};



}