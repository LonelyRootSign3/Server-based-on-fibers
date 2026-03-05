#include "../src/Socket.h"
#include "../src/Log.h"
#include "../src/IOManager.h"
DYX::Logger::Ptr g_logger = GET_ROOT();


void test_Socket(){
    DYX::IPAddress::Ptr addr = DYX::Address::LookupAnyIPAddress("www.baidu.com");
    
    if(addr){
        STREAM_LOG_INFO(g_logger)<<"get address: "<<addr->toString();
    }else{
        STREAM_LOG_ERROR(g_logger)<<"get address fail"<<addr->toString();
        return;
    }
    
    addr->setPort(80);
    STREAM_LOG_INFO(g_logger)<<"addr = "<<addr->toString();
    DYX::Socket::Ptr sock = DYX::Socket::CreateTCP(addr);
    if(!sock->connect(addr)){
        STREAM_LOG_ERROR(g_logger)<<"connect error";
        return;
    }else{
        STREAM_LOG_INFO(g_logger)<<"connect "<<addr->toString()<<" connected";
    }
    
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff,sizeof(buff));
    if(rt<=0){
        STREAM_LOG_ERROR(g_logger)<<" send error";
        return;
    }else{
        std::string buffers;
        buffers.resize(4096);
        rt = sock->recv(&buffers[0],buffers.size());
        if(rt <= 0){
            STREAM_LOG_ERROR(g_logger)<<"recv fail rt = "<<rt;
            return;
        }
        buffers.resize(rt);
        STREAM_LOG_INFO(g_logger)<<buffers;
    }
    
}


//基准测试（benchmark），评估框架中Socket错误检查操作的性能开销
void test2() {
    DYX::IPAddress::Ptr addr = DYX::Address::LookupAnyIPAddress("www.baidu.com");
    addr->setPort(80);
    if(addr) {
        STREAM_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        STREAM_LOG_ERROR(g_logger) << "get address fail";
        return;
    }

    DYX::Socket::Ptr sock = DYX::Socket::CreateTCP(addr);
    if(!sock->connect(addr)) {
        STREAM_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        STREAM_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    uint64_t ts = DYX::GetCurrentUS();
    for(size_t i = 0; i < 10000000000ul; ++i) {
        if(int err = sock->getErrorno()) {
            STREAM_LOG_INFO(g_logger) << "err=" << err << " errstr=" << strerror(err);
            break;
        }

        static int batch = 10000000;
        if(i && (i % batch) == 0) {
            uint64_t ts2 = DYX::GetCurrentUS();
            STREAM_LOG_INFO(g_logger) << "i=" << i << " used: " << ((ts2 - ts) * 1.0 / batch) << " us";
            ts = ts2;
        }
    }
}


int main(int argc, char const *argv[]){
    DYX::IOManager iom;
    //iom.pushtask(test_Socket);
    iom.pushtask(test2);
    return 0;
}

