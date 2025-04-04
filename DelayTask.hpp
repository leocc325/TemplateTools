#ifndef DELAYTASK_H
#define DELAYTASK_H

#include <sstream>
#include <thread>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <unordered_map>

#include <QObject>
#include <QThread>

class DelayTask:public QObject
{
    friend class DelayTaskProcessor;
private:
    template<typename Obj,typename Func>
    DelayTask(std::size_t delay,Obj* obj,Func func):
        m_HashValue(calculateHash(func,obj)),m_Delay(delay)
    {
        m_Running.store(false,std::memory_order_relaxed);
        m_Interrupt.store(false,std::memory_order_relaxed);
    }

    ~DelayTask();

    ///计算成员函数的hash值
    template <typename Class, typename ReturnType, typename... Args>
    static std::size_t calculateHash(ReturnType (Class::*ptr)(Args...),Class* obj)
    {
        union
        {
            ReturnType (Class::*memberFunc)(Args...);
            void* generalFunc;
        } converter;
        converter.memberFunc = ptr;
        std::stringstream ss;
        ss << std::hex <<reinterpret_cast<uintptr_t>(converter.generalFunc)<<obj;
        return std::hash<std::string>{}(ss.str());
    }

    ///获取这个类所持有的成员函数的hash值
    inline std::size_t getHashValue() const noexcept{
        return m_HashValue;
    }

    ///更新任务延时,更新时停止正在执行的任务,更新完成之后不重新启动线程,启动线程的任务应该由外部调用excute完成
    void changeDelay(std::size_t mseconds);

    ///根据延时设置执行传入的成员函数,如果线程正在执行,则刷新std::function,虽然这样会对效率产生一些影响,但是可以支持被调用函数所需参数的更新
    template<typename Func,typename Obj,typename...Args>
    void excute(Func func,Obj* obj,Args&&...args)
    {
        if(m_Delay == 0)
        {
            //如果延时为0,则直接执行任务函数
            std::bind(func,obj,std::forward<Args>(args)...)();
        }
        else
        {
            //如果延时不为0,则对当前线程执行状态进行判断
            //如果线程正在执行,则只对任务函数进行更新,如果线程没有执行,则更新任务函数,随后启动线程
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_Task = std::bind(func,obj,std::forward<Args>(args)...);

            if(!m_Running.load(std::memory_order_relaxed))
            {
                m_Running.store(true,std::memory_order_relaxed);
                m_Interrupt.store(false,std::memory_order_relaxed);
                m_Future = std::async(std::launch::async,&DelayTask::threadImpl,this);
#if (ATOMIC_INT_LOCK_FREE == 1)
    __GCC_ATOMIC_INT_LOCK_FREE;
    /**
     * 从这里可以看出ATOMIC_INT_LOCK_FREE在本地的实际值为1
     * std::future在本地编译会报错,因为future文件中#define ATOMIC_INT_LOCK_FREE > 1 为false导致future相关的代码被屏蔽
     * 但是在远程运行时打印出来的ATOMIC_INT_LOCK_FREE值为2,QT编译的moc文件中的__GCC_ATOMIC_INT_LOCK_FREE值也是2,不清楚为什么在IDE中会报错
     * 程序可以远程运行,可能是本地环境和远程环境不一致导致的
     **/
#endif
            }
        }
    }

    ///停止正在执行的后台任务
    void stop();

    ///后台任务所执行的函数
    void threadImpl();

private:
    const std::size_t m_HashValue;
    std::condition_variable m_CV;
    std::mutex m_Mutex;
    std::function<void()> m_Task;
    std::future<void> m_Future;
    std::size_t m_Delay;
    std::atomic<bool> m_Interrupt;
    std::atomic<bool> m_Running;
};

class DelayTaskProcessor
{
public:
    DelayTaskProcessor();
    ~DelayTaskProcessor();

    ///添加一个任务,默认延时为0ms,当延时为0时直接在调用处执行task,延时不为0时开启线程等待任务执行,并最终将任务移动到DelayTask对象所在线程
    template<typename Func,typename Obj>
    std::size_t addTask(std::size_t delay,Func func,Obj* obj)
    {
        DelayTask* task = new DelayTask(delay,obj,func);
        std::size_t hash = task->getHashValue();
        m_TaskMap.emplace(hash,task);
        return hash;
    }

    ///设置延时,延时改变时停止等待更新延时,随后按新的延时启动延时等待任务
    void setDelay(std::size_t id,std::size_t mseconds)
    {
        if(m_TaskMap.count(id))
        {
            m_TaskMap.at(id)->changeDelay(mseconds);
        }
    }

    ///设置延时,延时改变时停止等待更新延时,随后按新的延时启动延时等待任务
    template<typename Func,typename Obj>
    void setDelay(Func func,Obj* obj,std::size_t mseconds)
    {
        std::size_t id = DelayTask::calculateHash(func,obj);
        setDelay(id,mseconds);
    }

    ///如果interrupt为true则停止等待,否则什么都不做
    template<typename Func,typename Obj,typename...Args>
    void delayExcute(bool interrupt,Func func,Obj* obj,Args&&...args)
    {
        std::size_t id = DelayTask::calculateHash(func,obj);
        if(m_TaskMap.count(id))
        {
            DelayTask* task = m_TaskMap.at(id);
            if(interrupt)
            {
                task->stop();
            }
            else
            {
                task->excute(func,obj,std::forward<Args>(args)...);
            }
        }
    }

private:
    std::unordered_map<std::size_t,DelayTask*> m_TaskMap;
};

#endif // DELAYTASK_H
