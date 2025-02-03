#ifndef THREAD_H
#define THREAD_H

#include <thread> // 包含 C++ 标准线程库

// 抽象类
class Thread
{
public:
    Thread() {} // 构造函数
    ~Thread() { // 析构函数
        if(thread_) { // 如果线程存在
            Thread::Stop(); // 停止线程
        }
    }

    int Start() {} // 启动线程的函数，未实现

    int Stop() { // 停止线程的函数
        abort_ = 1; // 设置中止标志为1，表示线程应当结束
        if(thread_) { // 如果线程存在
            thread_->join(); // 等待线程结束
            delete thread_; // 释放线程对象
            thread_ = NULL; // 将线程指针设为NULL，避免悬挂指针
        }
        return 0; // 成功返回
    }

    virtual void Run() = 0; // 纯虚函数，子类必须实现该方法

protected:
    int abort_ = 0; // 用于控制线程的中止状态
    std::thread *thread_ = NULL; // 指向实际的线程对象
};

#endif // THREAD_H
