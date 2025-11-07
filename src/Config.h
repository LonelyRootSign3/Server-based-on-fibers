#pragma once

#include <memory>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <set>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <yaml-cpp/yaml.h>
#include "Util.h"
#include "Log.h"
#include "Mutex.h"
namespace DYX{

//配置变量的基类
class ConfigVarBase{//基类
public:
    using Ptr = std::shared_ptr<ConfigVarBase>;//智能指针类型定义

    ConfigVarBase(const std::string &name, const std::string &desc = "")
        :var_name(name), var_desc(desc)
    { std::transform(var_name.begin(), var_name.end(), var_name.begin(), ::tolower); }//把参数名称统一转成小写（避免大小写不一致带来的混乱）
    
    virtual ~ConfigVarBase(){}

    std::string &Get_name() { return var_name; }//获取配置变量的名称
    std::string &Get_desc() { return var_desc; }//获取配置变量的描述

    virtual bool Fromstring(const std::string &str) = 0;//从字符串中解析配置变量的值
    virtual std::string ToString()  = 0;//将配置变量的值转换为字符串
    virtual std::string GetTypeName() const = 0;//获取配置变量的类型
protected:
    // 每个配置项都有一个唯一的名字和文字说明。
    std::string var_name;//配置项名称
    std::string var_desc;// 配置项描述
};

//Src: 源类型，Tar: 目标类型
template<class Src,class Tar>
struct LexicalCast{//类型转换模板类 ->仿函数
    Tar operator()(const Src &src){
        std::stringstream ss;
        ss<<src;// 把源类型写入到流中
        Tar tar;
        ss>>tar;// 从流中读取到目标类型

        // 检查是否转换完整（例如 "123abc" 转 int 时会有残余）
        if (!ss.eof() || ss.fail()) {
            throw std::invalid_argument("LexicalCast: conversion failed");
        }
        return tar; 
    }

};

// 针对 string -> string 的特化（避免额外开销）
template<>  
struct LexicalCast<std::string, std::string> {
    std::string operator()(const std::string& v) {
        return v;
    }
};

// -------------------- vector --------------------

template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string& s) {
        YAML::Node node = YAML::Load(s);
        std::vector<T> out;
        if (!node || !node.IsSequence()) return out;
        out.reserve(node.size());
        for (std::size_t i = 0; i < node.size(); ++i) {
            const std::string val = node[i].as<std::string>();
            out.push_back(LexicalCast<std::string, T>()(val));
        }
        return out;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const auto& e : v) {
            const std::string s = LexicalCast<T, std::string>()(e);
            node.push_back(s); // ★ 直接放字符串
        }
        return YAML::Dump(node);
    }
};

// -------------------- list --------------------

template<class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string& s) {
        YAML::Node node = YAML::Load(s);
        std::list<T> out;
        if (!node || !node.IsSequence()) return out;
        for (std::size_t i = 0; i < node.size(); ++i) {
            const std::string val = node[i].as<std::string>();
            out.push_back(LexicalCast<std::string, T>()(val));
        }
        return out;
    }
};

template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const auto& e : v) {
            const std::string s = LexicalCast<T, std::string>()(e);
            node.push_back(s);
        }
        return YAML::Dump(node);
    }
};

// -------------------- set --------------------

template<class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string& s) {
        YAML::Node node = YAML::Load(s);
        std::set<T> out;
        if (!node || !node.IsSequence()) return out;

        for (std::size_t i = 0; i < node.size(); ++i) {
            // ✅ 把每个子节点(Map/Scalar/Sequence)转成YAML字符串再递归解析
            const std::string val = YAML::Dump(node[i]);
            out.insert(LexicalCast<std::string, T>()(val));
        }
        return out;
    }
};

template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const auto& e : v) {
            const std::string s = LexicalCast<T, std::string>()(e);
            node.push_back(s);
        }
        return YAML::Dump(node);
    }
};

// -------------------- unordered_set --------------------

template<class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string& s) {
        YAML::Node node = YAML::Load(s);
        std::unordered_set<T> out;
        if (!node || !node.IsSequence()) return out;
        for (std::size_t i = 0; i < node.size(); ++i) {
            const std::string val = node[i].as<std::string>();
            out.insert(LexicalCast<std::string, T>()(val));
        }
        return out;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T>& v) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (const auto& e : v) {
            const std::string s = LexicalCast<T, std::string>()(e);
            node.push_back(s);
        }
        return YAML::Dump(node);
    }
};

