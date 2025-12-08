#include "../src/Hook.h"
#include "../src/Log.h"
#include "../src/IOManager.h"
static DYX::Logger::Ptr g_logger = GET_ROOT();  

void test_sleep(){
    DYX::IOManager iom(1,true);
    iom.pushtask([](){
        STREAM_LOG_DEBUG(g_logger)<<"cb fiber run the task 1";
        sleep(2);
        STREAM_LOG_DEBUG(g_logger) << "sleep 2s";
    });
    iom.pushtask([](){
        STREAM_LOG_DEBUG(g_logger)<<"cb fiber run the task 2";
        sleep(3);
        STREAM_LOG_DEBUG(g_logger) << "sleep 3s";
    });
    STREAM_LOG_DEBUG(g_logger) << "test sleep";
}




int main(){
    STREAM_LOG_DEBUG(g_logger)<<"begin test";


    test_sleep();


    STREAM_LOG_DEBUG(g_logger)<<"end test";
    return 0;
}
