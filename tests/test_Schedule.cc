#include "../src/Schedule.h"
#include "../src/Log.h"

static DYX::Logger::Ptr g_logger = GET_ROOT();

void test_fiber(){
    STREAM_LOG_INFO(g_logger)<<"test in fiber";
}


int main(){
    STREAM_LOG_INFO(g_logger)<<"main thread begin ";
    DYX::Scheduler sc(2,true,"test");

    std::cout<<"----------------------start--------------------------"<<std::endl;
    sc.start();
    sleep(1);
    std::cout<<"---------------------push_task-------------------------"<<std::endl;
    STREAM_LOG_INFO(g_logger)<<"schedule begin";
    sc.pushtask(test_fiber);

    std::cout<<"-----------------------stop-----------------------------"<<std::endl;
    sc.stop();

    STREAM_LOG_INFO(g_logger)<<"over";
    return 0;
}

