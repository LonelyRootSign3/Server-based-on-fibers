#include "HttpServlet.h"
#include <fnmatch.h>
namespace DYX{
namespace http{

FunctionServlet::FunctionServlet(callback cb)
        :m_cb(cb)
        ,Servlet("FunctionServlet"){}
        
int32_t FunctionServlet::handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session){
    return m_cb(req, resp, session);
}

NotFoundServlet::NotFoundServlet(const std::string &name)
    :Servlet("NotFoundServlet")
    ,m_name(name) {
    m_content = "<html><head><title>404 Not Found"
    "</title></head><body><center><h1>404 Not Found</h1></center>"
    "<hr><center>" + name + "</center></body></html>";
}
int32_t NotFoundServlet::handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session){
    resp->setStatus(http_status::HTTP_STATUS_NOT_FOUND);
    resp->setHeader("Server","dyx/1.0.0");
    resp->setHeader("Content-Type","text/html");
    resp->setBody(m_content);
    return 0;
}
ServletDispatcher::ServletDispatcher()
    :Servlet("ServletDispatcher") 
    ,m_default(std::make_shared<NotFoundServlet>("dyx/1.0.0")){}

int32_t ServletDispatcher::handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session){
    auto it = getServlet(req->getPath());
    if(it){
        return it->handle(req, resp, session);
    }
    return m_default->handle(req, resp, session);
}
  
Servlet::Ptr ServletDispatcher::getMatchedServlet(const std::string& uri){
    MutexType::RLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second->get();
}
Servlet::Ptr ServletDispatcher::getGlobServlet(const std::string& uri){
    MutexType::RLock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end() ;++it){
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0)) {
            return it->second->get();
        }
    }
    return nullptr;
}
Servlet::Ptr ServletDispatcher::getServlet(const std::string& uri){
    auto it = getMatchedServlet(uri);
    if(it) return it;
    it = getGlobServlet(uri);
    if(it) return it;
    return m_default;
}

void ServletDispatcher::delMatchedServlet(const std::string& uri){
    MutexType::WLock lock(m_mutex);
    m_datas.erase(uri);
}
void ServletDispatcher::delGlobServlet(const std::string& uri){
     MutexType::WLock lock(m_mutex);
     for(auto it = m_globs.begin();it != m_globs.end();++it){
         if(it->first == uri){
             m_globs.erase(it);
             break;
         }
     }

}
void ServletDispatcher::addServlet(const std::string& uri, Servlet::Ptr slt){
    MutexType::WLock lock(m_mutex);
    m_datas[uri] = std::make_shared<HoldServletCreator>(slt);
}
void ServletDispatcher::addServlet(const std::string& uri, FunctionServlet::callback cb){
    MutexType::WLock lock(m_mutex);
    m_datas[uri] = std::make_shared<HoldServletCreator>(std::make_shared<FunctionServlet>(cb));
}
void ServletDispatcher::addGlobServlet(const std::string& uri, Servlet::Ptr slt){
    MutexType::WLock lock(m_mutex);
    for(auto it = m_globs.begin();it != m_globs.end();++it){
         if(it->first == uri){
             m_globs.erase(it);
             break;
         }
     }
     m_globs.push_back(std::make_pair(uri, std::make_shared<HoldServletCreator>(slt)));
}
void ServletDispatcher::addGlobServlet(const std::string& uri, FunctionServlet::callback cb){
    addGlobServlet(uri, std::make_shared<FunctionServlet>(cb));
}
// void ServletDispatcher::addServletCreator(const std::string& uri, InterfaceServletCreator::Ptr creator){
//     MutexType::WLock lock(m_mutex);
//      m_datas[uri] = creator;
// }
// void ServletDispatcher::addGlobServletCreator(const std::string& uri, InterfaceServletCreator::Ptr creator){
//     MutexType::WLock lock(m_mutex);
//     for(auto it = m_globs.begin();
//             it != m_globs.end(); ++it) {
//         if(it->first == uri) {
//             m_globs.erase(it);
//             break;
//         }
//     }
//     m_globs.push_back(std::make_pair(uri, creator));
// }
void ServletDispatcher::listAllServletCreator(std::map<std::string, InterfaceServletCreator::Ptr>& infos){
    MutexType::RLock lock(m_mutex);
    for(auto &it : m_datas){
        infos[it.first] = it.second;
    }
}
void ServletDispatcher::listAllGlobServletCreator(std::map<std::string, InterfaceServletCreator::Ptr>& infos){
    MutexType::RLock lock(m_mutex);
    for(auto &it : m_globs){
        infos[it.first] = it.second;
    }
}
}
}
