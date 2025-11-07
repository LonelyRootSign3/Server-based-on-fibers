#include "../src/Util.h"
#include "../src/Macro.h"
DYX::Logger::Ptr g_logger = GET_NAME_LOGGER("system");

void test_backtrace(){
    STREAM_LOG_INFO(g_logger) << "test backtrace begin";
    STREAM_LOG_INFO(g_logger) << DYX::BacktraceToString(10, 0, "    ");
    STREAM_LOG_INFO(g_logger) << "test backtrace end";
}

void test_assert(){
    MY_ASSERT2(1 == 1, "判断错误");
}
int main(int argc,char **argv){
    // test_backtrace(); 
    // test_assert();
    return 0;
}