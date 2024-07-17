#ifndef THROTTLER_HPP
#define THROTTLER_HPP

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

/// 可以通过宏调用节流接口，参数从前到后分别为:
/// 处理函数的间隔时间、函数指针、类对象指针(如果不是成员函数则忽略这个参数)、参数1、参数2...
/// 此外，暂时不接受返回值为void以外的函数

#define SYNCTHROTTLE(time,...) \
static SyncThrottle throttle(time);\
QThread* thread = this->thread();\
throttle.call(thread,__VA_ARGS__);

#define ASYNCTHROTTLE(time,...) \
static AsyncThrottle throttle(time);\
throttle.call(__VA_ARGS__);

template<typename Func>
struct ReturnVoid
{
    using RT =typename FunctionTraits<typename std::decay<Func>::type>::ReturnType;
    constexpr static bool value = std::is_same<RT,void>::value;
};

template<typename Func>
struct IsFunction{
    using NonrefFunc = typename std::remove_reference<Func>::type;//这里使用decay不能正确推导自由函数类型，编译器你又在发什么癫!!
    constexpr static bool value  = std::is_function<NonrefFunc>::value || std::is_member_function_pointer<NonrefFunc>::value;
};

template <typename Func>
typename std::enable_if<IsFunction<Func>::value,std::string>::type
getFunctionAddress(Func&& func)
{
    void* ptr = reinterpret_cast<void*>(func);
    std::stringstream ss;
    ss << std::hex <<ptr;
    return ss.str();
}

/*
template <typename Class, typename Ret, typename... Args>
std::string getFunctionAddress(Ret (Class::*ptr)(Args...))
{
    union
    {
        Ret (Class::*memberFunc)(Args...);
        void* generalFunc;
    } converter;
    converter.memberFunc = ptr;
    std::stringstream ss;
    ss << std::hex <<reinterpret_cast<uintptr_t>(converter.generalFunc);
    return ss.str();
}
*/

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
    std::list<std::function<void()>> m_TaskQue;
    std::chrono::milliseconds m_Interval{1000};
    std::chrono::steady_clock::time_point m_LastCall;
};

///同步节流
class SyncThrottle:public AbstractThrottle
{
    ///捕获时间间隔内最后一次被调用的函数和参数，并将其交还给捕获动作发生所在的线程，被捕获的函数会在其自身线程的下一次事件循环中被调用
public:
    SyncThrottle(size_t interval);

    template<typename Func,typename...Args>
    typename std::enable_if<ReturnVoid<Func>::value>::type
    call(QThread* thrd,Func&& func,Args&&...args)
    {
        m_TargetThread = thrd;
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

    template<typename Func,typename Obj,typename...Args>
    typename std::enable_if<ReturnVoid<Func>::value>::type
    call(QThread* thrd, Func&& func,Obj* obj,Args&&...args)
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
    ///传入的函数会在单独的线程中执行,不建议调用GUI相关的函数
public:
    AsyncThrottle(size_t interval);

    template<typename Func,typename...Args>
    typename std::enable_if<ReturnVoid<Func>::value>::type
    call(Func&& func,Args&&...args)
    {
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

    template<typename Func,typename Obj,typename...Args>
    typename std::enable_if<ReturnVoid<Func>::value>::type
    call(Func&& func,Obj* obj,Args&&...args)
    {
        std::function<void()> task = std::bind(std::forward<Func>(func),obj,std::forward<Args>(args)...);
        this->addTask(std::move(task));
    }

protected:
    void processTask() override;
};

namespace Throttle {

}
#endif // THROTTLER_HPP