// -------------------- map<string, T> --------------------

template<class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string& s) {
        YAML::Node node = YAML::Load(s);
        std::map<std::string, T> out;
        if (!node || !node.IsMap()) return out;
        for (auto it = node.begin(); it != node.end(); ++it) {
            const std::string key = it->first.as<std::string>();
            const std::string val = it->second.as<std::string>();
            out.emplace(key, LexicalCast<std::string, T>()(val));
        }
        return out;
    }
};

template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for (const auto& kv : v) {
            const std::string s = LexicalCast<T, std::string>()(kv.second);
            node[kv.first] = s; // ★ 直接放字符串
        }
        return YAML::Dump(node);
    }
};

// -------------------- unordered_map<string, T> --------------------

template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string& s) {
        YAML::Node node = YAML::Load(s);
        std::unordered_map<std::string, T> out;
        if (!node || !node.IsMap()) return out;
        out.reserve(node.size());
        for (auto it = node.begin(); it != node.end(); ++it) {
            const std::string key = it->first.as<std::string>();
            const std::string val = it->second.as<std::string>();
            out.emplace(key, LexicalCast<std::string, T>()(val));
        }
        return out;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T>& v) {
        YAML::Node node(YAML::NodeType::Map);
        for (const auto& kv : v) {
            const std::string s = LexicalCast<T, std::string>()(kv.second);
            node[kv.first] = s;
        }
        return YAML::Dump(node);
    }
};

// 如果派生类不同对象的数据独立，使用同一把锁反而降低并发性能，所以这里派生类自己定义自己的锁

//配置项类：继承自ConfigVarBase
//这里有变更回调函数的支持（观察者模式，对于配置变量更新后的监听）
template<typename T , typename FromStr = LexicalCast<std::string, T> \
                    , typename ToStr = LexicalCast<T, std::string> >
class ConfigVar : public ConfigVarBase{
public:
    //变更配置项时的回调函数
    using change_call_back = std::function<void(const T& old_value , const T& new_value)>;
    using Ptr = std::shared_ptr<ConfigVar<T, FromStr, ToStr>>;
    using MutexType = DYX::RWMutex; //读写锁类型定义

    ConfigVar(const std::string &name, const T &value, const std::string &desc = "")
        :ConfigVarBase(name, desc), _value(value)
    {}

    T getValue(){ return _value; }

    void setValue(const T &value)
    {
        {//先加读锁判断新旧值是否相等
            MutexType::RLock lock(_mutex);
            if(_value == value) {
                std::cout<<"值相同不变更"<<std::endl;
                return;
            }
        }
        MutexType::WLock lock(_mutex);//加写锁

        //设置新值的话就触发变更回调函数
        //有的回调会修改配置项的值，所以要在赋值之前触发
        for(auto &it:_change_callbacks){
            // std::cout<<"触发变更回调函数 id: "<<it.first<<std::endl;
            STREAM_LOG_DEBUG(GET_ROOT())<<"触发回调函数 id: "<<it.first;    
            it.second(_value, value);  //changecallback(old,new)
        }
        _value = value;//将新的值赋给配置项旧值
        STREAM_LOG_DEBUG(GET_ROOT())<<"ConfigVar<"<<Get_name()<<"> changed.";
    }



    virtual bool Fromstring(const std::string &str) override//从字符串中解析配置变量的值
    {
        try{
            // std::cout<<"--------------------------------------------------------"<<std::endl;
            // STREAM_LOG_DEBUG(GET_ROOT())<<"ConfigVar<"<<GetTypeName()<<">::Fromstring: \n"<<str;
            // std::cout<<"--------------------------------------------------------"<<std::endl;
            setValue(FromStr()(str));//setValue内部已经加锁了，这里不需要重复加锁，避免死锁
            return true;
        }catch(const std::exception& e){
            STREAM_LOG_ERROR(GET_ROOT())<<"ConfigVar<"<<GetTypeName()<<">::Fromstring error: "<<e.what();
            // std::cerr<<"ConfigVar<"<<GetTypeName()<<">::Fromstring: "<<e.what()<<std::endl;
            return false;
        }
    }
    virtual std::string ToString() override//将配置变量的值转换为字符串
    {
        try{
            return ToStr()(_value);
        }catch(const std::exception& e){
            std::cerr<<"ConfigVar<"<<GetTypeName()<<">::ToString: "<<e.what()<<std::endl;
            return "";
        }
    }
    virtual std::string GetTypeName() const override{//获取配置变量的类型
        return TypeToName<T>();
    }

