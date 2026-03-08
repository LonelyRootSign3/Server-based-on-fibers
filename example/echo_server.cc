#include "../src/Tcp_server.h"
#include "../src/Bytearray.h"
#include "../src/Log.h"
static DYX::Logger::Ptr g_logger = GET_ROOT();
class EchoServer : public DYX::TcpServer{
public:
    EchoServer(int type);
    void handleClient(DYX::Socket::Ptr client);
private:
    int m_type = 0;
};

EchoServer::EchoServer(int type) : m_type(type){}

void EchoServer::handleClient(DYX::Socket::Ptr client){
    STREAM_LOG_DEBUG(g_logger)<<"welcome client "<<client->toString();
    DYX::ByteArray::Ptr ba = std::make_shared<DYX::ByteArray>();
    while(true){
        ba->clear();
        std::vector<iovec> iovs;//连接内核和用户的桥梁
        ba->getWriteBuffers(iovs,1024);//不会改变m_postion，只是将用户态的Node结点地址都交给了iovc,然后使用系统调用recv将数据拷贝到用户态的Node结点中
        int rt = client->recv(&iovs[0],iovs.size());//
        if(rt == 0){
            STREAM_LOG_DEBUG(g_logger)<<"client "<<client->toString()<<" close";
            break;
        }else if(rt < 0){
            STREAM_LOG_ERROR(g_logger)<<"client "<<client->toString()<<" recv error";
            break;
        }
        ba->setWritePosition(ba->getWritePosition() + rt);//将写位置移动到有效数据的末尾
        if(m_type == 1) {//text 
            std::cout << ba->toString();// << std::endl;
        } else {
            std::cout << ba->toHexString();// << std::endl;
        }
        std::cout.flush();
    }  
      
}
int type = 1;//以文本形式

void run() {
    STREAM_LOG_INFO(g_logger) << "server type=" << type;
    EchoServer::Ptr es(new EchoServer(type));
    auto addr = DYX::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}
int main(int argc ,char *argv[]){

    if(argc < 2){
        STREAM_LOG_INFO(g_logger) << "use as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    if(strcmp(argv[1],"-t")){
        type = 2;//以二进制
    }
    DYX::IOManager iom(2);
    iom.pushtask(run);
    return 0;
}