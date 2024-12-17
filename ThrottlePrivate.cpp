#include "ThrottlePrivate.hpp"

AbstractThrottle::AbstractThrottle(size_t interval):m_Interval(interval)
{
    m_QuitFlag.store(false,std::memory_order_relaxed);
    std::thread t(&AbstractThrottle::taskThread,this);
    t.detach();
}

AbstractThrottle::~AbstractThrottle()
{
    m_QuitFlag.store(true,std::memory_order_release);
    m_CV.notify_one();
}

bool AbstractThrottle::isEmpty()
{
    std::lock_guard<std::mutex> guard(m_Mutex);
    return m_TaskQue.empty();
}

void AbstractThrottle::taskThread()
{
    auto pred = [this](){
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        bool ret = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastCall) >= m_Interval;
        //时间间隔超过设置值或者要退出while循环时均返回true
        return ( ret || m_QuitFlag.load(std::memory_order_relaxed) );
    };

    while (true)
    {
        if(!this->isEmpty())
        {
            processTask();
            m_LastCall = std::chrono::steady_clock::now();
        }

        //一定要在这里判断退出标志位,避免m_QuitFlag设置为true时正在执行processTask(),这将会使notify不起作用导致break再次等待wait_for结束之后才执行
        if(m_QuitFlag.load(std::memory_order_acquire))
            break;

        std::unique_lock<std::mutex> ulock(m_Mutex);
        m_CV.wait_for(ulock,std::chrono::milliseconds(m_Interval),pred);
    }
}

void AbstractThrottle::addTask(std::function<void ()> &&task)
{
    {
        std::lock_guard<std::mutex> guard(m_Mutex);
        m_TaskQue.push_back(std::move(task));
    }

    m_CV.notify_one();
}

AsyncThrottle::AsyncThrottle(size_t interval):AbstractThrottle (interval){}

void AsyncThrottle::processTask()
{
    std::function<void()> task;
    {
        //取出最后一个任务,然后清空队列
        std::lock_guard<std::mutex> guard(m_Mutex);
        task = std::move(m_TaskQue.back());
        m_TaskQue.clear();
    }

    if(task.operator bool())
        task();
}

SyncThrottle::SyncThrottle(size_t interval):AbstractThrottle (interval){
    m_TargetThread = this->thread();
}

void SyncThrottle::processTask()
{
    std::function<void()> task;
    {
        //取出最后一个任务,然后清空队列
        std::lock_guard<std::mutex> guard(m_Mutex);
        task = std::move(m_TaskQue.back());
        m_TaskQue.clear();
    }

    if(task.operator bool() && m_TargetThread)
        QMetaObject::invokeMethod(m_TargetThread,std::move(task));
}
