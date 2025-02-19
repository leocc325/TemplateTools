#include "DelayTask.hpp"

DelayTaskProcessor::DelayTaskProcessor()
{

}

DelayTaskProcessor::~DelayTaskProcessor()
{
    auto begin = m_TaskMap.cbegin();
    while (begin != m_TaskMap.end())
    {
        //停止任务、释放内存
        (*begin).second->stop();
        ++begin;
    }
}

DelayTask::~DelayTask()
{
    this->stop();
}

void DelayTask::changeDelay(std::size_t mseconds)
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Delay = mseconds;
    m_Interrupt.store(true,std::memory_order_relaxed);
    m_CV.notify_one();
}

void DelayTask::stop()
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_Interrupt.store(true,std::memory_order_relaxed);
    m_CV.notify_one();
}

void DelayTask::threadImpl()
{
    try
    {
        std::unique_lock<std::mutex> lock(m_Mutex);

        m_Running.store(true,std::memory_order_relaxed);
        m_Interrupt.store(false,std::memory_order_relaxed);

        m_CV.wait_for(lock,std::chrono::milliseconds(m_Delay),[this](){
            return m_Interrupt.load(std::memory_order_relaxed);
        });

        //如果task包含了可执行函数而且等待过程中没有被打断,就执行task
        bool doExcute = m_Task.operator bool() && !m_Interrupt.load(std::memory_order_relaxed);
        if(doExcute)
        {
            //将函数发送到该对象被创建时的线程中执行,通常是主线程
            QMetaObject::invokeMethod(this->thread(),std::move(m_Task));
        }

        m_Running.store(false,std::memory_order_relaxed);
    }
    catch (const std::exception& e)
    {
        m_Running.store(false,std::memory_order_relaxed);
        m_Interrupt.store(true,std::memory_order_relaxed);
        throw e;
    }
}
