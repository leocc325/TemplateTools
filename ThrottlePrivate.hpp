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

///判断返回值是否是void
template<typename Func>
struct ReturnVoid
{
    using RT =typename MetaUtility::FunctionTraits<typename std::decay<Func>::type>::ReturnType;
    constexpr static bool value = std::is_same<RT,void>::value;
};

///判断Func是否是自由函数或成员函数指针
template<typename Func>
struct IsFunctionPointer{
    using FreeFunc = typename std::remove_pointer<Func>::type;
    constexpr static bool value  = std::is_function<FreeFunc>::value || std::is_member_function_pointer<Func>::value;
};

///每一个函数都有一个与之对应的Throttle对象专门管理这个函数的调用
class AbstractThrottle : public QObject
{
public:
    AbstractThrottle(std::size_t interval);

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
    std::list<std::function<void()>> m_TaskQue;
    std::chrono::milliseconds m_Interval{1000};
    std::chrono::steady_clock::time_point m_LastCall;
};

///同步节流
class SyncThrottle:public AbstractThrottle
{
public:
    SyncThrottle(std::size_t interval);

    template<typename Func,typename...Args>
    typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    call(Func func,Args&&...args)
    {
        m_TargetThread =  this->thread();
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

    template<typename Func,typename Obj,typename...Args>
    typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    call(Func func,Obj* obj,Args&&...args)
    {
        m_TargetThread = this->thread();
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
public:
    AsyncThrottle(std::size_t interval);

    template<typename Func,typename...Args>
    typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    call(Func func,Args&&...args)
    {
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

    template<typename Func,typename Obj,typename...Args>
    typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    call(Func func,Obj* obj,Args&&...args)
    {
        std::function<void()> task = std::bind(std::forward<Func>(func),obj,std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

protected:
    void processTask() override;
};


#endif // THROTTLEPRIVATE_HPP
