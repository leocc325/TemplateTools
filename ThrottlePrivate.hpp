#ifndef THROTTLEPRIVATE_HPP
#define THROTTLEPRIVATE_HPP

#include <type_traits>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <list>
#include <sstream>

#include <QApplication>
#include <QThread>
#include <QDebug>

#include "FunctionTraits.hpp"

///每一个函数都有一个与之对应的Throttle对象专门管理这个函数的调用
class AbstractThrottle : public QObject
{
public:
    AbstractThrottle(std::size_t interval);

    virtual ~AbstractThrottle();

    void addTask(std::function<void()>&& task);

protected:
    bool isEmpty();

    ///线程函数,用于处理任务队列,调用每次有任务进入队列时都尝试启动这个线程函数,队列中任务为空时结束线程
    void taskThread();

    virtual void processTask() = 0;

protected:
    std::mutex m_Mutex;
    std::atomic<bool> m_IsRunning{false};
    std::list<std::function<void()>> m_TaskQue;
    std::chrono::milliseconds m_Interval{1000};
    std::chrono::steady_clock::time_point m_LastCall;
};

///同步节流,当SyncThrottle在多个线程中addTask时,最终会将任务转发至创建SyncThrottle对象时所在的线程
///因为事件循环的线程是在SyncThrottle对象被创建时确定的
class SyncThrottle:public AbstractThrottle
{
public:
    SyncThrottle(std::size_t interval);

protected:
    void processTask() override;

protected:
    QThread* m_TargetThread = nullptr;
};

///异步节流
class AsyncThrottle:public AbstractThrottle
{
public:
    AsyncThrottle(std::size_t interval);

protected:
    void processTask() override;
};


#endif // THROTTLEPRIVATE_HPP
