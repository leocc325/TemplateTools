#ifndef THROTTLER_HPP
#define THROTTLER_HPP

#include <type_traits>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <list>

#include <QApplication>
#include <QThread>
#include <QDebug>

#define SYNCTHROTTLE(time,...) \
static SyncThrottle throttle(time);\
QThread* thread = this->thread();\
throttle.call(thread,__VA_ARGS__);

#define ASYNCTHROTTLE(time,...) \
static AsyncThrottle throttle(time);\
throttle.call(__VA_ARGS__);

///每一个函数都有一个与之对应的Throttle对象专门管理这个函数的调用
class AbstractThrottle
{
public:
    AbstractThrottle(size_t interval);

    virtual ~AbstractThrottle();

protected:
    bool isEmpty();

    ///线程函数,用于处理任务队列,调用每次有任务进入队列时都尝试启动这个线程函数,队列中任务为空时结束线程
    void taskThread();

    void addTask(std::function<void()>&& task);

    virtual void processTask() = 0;

protected:
    std::mutex m_Mutex;
    std::atomic<bool> m_IsRunning{false};
    std::function<void()> m_StoredFunc;
    std::list<std::function<void()>> m_TaskQue;
    std::chrono::milliseconds m_Interval{1000};
    std::chrono::steady_clock::time_point m_LastCall;
};

///同步节流
class SyncThrottle:public AbstractThrottle
{
    //捕获时间间隔内最后一次被调用的函数和参数，并将其交还给捕获动作发生所在的线程，被捕获的函数会在其自身线程的下一次事件循环中被调用
public:
    SyncThrottle(size_t interval);

    ///普通函数,std::function,lambda
    template<typename Func,typename...Args>
    void call(QThread* thrd,Func&& func,Args&&...args)
    {
        m_TargetThread = thrd;
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

    ///成员函数
    template<typename Func,typename Obj,typename...Args>
    void call(QThread* thrd, Func&& func,Obj* obj,Args&&...args)
    {
        m_TargetThread = thrd;
        std::function<void()> task = std::bind(std::forward<Func>(func),obj,std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

protected:
    void processTask() override;

protected:
    QThread* m_TargetThread = nullptr;
};

///异步节流
class AsyncThrottle:public AbstractThrottle
{
    //传入的函数会在单独的线程中执行,不建议调用GUI相关的函数
public:
    AsyncThrottle(size_t interval);

    ///普通函数,std::function,lambda
    template<typename Func,typename...Args>
    void call(Func&& func,Args&&...args)
    {
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

    ///成员函数
    template<typename Func,typename Obj,typename...Args>
    void call(Func&& func,Obj* obj,Args&&...args)
    {
        std::function<void()> task = std::bind(std::forward<Func>(func),obj,std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

protected:
    void processTask() override;
};
#endif // THROTTLER_HPP
