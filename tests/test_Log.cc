#include <time.h>
#include "../src/Log.h"

int main(){
    // DYX::Logger::Ptr logger = std::make_shared<DYX::Logger>();
    // logger->AddAppender(std::make_shared<DYX::ConsoleLogAppender>());
    // logger->AddAppender(std::make_shared<DYX::FileLogAppender>("./log.txt"));
    // logger->SetLevel(DYX::Loglevel::FATAL);
    // DYX::LogEvent::Ptr event = std::make_shared<DYX::LogEvent>(logger,DYX::Loglevel::DEBUG,__FILE__,__LINE__,0,1,2,time(0),"test");
    // event->_ss<<"Hello Log!";
    // logger->Log(DYX::Loglevel::FATAL,event);
    // STREAM_LOG_DEBUG(logger)<<"This is a debug!";
    // STREAM_LOG_INFO(logger)<<"This is a info!";
    // STREAM_LOG_WARN(logger)<<"This is a warrn!";
    // STREAM_LOG_ERROR(logger)<<"This is a error!";
    // STREAM_LOG_FATAL(logger)<<"This is a fatal!";

    // FORMAT_LOG_DEBUG(logger, "This is a debug! %d %s", 1, "test");
    // FORMAT_LOG_INFO(logger, "This is a info! %d %s", 2, "test");
    // FORMAT_LOG_WARN(logger, "This is a warrn! %d %s", 3, "test");
    // FORMAT_LOG_ERROR(logger, "This is a error! %d %s", 4, "test");
    // FORMAT_LOG_FATAL(logger, "This is a fatal! %d %s", 5, "test");
      
    STREAM_LOG_DEBUG(GET_ROOT())<<"This is a debug!";
    STREAM_LOG_INFO(GET_ROOT())<<"This is a info!";
    STREAM_LOG_WARN(GET_ROOT())<<"This is a warrn!";
    STREAM_LOG_ERROR(GET_ROOT())<<"This is a error!";
    STREAM_LOG_FATAL(GET_ROOT())<<"This is a fatal!";
    
    return 0;
}
