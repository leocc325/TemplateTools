# 这是一个ThreadPool.hpp的使用说明

## 一：关于类 ThreadPool 的成员函数和使用方法介绍
ThreadPool是一个动态的线程池,可以根据当前线程池状态自动拓展、收缩以及任务队列转移,尽可能保证线程任务分配合理。 <br />

### ThreadPool使用说明:<br />

#### 1.构造函数ThreadPool(unsigned size = 0)
初始化一个大小为size的线程池,当size = 0时,线程池大小为CPU核心支持的最大线程数量,在默认状态下,线程池大小不会超过CPU核心数。<br />

### 2.成员函数template<Distribution Mode = Ordered,typename Func,typename...Args> 
###	      std::future<ReturnType> run(Func func,Args&&...args)
添加一个可执行任务,模板参数Mode表明添加策略:Ordered(按顺序添加)、Balanced(均衡线程池任务)。
```

```


## 四：一个完整的示例。

```c++

```
