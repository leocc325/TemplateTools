#ifndef THROTTLEPRIVATE_HPP
#define THROTTLEPRIVATE_HPP

#include <type_traits>
#include <functional>
#include <chrono>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <utility>
#include <list>
#include <sstream>

#include <QApplication>
#include <QThread>

///每一个函数都有一个与之对应的Throttle对象专门管理这个函数的调用
class AbstractThrottle : public QObject
{
    friend class Throttler;

protected:
    AbstractThrottle(std::size_t interval);
    virtual ~AbstractThrottle();
    virtual void processTask() = 0;

private:
    void addTask(std::function<void()>&& task,std::size_t interval);
    bool isEmpty();
    void taskThread();

protected:
    std::condition_variable m_CV;
    std::mutex m_Mutex;
    std::list<std::function<void()>> m_TaskQue;
    std::atomic<std::size_t> m_Interval{1000};
    std::atomic<bool> m_QuitFlag;
};

///同步节流,当SyncThrottle在多个线程中addTask时,最终会将任务转发至创建SyncThrottle对象时所在的线程
///因为事件循环的线程是在SyncThrottle对象被创建时确定的
class SyncThrottle:public AbstractThrottle
{
     friend class Throttler;
private:
    SyncThrottle(std::size_t interval);
    void processTask() override;
private:
    QThread* m_TargetThread = nullptr;
};

///异步节流
class AsyncThrottle:public AbstractThrottle
{
    friend class Throttler;
private:
    AsyncThrottle(std::size_t interval);
    void processTask() override;
};


#endif // THROTTLEPRIVATE_HPP
