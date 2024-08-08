#include "ThrottlePrivate.hpp"

AbstractThrottle::AbstractThrottle(size_t interval):m_Interval(interval){}

AbstractThrottle::~AbstractThrottle(){}

bool AbstractThrottle::isEmpty()
{
    std::lock_guard<std::mutex> guard(m_Mutex);
    return m_TaskQue.empty();
}

void AbstractThrottle::taskThread()
{
    m_IsRunning.store(true);
    while (true)
    {
        //每隔m_Interval毫秒处理一次任务队列,将队列中最后一个任务取出
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastCall) >= m_Interval)
        {
            if(!this->isEmpty())
            {
                processTask();
                m_LastCall = std::chrono::steady_clock::now();
            }
            else
                break;//如果时间间隔大于m_Interval且任务队列为空,就可以退出线程
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_IsRunning.store(false);
}

void AbstractThrottle::addTask(std::function<void ()> &&task)
{
    {
        std::lock_guard<std::mutex> guard(m_Mutex);
        m_TaskQue.push_back(task);
    }

    //如果线程还没有启动,就启动线程处理任务队列
    if(!m_IsRunning.load())
    {
        std::thread t(&AbstractThrottle::taskThread,this);
        t.detach();
    }
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

SyncThrottle::SyncThrottle(size_t interval):AbstractThrottle (interval){}

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
        QMetaObject::invokeMethod(m_TargetThread,task);
}
