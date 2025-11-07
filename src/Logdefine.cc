#include "Logdefine.h"

namespace DYX{
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
