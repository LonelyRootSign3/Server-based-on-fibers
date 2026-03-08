#include "../src/Http/Http.h"

void test_req(){
    DYX::http::HttpRequest::Ptr req = std::make_shared<DYX::http::HttpRequest>();
    req->setHeader("Host","www.baidu.com");
    req->setBody("hello world");
    req->dump(std::cout)<<std::endl;
}
void test_resp(){
    DYX::http::HttpResponse::Ptr resp = std::make_shared<DYX::http::HttpResponse>();
    resp->setHeader("b-b","baidu");
    resp->setBody("send to ...");
    resp->setStatus(DYX::http::http_status::HTTP_STATUS_OK);
    resp->setClose(false);
    resp->dump(std::cout)<<std::endl;
}

int main(){
    test_req();
    test_resp();
    return 0;
}
