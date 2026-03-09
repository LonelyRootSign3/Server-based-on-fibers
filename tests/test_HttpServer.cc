#include "../src/Http/HttpServer.h"
#include "../src/Log.h"
#include "../Logdefine.h"
static DYX::Logger::Ptr g_logger = GET_ROOT();
#define XX(...) #__VA_ARGS__

void Run(){
    g_logger->SetLevel(DYX::Loglevel::INFO);
    STREAM_LOG_INFO(g_logger)<<"=====================in the run==========================";
    
    DYX::http::HttpServer::Ptr server = std::make_shared<DYX::http::HttpServer>();
    auto addr = DYX::Address::LookupAnyIPAddress("0.0.0.0:8080");
    bool ret = server->bind(addr);
    if(!ret) {
        STREAM_LOG_ERROR(g_logger)<<"bind "<<addr->toString()<<" fail";
    }
    auto sd = server->getDispatcher();
    sd->addServlet("/DYX/xx", [](DYX::http::HttpRequest::Ptr req
                ,DYX::http::HttpResponse::Ptr rsp
                ,DYX::http::HttpSession::Ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/DYX/*", [](DYX::http::HttpRequest::Ptr req
                ,DYX::http::HttpResponse::Ptr rsp
                ,DYX::http::HttpSession::Ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });
    sd->addGlobServlet("/DYXx/*", [](DYX::http::HttpRequest::Ptr req
                ,DYX::http::HttpResponse::Ptr rsp
                ,DYX::http::HttpSession::Ptr session) {
            rsp->setBody(XX(<html>
            <head><title>404 Not Found</title></head>
            <body>
            <center><h1>404 Not Found</h1></center>
            <hr><center>nginx/1.16.0</center>
            </body>
            </html>
            <!-- a padding to disable MSIE and Chrome friendly error page -->
            <!-- a padding to disable MSIE and Chrome friendly error page -->
            <!-- a padding to disable MSIE and Chrome friendly error page -->
            <!-- a padding to disable MSIE and Chrome friendly error page -->
            <!-- a padding to disable MSIE and Chrome friendly error page -->
            <!-- a padding to disable MSIE and Chrome friendly error page -->));
            return 0;
        });
    server->start();

}

int main(int argc ,char *argv[]){
    try {
        // DYX::g_log_defines->definethis();
        YAML::Node root = YAML::LoadFile("/home/dyx/HardProject/configs/log.yaml"); 
        // 2. 将配置注入到 ConfigManager，这会触发全局日志配置的更新回调
        DYX::ConfigManager::LoadFromYaml(root);
    } catch (const std::exception& e) {
        std::cerr << "Load log.yaml failed: " << e.what() << std::endl;
        return -1;
    }

    DYX::IOManager iom(2);
    iom.pushtask(Run);
    return 0;
}

