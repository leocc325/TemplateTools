#include "ThrottlePrivate.hpp"

AbstractThrottle::AbstractThrottle(size_t interval):m_Interval(interval)
{
    m_QuitFlag.store(false,std::memory_order_relaxed);
    std::thread t(&AbstractThrottle::taskThread,this);
    t.detach();
}

AbstractThrottle::~AbstractThrottle()
{
    {
        std::lock_guard<std::mutex> guard(m_Mutex);
        m_QuitFlag.store(true,std::memory_order_release);
    }

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
        bool newTask = !m_TaskQue.empty();
        bool quitFlag = m_QuitFlag.load(std::memory_order_relaxed);
        return (newTask || quitFlag);
    };

    auto quit = [this](){
        return m_QuitFlag.load(std::memory_order_relaxed);
    };

    while (!quit())
    {
        if(this->isEmpty())
        {
            std::unique_lock<std::mutex> waitlock(m_Mutex);
            m_CV.wait(waitlock,pred);
        }
        else
        {
            processTask();

            std::unique_lock<std::mutex> delayLock(m_Mutex);
            m_CV.wait_for(delayLock,std::chrono::milliseconds(m_Interval),quit);
        }
    }
}

void AbstractThrottle::addTask(std::function<void ()> &&task)
{
    bool notifyFlag = false;
    {
        std::lock_guard<std::mutex> guard(m_Mutex);
        notifyFlag = m_TaskQue.empty();
        m_TaskQue.push_back(std::move(task));
    }

    //仅在任务队列为空的情况下才唤醒条件变量,避免频繁唤醒造成资源浪费
    if(notifyFlag)
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
        QMetaObject::invokeMethod(m_TargetThread,std::move(task),Qt::DirectConnection);
    //这里使用Qt::DirectConnection的原因见https://www.cnblogs.com/GengMingYan/p/18239440例5
}
