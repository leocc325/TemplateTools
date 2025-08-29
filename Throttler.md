# 这是一个Throttler.hpp的使用说明

## 一：关于类 Throttler 的成员函数和使用方法介绍
Throttler是一个节流器类,用于减少频繁大量的重复函数调用,例如快速采集的数据的计算、绘制图像、控制界面刷新频率等,减少系统资源消耗降低CPU负担。 <br />

### Throttler成员函数简介:<br />

#### 1.template<typename Func,typename...Args> static async(std::size_t time, Func func,Args&&...args)
对自由函数、静态成员函数、可调用对象执行异步节流,传入的函数在独立的线程中每间隔time毫秒执行一次。

#### 2.template<typename Func,typename Obj,typename...Args> static async(std::size_t time, Func func,Obj* obj,Args&&...args)
对成员函数执行异步节流,传入的函数在独立的线程中每间隔time毫秒执行一次。

#### 3.template<typename Func,typename...Args> static sync(std::size_t time,Func func,Args&&...args)
对自由函数、静态成员函数、可调用对象执行同步节流,每隔time毫秒时长执行一次func,func会被传递到第一次调用这个函数的事件循环中被执行。

#### 4.template<typename Func,typename Obj,typename...Args> static sync(std::size_t time, Func func,Obj* obj,Args&&...args)
对成员执行同步节流,每隔time毫秒时长执行一次func,func会被传递到第一次调用这个函数的事件循环中被执行。

### Throttler使用说明:<br />

同一个函数指:对于非成员函数,函数指针地址相同则被认为是同一个函数。对于成员函数,函数指针地址和调用这个对象的地址组成的字符串相同则认为是同一个成员函数。<br />
**可以在多个文件或函数中对同一个函数执行节流操作,但是不能将同一个函数同时标记为同步和异步,对同一个函数的节流只能选择一种节流方式,否则会导致程序异常**

假设存在以下的使用场景:某一台设备有多个通道需要不停的采集信号并将每一个通道采集的信号数据都绘制到屏幕上,但是数据的采集速度很快和数据量也比较大,绘图速度远远赶不上采集速度<br />
如果每采集一次数据都绘制一次图像,则会给系统带来相当沉重的负担,软件也会变得相当卡顿，这样的场景就可以使用节流器来控制界面图像的绘制和刷新。<br />

假设采集和绘制的函数分别如下:
```c++
class Channel
{
public:
    void draw(float* dataA,float* dataB,float* dataC)
    {
        ///draw  three wave graph
    }
    
    void sampleDataA()
    {
        ///sample data and save
    }
    
    void sampleDataB()
    {
        ///sample data and save
    }
    
    void sampleDataC()
    {
        ///sample data and save
    }
};


int main()
{
    Channel chan;
    
    ///...

    //in sample thread 1
    {
        while(true)
        {
            chan.sampleDataA();//采集数据
            Throttler::sync(500,&Channel::draw,&chan);
        }
    }
    
    //in sample thread 2
    {
        while(true)
        {
            chan.sampleDataB();//采集数据
            Throttler::sync(1000,&Channel::draw,&chan);
        }
    }
    
    //in sample thread 3
    {
        while(true)
        {
            chan.sampleDataC();//采集数据
            Throttler::sync(2000,&Channel::draw,&chan);
        }
    }
    
    ///....
}
```

假设上面代码中对数据指针的使用都是线程安全的,只要有新的数据被采集到我们就重新绘制三幅图像,那么我们就可以将代码编写成上述main函数中的样式,可以在多个地方对目标函数节流。<br />
但是上面在调用sync的时候指定的时间都是不一样的,那么最后真实的时间间隔是多少呢？<br />
答案是由率先被执行的那一句sync决定,如果是线程1中的sync最先被执行,那么节流器的时间间隔就是500ms,且永远不会再发生改变(同理,如果是线程2和线程3中的sync最先被执行,那么节流器的时间间隔就是1000ms和2000ms)。<br />
此后无论三个线程while循环执行多久,draw()函数都会500ms执行一次。<br />

## 二：类Throttler设计方案和原理
Throttler会对每一个***不同的函数***创建一个节流管理器***SyncThrottle***或者***AsyncThrottle***,这两个类被定义于头文件***ThrottlePrivate.hpp***中。<br />
通过Throttler的sync()和async()传递函数指针和参数时,会执行以下几件事:<br />

1.获取指针地址字符串,如果是成员函数指针,则根据函数指针地址和类对象地址创建地址字符串,如果是非成员函数指针则直接获取指针地址字符串,对于不同的函数他们的地址字符串一定是唯一的。<br />

2.将指针地址转换为hash值<br />

3.从hash表中查找是否已经存在这个函数指针的节流管理器,如果不存在,则为这个函数指针创建一个节流管理器,后续这个函数指针的所有调用都由这个节流器进行管理。<br />

4.节流器内部创建了一个线程和任务队列,在线程内间隔指定时间从任务队列中取出最新的任务执行函数调用并清空队列中积累的任务。<br />

5.将传入的函数指针封装成一个任务传递到节流管理器中等待执行。<br />

6.对于异步节流管理器,节流器会在时间间隔结束时直接在自己的线程中执行传递的任务。对于同步节流管理器,则会在时间间隔结束时将任务队列中最新的任务转发给这个任务所对应的线程或事件循环,由这个任务的事件循环决定函数的调用时机。<br />

7.当节流管理器中任务队列为空时,内部线程会休眠,直到新的任务被传递到队列中,此时线程被重新唤醒继续处理任务队列。<br />

### ***注：节流器会确保第一次和最后一次函数调用一定会被执行,中间的任务间隔指定时间执行一次*** <br>

## 三：一个完整的使用案例

```c++
void printIndex(int index)
{
    std::cout<<index<<std::endl<<std::flush;
}

void setIndex(int start, int nums)
{
    for(int i = start; i < start+nums; i++)
    {
        Throttler::async(500,printIndex,i);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

int main()
{
    setIndex(0,1);
    std::thread t1(setIndex,1,120);
    std::thread t2(setIndex,121,300);
    t1.join();
    t2.join();
    setIndex(301,1);

    return 0
}
```

一种可能的输出为:**0**、134、30、165、60、76、210、105、240、255、272、287、302、317、331、344、359、375、389、405、**301**。<br />
其中**0**和**301**一定会出现,因为他们分别是最先和最后被添加的任务,中间输出的数据则存在各种可能,因为这些任务是在两个线程中交替添加的。<br />
