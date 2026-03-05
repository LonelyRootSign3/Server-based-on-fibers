#include "../src/Tcp_server.h"
#include "../src/Log.h"
#include <vector>
static DYX::Logger::Ptr g_logger = GET_ROOT();


void test(){
    DYX::IOManager::Ptr iom = std::make_shared<DYX::IOManager>(2);
    DYX::TcpServer::Ptr tcp_server = std::make_shared<DYX::TcpServer>(iom.get());
    DYX::Address::Ptr addr1 = DYX::Address::LookupAny("0.0.0.0:8080");
    // DYX::Address::Ptr addr2 = DYX::Address::Ptr(new DYX::UnixAddress("/tmp/unix.sock"));
    std::vector<DYX::Address::Ptr> addrs;
    std::vector<DYX::Address::Ptr> fails;
    addrs.push_back(addr1);
    // addrs.push_back(addr2);


    STREAM_LOG_INFO(g_logger)<<"==============================test bind==================================";

    bool ret = tcp_server->bind(addrs, fails);//将tcp服务器里的所有监听套接字都绑定地址
    if(ret)
        tcp_server->start();
    else{
        sleep(2);
    }
}

void run(){
    DYX::Address::Ptr addr = DYX::Address::LookupAny("0.0.0.0:8080");
    DYX::TcpServer::Ptr tcpser = std::make_shared<DYX::TcpServer>();
    if(tcpser->bind(addr)){
        tcpser->start();
    }

}


int main(int argc ,char **argv){
    // DYX::IOManager iom(2);
    // iom.pushtask(run);
    test();
    return 0;
}

