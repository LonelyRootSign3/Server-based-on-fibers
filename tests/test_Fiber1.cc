#include "../src/Fiber1.h"
#include "../src/Log.h"
#include "../src/Thread.h"

static DYX::Logger::Ptr g_logger = GET_ROOT();
static thread_local int s_count = 0;

void test_fiber(){
    s_count++;
    if(s_count == 3){
        return;
    }
    STREAM_LOG_DEBUG(g_logger)<<"in the test_fiber "<<s_count;
    DYX::Fiber::YieldToHold();
}
 

void thread_func(){
    DYX::Fiber::Ptr sc_fiber = DYX::Fiber::BuildScheduleFiber();
    DYX::Fiber::SetScheduleFiber(sc_fiber.get());
    DYX::Fiber::SetCurFiber(sc_fiber.get());
    STREAM_LOG_DEBUG(g_logger)<<"in the thread_func "<<sc_fiber->GetId();
    DYX::Fiber sub_fiber(test_fiber);
    sub_fiber.SwapIn();
    STREAM_LOG_DEBUG(g_logger)<<"back to main fiber 1";
    sub_fiber.Reset(test_fiber);
    sub_fiber.SwapIn();
    STREAM_LOG_DEBUG(g_logger)<<"back to main fiber 2";
    sub_fiber.Reset(test_fiber);
    sub_fiber.SwapIn();
    STREAM_LOG_DEBUG(g_logger)<<"back to main fiber 3";

}

int main(){
    std::vector<DYX::Thread::Ptr> thrs;
    for(int i = 0;i<1;i++){
        auto thrptr = std::make_shared<DYX::Thread>(thread_func,"thread"+std::to_string(i));
        thrs.push_back(thrptr);
    }
    for(auto &thr:thrs){
        thr->join();
    }

    STREAM_LOG_DEBUG(g_logger)<<"total fibers "<<DYX::Fiber::TotalFibers();
    return 0;
}