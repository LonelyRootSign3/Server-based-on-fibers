#include "../src/Http/HttpServer.h"
#include "../src/Log.h"
#include "../Logdefine.h"
static DYX::Logger::Ptr g_logger = GET_ROOT();

void Run(){
    g_logger->SetLevel(DYX::Loglevel::INFO);
    STREAM_LOG_INFO(g_logger)<<"=====================in the run==========================";
    DYX::http::HttpServer::Ptr server = std::make_shared<DYX::http::HttpServer>();
    auto addr = DYX::Address::LookupAnyIPAddress("0.0.0.0:8080");
    
    bool ret = server->bind(addr);
    if(!ret) {
        STREAM_LOG_ERROR(g_logger)<<"bind "<<addr->toString()<<" fail";
    }
    server->start();
}

int main(int argc ,char *argv[]){
    DYX::g_log_defines->definethis();
    DYX::IOManager iom(2);
    iom.pushtask(Run);
    return 0;
}

