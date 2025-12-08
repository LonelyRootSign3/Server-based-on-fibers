#include "../src/IOManager.h"
#include "../src/Log.h"
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>

int sock;
static DYX::Logger::Ptr g_logger = GET_ROOT();
void test_fiber(){
    STREAM_LOG_INFO(g_logger)<<"test_fiber";
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "115.239.210.27", &addr.sin_addr.s_addr);


    if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
    } else if(errno == EINPROGRESS) {
        STREAM_LOG_DEBUG(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        DYX::IOManager::GetThis()->addEvent(sock, DYX::IOManager::READ, [](){
            STREAM_LOG_INFO(g_logger) << "read callback";
        });
        DYX::IOManager::GetThis()->addEvent(sock, DYX::IOManager::WRITE, [](){
            STREAM_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            DYX::IOManager::GetThis()->cancelEvent(sock, DYX::IOManager::READ);
            close(sock);
        });
    } else {
        STREAM_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test(){
    DYX::IOManager iom(2, false);
    iom.pushtask(test_fiber);
}

DYX::Timer::Ptr s_timer;

void test_timer(){
    DYX::IOManager iom(2, false);
    // iom.addTimer(3000, [](){
    //     STREAM_LOG_INFO(g_logger) << "hello timer";
    // }, true);
    s_timer = iom.addTimer(3000,[](){
        static int i = 0;
        STREAM_LOG_INFO(g_logger) << "hello timer" << i++;
        if(i == 3){
            s_timer->reset(2000, true);
            // s_timer->cancel();
        }
    },true);

}

int main(){
    STREAM_LOG_INFO(g_logger) << "main";
    // test();
    test_timer();
    return 0;
}

