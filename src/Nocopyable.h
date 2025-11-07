#ifndef __NOCOPYABLE_H__
#define __NOCOPYABLE_H__

class Nocopy{
public:
    Nocopy() = default;
    ~Nocopy() = default;
    
    //禁用拷贝和赋值
    Nocopy(const Nocopy&) = delete; //禁用拷贝构造
    Nocopy(Nocopy&&) = delete;      //禁用移动构造
    Nocopy& operator=(const Nocopy&) = delete;

};

#endif
