#include "../src/Schedule1.h"
#include "../src/Log.h"


static DYX::Logger::Ptr g_logger = GET_ROOT();

void test_fiber(){
    static int count = 5;
    STREAM_LOG_INFO(g_logger)<<"test in fiber";
    sleep(1);
    if(count-- > 0){
        DYX::Scheduler::GetThis()->pushtask(test_fiber);
    }

}


int main(){

    STREAM_LOG_INFO(g_logger)<<"main thread begin ";
    DYX::Scheduler sc(3,false,"test");
    std::cout<<"----------------------start--------------------------"<<std::endl;
    sc.Start();
    sleep(1);
    std::cout<<"---------------------push_task-------------------------"<<std::endl;
    STREAM_LOG_INFO(g_logger)<<"push task begin";
    sc.pushtask(test_fiber);
    // sc.pushtask(test_fiber);
    std::cout<<"-----------------------stop-----------------------------"<<std::endl;
    sc.Stop();
    STREAM_LOG_INFO(g_logger)<<"over";
    // while(1){ sleep (1);}

    return 0;
}
