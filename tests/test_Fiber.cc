#include "../src/Fiber.h"
#include "../src/Log.h"
#include "../src/Thread.h"
inline DYX::Logger::Ptr g_logger = GET_ROOT();

void run_in_fiber(){
    STREAM_LOG_INFO(g_logger)<<"run in subfiber begin";
    DYX::Fiber::YieldToHold();
    STREAM_LOG_INFO(g_logger)<<"run in subfiber after hold ";

}
void testfiber(){
    DYX::Fiber::Ptr main_fiber = DYX::Fiber::GetThis();
    STREAM_LOG_DEBUG(g_logger)<<"main Ptr num : "<<main_fiber.use_count();
    STREAM_LOG_INFO(g_logger)<<"main begin";
    auto sub_fiber = std::make_shared<DYX::Fiber>(run_in_fiber);
    sub_fiber->swapIn();//子协程切入
    STREAM_LOG_INFO(g_logger)<<"main after swapin 1 ";
    sub_fiber->swapIn();
    STREAM_LOG_INFO(g_logger)<<"main after swapin 2 ";

}
int main(){

    STREAM_LOG_INFO(g_logger)<<"test begin";
    std::vector<DYX::Thread::Ptr> thrs;
    for(int i = 0;i<3;i++){
        auto thrptr = std::make_shared<DYX::Thread>(testfiber,"thread"+std::to_string(i));
        thrs.push_back(thrptr);
    }
    for(int i = 0;i<3;i++){
        thrs[i]->join();
    }

    STREAM_LOG_INFO(g_logger)<<"test end";
    STREAM_LOG_INFO(g_logger)<<"fiber num : "<<DYX::Fiber::TotalFibers();
    return 0;
}
