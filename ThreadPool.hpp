#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <queue>
#include <vector>
#include <map>
#include <QDebug>
#include <QThread>
#include "FunctionTraits.hpp"
/**
 * @brief The ThreadQueue class
 */
class ThreadQueue
{
    friend class ThreadPool;
public:
    ThreadQueue()
    {
        m_Stop.store(false,std::memory_order_relaxed);
        m_Thread = std::thread(&ThreadQueue::run,this);
    }

    ~ThreadQueue()
    {
        m_Stop.store(true,std::memory_order_relaxed);
        m_CV.notify_one();
    }

    bool empty()
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        return m_TaskQue.empty();
    }

    std::size_t size()
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        return m_TaskQue.size();
    }

    void addTask(const std::function<void()>&& task)
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        bool isEmpty = this->m_TaskQue.empty();
        m_TaskQue.emplace(std::move(task));
        lock.unlock();

        //仅在队列为空的情况下才唤醒线程
        if(isEmpty)
            m_CV.notify_one();
    }

    void run()
    {
        while (!m_Stop.load(std::memory_order_relaxed))
        {
            if(this->empty())
            {
                std::unique_lock<std::mutex> lock(m_Mutex);
                m_CV.wait(lock,[this](){return !m_TaskQue.empty();});
            }
            else
            {
                std::unique_lock<std::mutex> lock(m_Mutex);
                std::function<void()> task = std::move(m_TaskQue.front());
                m_TaskQue.pop();
                lock.unlock();

                task();
            }
        }
    }
private:
    std::queue<std::function<void()>> m_TaskQue;
    std::thread m_Thread;
    std::mutex m_Mutex;
    std::condition_variable m_CV;
    std::atomic<bool> m_Stop;
};

/**
 * @brief The ThreadPool class
 */
class ThreadPool
{
public:
    enum Distribution{
        Ordered,//按线程池顺序依次分配任务
        Balanced//按线程池任务分布情况均匀地将任务分配给线程
    };

    ThreadPool(unsigned size = 0)
    {
        if(size >  std::thread::hardware_concurrency() || size == 0)
            size = std::thread::hardware_concurrency();
        for(unsigned i = 0; i < size; i++)
        {
            m_Threads.push_back(new ThreadQueue());
        }
        m_CurrentThread = m_Threads.begin();
    }

    ///启动一个后台任务,返回值是一个与std::packaged_task相关联的future,当传入的函数抛出异常时异常会被保存到future中,因此不会对线程池的while循环造成破坏
    ///对future调用get()等同于同步执行任务,当前线程会阻塞直到后台任务完成并获取返回值
    ///不对future调用get()等同于异步执行任务,当前线程会继续向下执行并忽视返回值
    template<Distribution mode = Ordered,typename Func,typename...Args,typename ReturnType = typename MetaUtility::FunctionTraits<Func>::ReturnType>
    std::future<ReturnType> run(Func func,Args&&...args)
    {
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::bind(func,std::forward<Args>(args)...));
        std::future<ReturnType> future = task->get_future();
        add<mode>( [task](){(*task)();} );
        return future;
    }

private:
    template<Distribution Mode>
    typename std::enable_if<Mode == Ordered>::type add(std::function<void()> && task)
    {
        (*m_CurrentThread)->addTask(std::move(task));

        ++m_CurrentThread;
        if(m_CurrentThread == m_Threads.end()){
            m_CurrentThread = m_Threads.begin();
        }
    }

    template<Distribution Mode>
    typename std::enable_if<Mode == Balanced>::type add(std::function<void()> && task)
    {
        std::map<std::size_t,ThreadQueue*,std::less<std::size_t>> queMap;
        std::vector<ThreadQueue*>::const_iterator it = m_Threads.cbegin();
        while (it != m_Threads.cend()) {
            queMap.emplace((*it)->size(),*it);
            ++it;
        }
        ThreadQueue*  t = (*queMap.begin()).second;
        t->addTask(std::move(task));
    }

private:
    std::vector<ThreadQueue*> m_Threads;
    std::vector<ThreadQueue*>::iterator m_CurrentThread;
};

#endif // THREADPOOL_H
