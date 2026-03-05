#include "../src/Address.h"
#include "../src/Log.h"
#include <map>
#include <string>
DYX::Logger::Ptr g_logger = GET_ROOT();

void test_ipv4(){
    auto addr = DYX::IPAddress::Create("127.0.0.1");
    if(addr){
        STREAM_LOG_DEBUG(g_logger)<< addr->toString();
        return;
    }
    STREAM_LOG_ERROR(g_logger)<<"error";
}



void test_iface(){
    std::multimap<std::string, std::pair<DYX::Address::Ptr, uint32_t> > results;
    bool v = DYX::Address::GetInterfaceAddresses(results,AF_INET);
    v = DYX::Address::GetInterfaceAddresses(results,AF_INET6);
    if(!v) {
        STREAM_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }

    for(auto& i: results) {
        STREAM_LOG_INFO(g_logger) <<" 网卡名 ："<<i.first << " 网卡地址：" << i.second.first->toString() <<"/"
            << i.second.second;// 子网掩码
    }
}


void test(){
    std::vector<DYX::Address::Ptr> addrs;

    STREAM_LOG_DEBUG(g_logger) << "begin";
    bool v = DYX::Address::Lookup(addrs, "localhost:3080");
    v = DYX::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    if(!v){
        STREAM_LOG_ERROR(g_logger)<<"look up fail";
        return;
    }
    
    for(size_t i = 0; i < addrs.size();i++){
        STREAM_LOG_INFO(g_logger)<<i<<"-"<<addrs[i]->toString();
    }
    
    auto addr = DYX::Address::LookupAny("localhost:4080");
    if(addr){
        STREAM_LOG_INFO(g_logger)<< *addr;

    }else{
        STREAM_LOG_ERROR(g_logger)<<"error";
    }

}
int main(){
    //test_ipv4();
    
    test_iface();
    //test();
    return 0;
}
