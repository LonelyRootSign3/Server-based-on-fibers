#include "Http.h"
#include "../Util.h"
#include <strings.h>
#include <cstring>
namespace DYX{

namespace http{

bool CompareIngnoreCase::operator()(const std::string &lhs ,const std::string &rhs) const{
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

http_method StringToHttpMethod(const std::string& m){
#define XX(num,name,string) \
    if(strcmp(#string,m.c_str()) == 0) {return http_method::HTTP_##name;}
    HTTP_METHOD_MAP(XX);
#undef XX       
    return http_method::INVALID_METHOD;
}
http_method CharsToHttpMethod(const char* m){
#define XX(num,name,string) \
    if(strncmp(#string,m,strlen(#string)) == 0) {return http_method::HTTP_##name;}
    HTTP_METHOD_MAP(XX);
#undef XX       
    return http_method::INVALID_METHOD;
}

static const char *s_method_string[] = {
    #define XX(num,name,string) #string,
        HTTP_METHOD_MAP(XX)
    #undef XX
};

const char *HttpMethodToString(const http_method& m){
    if(m > http_method::INVALID_METHOD) { return "<unknown>"; }
    return s_method_string[(uint32_t)m];
}

const char *HttpStatusToString(const http_status& s){
    switch(s){
        #define XX(num,name,string) \
            case http_status::HTTP_STATUS_##name: return #string;
        HTTP_STATUS_MAP(XX)
        #undef XX
    default: return "<unknown>";
    }
}

HttpRequest::HttpRequest(uint8_t version,bool close)
        :m_version(version)
        ,m_close(close)
        ,m_path("/")
        ,m_websocket(false)
        ,m_parserParamFlag(0x00)
        ,m_method(http_method::HTTP_GET){}

std::shared_ptr<HttpResponse> HttpRequest::createHttpResponse(){
    HttpResponse::Ptr rsp(new HttpResponse(getVersion()
                            ,isClose()));
    return rsp;
}


void HttpRequest::init(){
    std::string con = getHeader("Connection");
    if(con == "keep-alive") m_close = false;
    else m_close = true;
}

// 使用模板以支持不同的 Map 类型 (std::map, std::unordered_map) 
// 和不同的 Callable 类型 (函数指针, lambda 表达式等)
template <typename MapType, typename TrimFunc>
inline void ParseParam(const std::string& str, MapType& m, char flag, TrimFunc trim) {
    if (str.empty()) return;

    size_t start = 0;
    const size_t len = str.length();

    while (start < len) {
        // 1. 找当前键值对的边界 (按 flag 分割)
        size_t end = str.find(flag, start);
        if (end == std::string::npos) {
            end = len; // 如果没找到分隔符，说明这是最后一个参数
        }

        size_t pair_len = end - start;
        if (pair_len > 0) { // 忽略连续的分隔符 (比如 &&)
            // 2. 在当前键值对内部找等号 '='
            size_t eq_pos = str.find('=', start);

            // 3. 逻辑分流：区分有值 Key 和无值 Key
            if (eq_pos != std::string::npos && eq_pos < end) {
                // 场景 A: 标准键值对 (例如 "name=Gemini")
                std::string key = str.substr(start, eq_pos - start);
                std::string val = str.substr(eq_pos + 1, end - eq_pos - 1);
                
                // 注意：这里调用了我们上一步完善的 UrlDecode，并假设默认开启空格转加号
                m.insert(std::make_pair(trim(key,"\t\r\n"), UrlDecode(val, true))); //这里trim即使第二个参数为默认参数也要显示写，因为模板这东西不知道
            } else {
                // 场景 B: 无值 Key (例如 "debug")
                std::string key = str.substr(start, pair_len);
                // 无值 Key 默认插入空字符串 ""
                m.insert(std::make_pair(trim(key,"\t\r\n"), "")); 
            }
        }

        // 4. 步进到下一个键值对的起点
        start = end + 1;
    }
}



void HttpRequest::initQueryParam(){
    if(m_parserParamFlag & 0x01) return;
    ParseParam(m_query, m_params, '&', Trim);
    m_parserParamFlag |= 0x01;
}
void HttpRequest::initBodyParam(){
    if(m_parserParamFlag & 0x02) return;
    std::string content_type = getHeader("content-type");
    if(strcasestr(content_type.c_str(), "application/x-www-form-urlencoded") == nullptr) {//是否是标准的网页表单数据
        m_parserParamFlag |= 0x2;
        return;//不是则不解析（只有这种格式的数据，长得才像 name=ccc&age=18 这样由 & 和 = 拼接而成的字符串。）
    }
    ParseParam(m_body, m_params, '&', Trim);
    m_parserParamFlag |= 0x02;
}
void HttpRequest::initCookies(){
    if(m_parserParamFlag & 0x04) return;
    std::string cookie = getHeader("cookie");
    if(cookie.empty()){
        m_parserParamFlag |= 0x04;
        return;
    }
    ParseParam(cookie, m_cookies, ';', Trim);
    m_parserParamFlag |= 0x04;
}
void HttpRequest::initParamsAndCookies(){
    initQueryParam();
    initBodyParam();
    initCookies();
}



std::string HttpRequest::getHeader(const std::string& key,
                                   const std::string& def) const {
        auto it = m_headers.find(key);
        return it == m_headers.end() ? def : it->second;
}
std::string HttpRequest::getParam(const std::string& key,
                                  const std::string& def){
        initQueryParam();
        initBodyParam();
        auto it = m_params.find(key);
        return it == m_params.end() ? def : it->second;
}
std::string HttpRequest::getCookie(const std::string& key,
                                   const std::string& def){
        initCookies();
        auto it = m_cookies.find(key);
        return it == m_cookies.end() ? def : it->second;
}       

void HttpRequest::setHeader(const std::string& key,
                    const std::string& val){
        m_headers[key] = val;
}
void HttpRequest::delHeader(const std::string& key){
        m_headers.erase(key);
}

void HttpRequest::setParam(const std::string& key,
                const std::string& val){
        m_params[key] = val;
}
void HttpRequest::delParam(const std::string& key){
    m_params.erase(key);
}

void HttpRequest::setCookie(const std::string& key,
       const std::string& val){
        m_cookies[key] = val;
}
void HttpRequest::delCookie(const std::string& key){
    m_cookies.erase(key);
}

bool HttpRequest::hasHeader(const std::string& key,
          std::string* val){
        auto it = m_headers.find(key);
        if(it == m_headers.end()) return false;
        if(val) *val = it->second;
        return true;
}

bool HttpRequest::hasParam(const std::string& key,
          std::string* val){
        initQueryParam();
        initBodyParam();
        auto it = m_params.find(key);
        if(it == m_params.end()) return false;
        if(val) *val = it->second;
        return true;
}

bool HttpRequest::hasCookie(const std::string& key,
              std::string* val){
        initCookies();
        auto it = m_cookies.find(key);
        if(it == m_cookies.end()) return false;
        if(val) *val = it->second;
        return true;
}   


std::ostream &HttpRequest::dump(std::ostream &os) const{
    //GET /url HTTP/1.1
    //Host: www.baidu.com ...
    //\r\n\r\n
    //body
        os << HttpMethodToString(m_method) << " "
       << m_path
       << (m_query.empty() ? "" : "?")
       << m_query
       << (m_fragment.empty() ? "" : "#")
       << m_fragment
       << " HTTP/"
       << ((uint32_t)(m_version >> 4))
       << "."
       << ((uint32_t)(m_version & 0x0F))
       << "\r\n";
    if(!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }

    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}
std::string HttpRequest::toString() const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


/*----------------------------------httpResponse---------------------------*/


HttpResponse::HttpResponse(uint8_t version,bool close)
    :m_status(http_status::HTTP_STATUS_OK)
    ,m_close(close)
    ,m_version(version)
    ,m_websocket(false){}

std::string HttpResponse::getHeader(const std::string& key,
                                    const std::string& def) const{
    auto it = m_headers.find(key);
    return it == m_headers.end() ? def : it->second;
}
void HttpResponse::setHeader(const std::string& key,const std::string& val){
        m_headers[key] = val;
}
void HttpResponse::delHeader(const std::string& key){
        m_headers.erase(key);
}

void HttpResponse::setCookie(const std::string& key,
           const std::string& val,
           time_t expired ,
           const std::string& path,
           const std::string& domain,
           bool secure){
    std::stringstream ss;
    
    // 1. 组装基础的 键=值
    ss << key << "=" << val;
    
    // 2. 组装过期时间 (Expires)
    if(expired > 0) {
        ss << ";expires=" << Time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    
    // 3. 组装作用域名 (Domain)
    if(!domain.empty()) {
        ss << ";domain=" << domain;
    }
    
    // 4. 组装作用路径 (Path)
    if(!path.empty()) {
        ss << ";path=" << path;
    }
    
    // 5. 组装安全标志 (Secure)
    if(secure) {
        ss << ";secure";
    }
    
    // 6. 存入响应对象的 Cookie 列表中
    m_cookies.push_back(ss.str());
}
void HttpResponse::setRedirect(const std::string& url){
    m_status = http_status::HTTP_STATUS_FOUND;
    setHeader("Location", url);
}


std::ostream &HttpResponse::dump(std::ostream &os) const{
    os << "HTTP/"
    << ((uint32_t)(m_version >> 4))
    << "."
    << ((uint32_t)(m_version & 0x0F))
    << " "
    << (uint32_t)m_status
    << " "
    << (m_reason.empty() ? HttpStatusToString(m_status) : m_reason)
    << "\r\n";

    for(auto& i : m_headers) {
        if(!m_websocket && strcasecmp(i.first.c_str(), "connection") == 0) {
            continue;
        }
        os << i.first << ": " << i.second << "\r\n";
    }
    for(auto& i : m_cookies) {
        os << "Set-Cookie: " << i << "\r\n";
    }
    if(!m_websocket) {
        os << "connection: " << (m_close ? "close" : "keep-alive") << "\r\n";
    }
    if(!m_body.empty()) {
        os << "content-length: " << m_body.size() << "\r\n\r\n"
           << m_body;
    } else {
        os << "\r\n";
    }
    return os;
}
std::string HttpResponse::toString() const{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

std::ostream &operator<<(std::ostream &os, const HttpRequest &req){
    return req.dump(os);
}
std::ostream &operator<<(std::ostream &os, const HttpResponse &rsp){
    return rsp.dump(os);
}

}// namespace http

}// namespace DYX

