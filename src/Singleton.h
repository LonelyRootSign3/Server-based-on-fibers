#pragma once
#include <memory>

namespace DYX{

//返回引用单例
template<typename T,typename X,int N>
T &GetInstance(){
    static T var;
    return var;
}

//返回智能指针单例
template<typename T,typename X,int N>
std::shared_ptr<T> GetSharedPtrInstance(){
    static std::shared_ptr<T> var = std::make_shared<T>();
    return var;
}




// T 类型
// X 为了创造多个实例对应的Tag
// N 同一个Tag创造多个实例索引
//单例模式封装类
template<typename T,typename X = void,int N = 0>
class Singleton{
public:
    static T *GetInstance(){
        static T var;
        return &var;
    }
};


template<typename T,typename X = void ,int N = 0>
class SingletonPtr{
public:
    static std::shared_ptr<T> GetInstance(){
        static std::shared_ptr<T> var = std::make_shared<T>();//调用时初始化（静态变量）
        return var;
    }
};
}
