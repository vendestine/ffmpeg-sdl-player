#ifndef QUEUE_H
#define QUEUE_H

#include <mutex>                // 互斥量
#include <condition_variable>   // 条件变量
#include <queue>                // 标准队列类

// 队列 类模板
template <typename T>
class Queue {
public:
    Queue() {} // 构造函数
    ~Queue() {} // 析构函数

    // 设置队列为终止状态，并通知所有等待的线程
    void Abort() {
        abort_ = 1;                // 设置 abort_ 标志为1，表示队列已放弃（abort）
        cond_.notify_all();         // 唤醒所有等待线程
    }

    // 向队列中添加元素
    int Push(T val) {
        std::lock_guard<std::mutex> lock(mutex_); // 保护临界区
        if (1 == abort_) { // 检查是否已经终止
            return -1; // 如果是，返回-1表示失败
        }
        queue_.push(val); // 将值插入队列
        cond_.notify_one(); // 通知一个等待线程
        return 0; // 成功返回 0
    }

    // 从队列中弹出元素
    int Pop(T &val, const int timeout = 0) {
        std::unique_lock<std::mutex> lock(mutex_); // 使用 unique_lock 以便于条件变量的等待
        if (queue_.empty()) { // 如果队列为空
            // 等待 push 或者超时唤醒
            cond_.wait_for(lock, std::chrono::milliseconds(timeout), [this] {
                return !queue_.empty() || abort_; // 等待条件：队列非空或已终止
            });
        }
        if (1 == abort_) { // 如果队列已经被终止
            return -1; // 返回-1表示失败
        }
        if (queue_.empty()) { // 如果队列仍为空（超时）
            return -2; // 返回-2表示超时
        }
        val = queue_.front(); // 获取队头元素
        queue_.pop(); // 弹出队头元素
        return 0; // 成功返回 0
    }

    // 查看队头元素但不弹出
    int Front(T &val) {
        std::lock_guard<std::mutex> lock(mutex_); // 保护临界区
        if (1 == abort_) { // 检查是否已经终止
            return -1; // 返回-1表示失败
        }
        if (queue_.empty()) { // 如果队列为空
            return -2; // 返回-2表示队列为空
        }
        val = queue_.front(); // 获取队头元素
        return 0; // 成功返回 0
    }

    // 获取队列大小
    int Size() {
        std::lock_guard<std::mutex> lock(mutex_); // 保护临界区
        return queue_.size(); // 返回队列大小
    }

private:
    int abort_ = 0;                 // 队列终止标志，0 表示正常，1 表示终止
    std::mutex mutex_;              // 用于保护队列的互斥量
    std::condition_variable cond_;  // 条件变量用于等待和通知
    std::queue<T> queue_;           // 存储实际元素的标准队列
};

#endif // QUEUE_H
