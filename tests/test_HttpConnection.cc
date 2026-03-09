#include "../src/Http/HttpConnection.h"
#include "../src/Log.h"
#include "../src/Http/HttpServer.h"
static DYX::Logger::Ptr g_logger = GET_ROOT();

void run_test() {
    STREAM_LOG_INFO(g_logger) << "========== 1. 单次短连接测试 ==========";
    // 使用 DoGet 访问百度主页，超时时间设置为 3000ms
    auto result = DYX::http::HttpConnection::DoGet("http://www.baidu.com/", 3000);
    if(result->result == 0 && result->response) {
        STREAM_LOG_INFO(g_logger) << "✅ DoGet Success! Response code: " 
                                 << (int)result->response->getStatus()
                                 << " Body size: " << result->response->getBody().size();
    } else {
        STREAM_LOG_ERROR(g_logger) << "❌ DoGet Failed! Error msg: " << result->error;
    }

    STREAM_LOG_INFO(g_logger) << "========== 2. 连接池性能/复用测试 ==========";
    // 创建一个通向百度的连接池，最大 5 个连接，空闲存活 30秒
    auto pool = DYX::http::HttpConnectionPool::Create("http://www.baidu.com", "", 5, 30000, 100);
    
    if(!pool) {
        STREAM_LOG_ERROR(g_logger) << "❌ Connection Pool create failed!";
        return;
    }

    // 模拟连续发出 3 个请求
    for(int i = 1; i <= 3; ++i) {
        auto r = pool->doGet("/", 3000);
        if(r->result == 0) {
            STREAM_LOG_INFO(g_logger) << "✅ Pool Request [" << i << "] Success! Response size: " 
                                     << r->response->getBody().size();
        } else {
            STREAM_LOG_ERROR(g_logger) << "❌ Pool Request [" << i << "] Failed!";
        }
    }
    
    STREAM_LOG_INFO(g_logger) << "测试结束，协程退出。";
}

int main(int argc, char** argv) {
    // 创建包含 2 个线程的 IO 调度器
    DYX::IOManager iom(2);
    // 把测试任务投递给协程调度
    iom.pushtask(run_test);
    // 主线程将在此阻塞等待协程执行完毕
    return 0;
}
