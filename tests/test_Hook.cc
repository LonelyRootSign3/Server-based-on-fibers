#include "../src/Hook.h"
#include "../src/Log.h"
#include "../src/IOManager.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
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

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);

    STREAM_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(sockaddr_in));
    STREAM_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    STREAM_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    STREAM_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;


    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    STREAM_LOG_INFO(g_logger) << buff;


}



int main(){
    STREAM_LOG_DEBUG(g_logger)<<"begin test";
    // test_sleep();

    DYX::IOManager iom;
    iom.pushtask(test_sock);
    STREAM_LOG_DEBUG(g_logger)<<"end test";


    return 0;
}