    //添加变更回调函数
    inline uint64_t addChangeCallback(const change_call_back& callback){
        MutexType::WLock lock(_mutex);
        static uint64_t id = 0;
        ++id;
        _change_callbacks[id] = callback;
        return id;
    }

    //获取变更回调函数
    inline change_call_back getChangeCallback(uint64_t id){
        MutexType::RLock lock(_mutex);
        auto it = _change_callbacks.find(id);
        return it == _change_callbacks.end()? nullptr : it->second;
    }

    //删除变更回调函数
    inline void delChangeCallback(uint64_t id){
        MutexType::WLock lock(_mutex);
        auto it = _change_callbacks.find(id);
        if(it == _change_callbacks.end()) return;
        _change_callbacks.erase(it);
    }

    //清空变更回调函数
    inline void clearChangeCallbacks(){
        MutexType::WLock lock(_mutex);
        _change_callbacks.clear();
    }

    inline void showchangeCallbacks(){
        MutexType::RLock lock(_mutex);
        for(auto &it : _change_callbacks){
            std::cout<<"id: "<<it.first<<std::endl;
        }
    }
    inline void definethis(){return;}
private:
    T _value;
    std::unordered_map<uint64_t, change_call_back> _change_callbacks; //变更回调函数集合

    //锁保护，保证多线程下配置变量的安全访问
    mutable MutexType _mutex;
};


//配置管理类(ConfigVar管理器)
//提供便捷的方法访问配置项，并提供配置项变更时的通知
//该类采用单例模式。
class ConfigManager{
public:
    using ConfigMap = std::unordered_map<std::string, ConfigVarBase::Ptr>;   //配置项集合
    using MutexType = DYX::RWMutex; //读写锁类型定义
    /**
     * @brief 获取/创建对应参数名的配置参数
     * @param[in] name 配置参数名称
     * @param[in] default_value 参数默认值
     * @param[in] description 参数描述
     * @details 获取参数名为name的配置参数,如果存在直接返回
     *          如果不存在,创建参数配置并用default_value赋值
     * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
     * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
     */
    template<class T>
    static typename ConfigVar<T>::Ptr Lookup(const std::string& name,
            const T& default_value, const std::string& description = "") {

        MutexType::WLock lock(getMutex());
        auto it = getDatas().find(name);
        if(it != getDatas().end()) {
            auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
            if(tmp) {
                STREAM_LOG_ERROR(GET_ROOT())<< "Lookup name=" << name << " exists";
                return tmp;
            } else {
                STREAM_LOG_ERROR(GET_ROOT()) << "Lookup name=" << name << " exists but type not "
                        << TypeToName<T>() << " real_type=" << it->second->GetTypeName()
                        << " " <<it->second->ToString();
                return nullptr;
            }
        }

        if(name.find_first_not_of("abcdefghikjlmnopqrstuvwxyz._012345678")
                != std::string::npos) {
            STREAM_LOG_ERROR(GET_ROOT()) << "Lookup name invalid " << name;
            throw std::invalid_argument(name);
        }
        typename ConfigVar<T>::Ptr v = std::make_shared<ConfigVar<T>>(name, default_value, description);
        getDatas()[name] = v;
        return v;
    }

    /**
     * @brief 查找配置参数
     * @param[in] name 配置参数名称
     * @return 返回配置参数名为name的配置参数
     */
    template<class T>
    static typename ConfigVar<T>::Ptr Lookup(const std::string& name) {
        
        MutexType::RLock lock(getMutex());
        auto it = getDatas().find(name);
        if(it == getDatas().end()) {
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
    }

    //获取配置项的基类指针，当你不知道具体类型时，可以用基类来做统一处理。
    static ConfigVarBase::Ptr LookupBase(const std::string &name);

    // 使用YAML::Node初始化配置模块
    static bool LoadFromYaml(const YAML::Node &node);

    // 加载指定文件夹（path）里面的配置文件
    static void LoadFromConfigDir(const std::string &path , bool force = false);

    //遍历配置模块里面所有配置项, 输入配置项的回调函数
    static void Visit(std::function<void(ConfigVarBase::Ptr)> cb);

private:
    static ConfigMap& getDatas();//确保配置项集合先于其他静态对象创建(属于类的私有静态成员函数方法)
    static MutexType& getMutex();//确保读写锁先于其他静态对象创建(属于类的私有静态成员函数方法)
};

}
