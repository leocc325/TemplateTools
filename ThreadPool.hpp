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
    using TimePoint = std::chrono::time_point<std::chrono::system_clock,std::chrono::nanoseconds>;

    friend class ThreadPool;
private:
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

    ThreadQueue(const ThreadQueue&) = delete ;

    ThreadQueue(ThreadQueue&& other) = delete ;

    ThreadQueue& operator = (const ThreadQueue&) = delete ;

    ThreadQueue& operator = (ThreadQueue&&) = delete ;

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

    //这个函数只能在ThreadQueue对象刚刚创建还没有开始执行任务的时候调用,否则目标对象other没有对任务队列加锁,是不安全的行为
    //这个函数仅仅用来代替移动构造时转移任务队列
    void moveTasks(ThreadQueue& other)
    {
          std::unique_lock<std::mutex> lock(m_Mutex);
          other.m_TaskQue = std::move(this->m_TaskQue);
    }

    /**
     * @brief occupied
     * @return
     */
    bool occupied() const noexcept
    {
        bool occupyFlag = false;
        //如果起始时间大于结束时间,说明任务正在执行,如果执行时间大于10s,则进一步认为线程池被占用了,否则说明线程正在闲置
        if(m_Start > m_End)
        {
            std::chrono::nanoseconds interval = std::chrono::system_clock::now() - m_Start;
            double time = std::chrono::duration_cast<std::chrono::duration<double,std::ratio<1,1000>>>(interval).count();

            occupyFlag = time > 10*1000;
        }
        return occupyFlag;
    }

    /**
     * @brief isIdle 线程处于闲置状态,可以删除
     * @return
     */
    bool isIdle()
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        return m_TaskQue.empty() && (m_End > m_Start);
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

                m_Start = std::chrono::system_clock::now();
                task();
                m_End = std::chrono::system_clock::now();
            }
        }
    }

private:
    std::queue<std::function<void()>> m_TaskQue;
    std::thread m_Thread;
    std::mutex m_Mutex;
    std::condition_variable m_CV;
    std::atomic<bool> m_Stop;
    TimePoint m_Start;
    TimePoint m_End;
};

/**
 * @brief The ThreadPool class
 *
 *如果在添加新的任务的时候有线程池被占用了,就创建一个新的线程池,然后将被占用线程池中的任务移动到新的线程池中
 *如果在添加新的任务时,检测到之前被占用的线程池现在已经闲置了,就释放掉多余的线程池,使活跃的线程池数量尽量和CPU核心数保持一致
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
    /**
     *按线程队列在线程池中的顺序依次添加新的任务
     */
    template<Distribution Mode>
    typename std::enable_if<Mode == Ordered>::type add(std::function<void()> && task)
    {
        (*m_CurrentThread)->addTask(std::move(task));

        ++m_CurrentThread;
        if(m_CurrentThread == m_Threads.end()){
            m_CurrentThread = m_Threads.begin();
        }
    }

    /**
     *按线程队列持有的任务数量添加新的任务,优先将任务派发给持有任务少的线程队列
     */
    template<Distribution Mode>
    typename std::enable_if<Mode == Balanced>::type add(std::function<void()> && task)
    {
        std::map<std::size_t,ThreadQueue*,std::less<std::size_t>> queMap;
        std::vector<ThreadQueue*>::const_iterator it = m_Threads.cbegin();

        while (it != m_Threads.cend())
        {
            queMap.emplace((*it)->size(),*it);
            ++it;
        }
        ThreadQueue*  t = (*queMap.begin()).second;
        t->addTask(std::move(task));
    }

    void moveThreadQueue(ThreadQueue* from)
    {
        ThreadQueue* to = new ThreadQueue();
        from->moveTasks(*to);
    }

private:
    std::vector<ThreadQueue*> m_Threads;
    std::vector<ThreadQueue*>::iterator m_CurrentThread;
};

#endif // THREADPOOL_H
