# 这是一个FunctionWrapper.hpp的使用说明
这里是两个目标函数用于测试
```c++
double add(int a,double b)
{
    std::cout<<__PRETTY_FUNCTION__<<a<<b<<std::endl<<std::flush;
    return a + b;
}

class Object
{
public:
    double subtract(double a,int b)
    {
        std::cout<<__PRETTY_FUNCTION__<<a<<b<<std::endl<<std::flush;
        return a - b;
    }
};
```

## 一：关于类FunctionWrapper的成员函数和使用方法介绍


FunctionWrapper是一个函数包装器,可以将其视为一个***完全擦除了类型的std::function***,但是在类的内部保存了所有必要的信息,这些信息如下：<br />

### FunctionWrapper成员函数简介:<br />

#### 1.默认构造函数FunctionWrapper()
生成一个不包含函数指针和参数指针以及信息的的FunctionWrapper,这也意味着后续无法再设置其函数指针(移动构造和赋值除外) <br />
```c++
FunctionWrapper f;
//创建了一个不包含函数指针和参数包的FunctionWrapper;
```

#### 2.构造函数template<typename Func,typename Obj> FunctionWrapper(Func func,Obj* obj = nullptr)
这个构造函数接受一个函数指针作为参数,函数、参数等信息会在这个构造函数中进行初始化。<br />
```c++
FunctionWrapper funcB(add);
//创建了一个可以调用自由函数add(int,double)的FunctionWrapper

Object* obj = new Object();
FunctionWrapper funcC(&Object::subtract,obj);
//创建了一个可以调用成员函数Object::subtract的FunctionWrapper
```

### 3.template<typename...Args> setArgs(Args&&...args)
设置函数调用需要的参数,由于FunctionWrapper内部擦除了函数参数的类型信息(并非完全擦除,只是无法在这里获取,这也是FunctionWrapper的一个不完美的地方,但是确实想不到更好的解决办法了),所以为了确保后续对函数指针调用不发生异常,这里对传入参数的类型进行了非常严格的检查,不会对参数进行隐式转换,需要确保传入的每一个参数的类型和函数指针的参数完全一致,否则抛出异常。如果参数包为空,则setArgs函数内部什么都不会做。
```c++
//以funcB为例,内部的函数指针需要的参数类型分别为int,double

funcB.setArgs(5,10);
//10会被推导为int类型,和第二个参数double类型不符合,抛出异常

funcB.setArgs(5,double(10));
//两个参数类型都完全匹配,正确
```

#### 4.void exec()
exec()为调用保存的函数指针的最终接口,这个函数内部会将保存的参数转发到保存的函数指针并完成函数指针的调用。
如果FunctionWrapper内部没有包含函数指针,则什么都不会发生并且打印错误信息 函数指针信息 + :error:functor is empty,excute failed
如果FunctionWrapper内部设置函数参数,同样什么都不会发生并且打印错误信息 函数指针信息 + :error:no parameter has been setted,excute failed
```c++
//对于上方的FuncA,FuncB,FuncC分别调用exec()则会打印如下信息
funcA.exec();
//调用失败:输出Dn  error:functor is empty,excute failed
funcB.exec();
//调用失败:输出PFdidE  error:no parameter has been setted,excute failed
funcC.exec();
//调用失败:输出M6ObjectFddiE  error:no parameter has been setted,excute failed
funcC.exec(10.5,5);
//调用成功:输出double Object::subtract(double, int)10.55
```

#### 5.template<typename...Args> void exec(Args&&...args)
exec()的重载,相当于将args...设置为函数参数并且调用exec()

#### 6.template<typename...Args> void operator() (Args&&...args)
等同于template<typename...Args> void exec(Args&&...args)


## 二：一个完整的示例。

```c++

```
