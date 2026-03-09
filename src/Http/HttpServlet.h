#pragma once
#include "HttpSession.h"
#include "../Util.h"
#include "../Mutex.h"
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
namespace DYX{
namespace http{

class Servlet{
public:
    using Ptr = std::shared_ptr<Servlet>;
    Servlet(const std::string& name)
        :m_name(name){}
    virtual ~Servlet(){}
    virtual int32_t handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session) = 0;

    const std::string& getName() const { return m_name;}

private:
    std::string m_name; 
};

//函数式Servlet
class FunctionServlet : public Servlet{
public:
    using Ptr = std::shared_ptr<FunctionServlet>;
    using callback = std::function<int32_t (HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session)>;
    FunctionServlet(callback cb);
    virtual int32_t handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session) override;
private:
    callback m_cb;
};

//接口式Servlet创建器
class InterfaceServletCreator{
public:
    using Ptr = std::shared_ptr<InterfaceServletCreator>;
    virtual ~InterfaceServletCreator(){}
    virtual Servlet::Ptr get() const = 0;
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public InterfaceServletCreator{
public:
    HoldServletCreator(Servlet::Ptr servlet)
        :m_servlet(servlet){}
    virtual Servlet::Ptr get() const override{
        return m_servlet;
    }
    virtual std::string getName() const override{
        return m_servlet->getName();
    }
private:
    Servlet::Ptr m_servlet;
};

template<class T>
class ServerletCreator : public InterfaceServletCreator{
public:
    ServerletCreator(){}
    virtual Servlet::Ptr get() const override{
        return Servlet::Ptr(new T);
    }
    virtual std::string getName() const override{
        return TypeToName<T>();
    }
};

class NotFoundServlet : public Servlet{
public:
    using Ptr = std::shared_ptr<NotFoundServlet>;
    NotFoundServlet(const std::string &name);
    virtual int32_t handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session) override;
private:
    std::string m_name;
    std::string m_content;
};


class ServletDispatcher : public Servlet{
public:
    using Ptr = std::shared_ptr<ServletDispatcher>;
    using MutexType = RWMutex;
    ServletDispatcher();
    virtual int32_t handle(HttpRequest::Ptr req, HttpResponse::Ptr resp, HttpSession::Ptr session) override;
    //获得精准匹配的uri资源
    Servlet::Ptr getMatchedServlet(const std::string& uri);
    //获得模糊匹配的uri资源
    Servlet::Ptr getGlobServlet(const std::string& uri);
    //含有兜底
    Servlet::Ptr getServlet(const std::string& uri);

    void setDefault(Servlet::Ptr v) { m_default = v;}
    Servlet::Ptr getDefault() const { return m_default;}

    void delMatchedServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    void addServlet(const std::string& uri, Servlet::Ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& uri, Servlet::Ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    // void addServletCreator(const std::string& uri, InterfaceServletCreator::Ptr creator);
    // void addGlobServletCreator(const std::string& uri, InterfaceServletCreator::Ptr creator);
    // template<class T>
    // void addServletCreator(const std::string& uri) {
    //     addServletCreator(uri, std::make_shared<ServletCreator<T> >());
    // }

    // template<class T>
    // void addGlobServletCreator(const std::string& uri) {
    //     addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
    // }
    
    void listAllServletCreator(std::map<std::string, InterfaceServletCreator::Ptr>& infos);
    void listAllGlobServletCreator(std::map<std::string, InterfaceServletCreator::Ptr>& infos);
private:
    MutexType m_mutex;
    //精准匹配（请求的uri与注册的uri完全一致）
    std::unordered_map<std::string ,InterfaceServletCreator::Ptr> m_datas;
    //模糊匹配（请求的uri与注册的uri前缀一致）
    std::vector<std::pair<std::string ,InterfaceServletCreator::Ptr>> m_globs;
    //默认Servlet(兜底)
    Servlet::Ptr m_default;
};



}//namespace http
}//namespace DYX
