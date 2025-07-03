#ifndef MESSAGEWAPPER_H
#define MESSAGEWAPPER_H

#include "TemplateTools/FunctionTraits.hpp"
#include "TemplateTools/StringConvertorQ.hpp"
#include <any>
#include <stdexcept>
#include <functional>
#include <future>
#include <QDebug>
#include <QStringList>

#define USE_MEMPOOL 1

#if USE_MEMPOOL
#include "TemplateTools/MemoryPool.hpp"
#endif

using namespace MetaUtility;

//使用消息id获取返回值类型
template <std::size_t N>
struct MessageReturnType{using type = void;};

template <>
struct MessageReturnType<std::numeric_limits<std::size_t>::max()>{using type = void;};

//在excel中填写函数的返回值类型，然后用python脚本自动生成MessageReturnType模板用来获取函数的返回值
#define megRegister(msg, func) \
template <> \
struct MessageReturnType<msg>{using type = func;};

class MessageWapper
{
    template<typename Func>
    struct Manager
    {
        using Ret = typename FunctionTraits<Func>::ReturnType;
        using ArgsTuple = typename FunctionTraits<Func>::BareTupleType;
        static constexpr unsigned arity = FunctionTraits<Func>::Arity;

        static void setMsgArgs(void* input,void* argsTuple)
        {
            //需要在外面确保两个指针的类型一致,否则会UB
            resetTuple(argsTuple);
            *static_cast<ArgsTuple*>(argsTuple) = *static_cast<ArgsTuple*>(input);
        }

        static void setStringArgs(const QStringList& argsList,void* argsTuple)
        {
            if(arity != argsList.count()){
                qDebug()<<QString("error:argments number mismatch,%1/%2").arg(arity).arg(argsList.count());
                return;
            }

            resetTuple(argsTuple);
            TupleHelper<arity - 1, ArgsTuple>::set(*static_cast<ArgsTuple*>(argsTuple), argsList);
        }

        static void deleteMembers(void* args,void* ret)
        {
            deleter(static_cast<ArgsTuple*>(args));
            deleter(static_cast<Ret*>(ret));
        }
        //有内存池的情况下可以将这里改为内存池分配空间
        static void initMembers(void*& args,const std::type_info*& argsInfo,void*& ret,const std::type_info*& retInfo)
        {
            init<ArgsTuple>(args,argsInfo);
            init<Ret>(ret,retInfo);
        }

        template<typename T> static typename std::enable_if<!std::is_void<T>::value>::type
        deleter(T* ptr)
        {
            if(ptr != nullptr)
            {
#if USE_MEMPOOL
                MemoryPool::GlobalPool->deallocate(ptr);
#else
                ptr->~T();
                ::operator delete(ptr);
#endif
                ptr = nullptr;
            }
        }

        template<typename T> static typename std::enable_if<std::is_void<T>::value>::type
        deleter(T* ptr)
        {
            if(ptr != nullptr)
            {
#if USE_MEMPOOL
                MemoryPool::GlobalPool->deallocate(ptr);
#else
                ::operator delete(ptr);
#endif
                ptr = nullptr;
            }
        }

        template<typename T> static typename std::enable_if<!std::is_void<T>::value>::type
        init(void*& ptr,const std::type_info*& info)
        {
            if(ptr == nullptr)
            {
#if USE_MEMPOOL
                ptr = MemoryPool::GlobalPool->allocate<T>();
#else
                ptr = static_cast<T*>(::operator new(sizeof (T)));
#endif
                info = &typeid(T);
            }
        }

        template<typename T> static typename std::enable_if<std::is_void<T>::value>::type
        init(void*& ptr,const std::type_info*& info)
        {
            if(ptr == nullptr)
            {
                info = &typeid(T);
            }
        }

        static void resetTuple(void* tpl)
        {
            if(tpl == nullptr)
            {
#if USE_MEMPOOL
                tpl = MemoryPool::GlobalPool->allocate<ArgsTuple>();
#else
                tpl = ::operator new(sizeof (ArgsTuple));
#endif
            }
            else
            {
                static_cast<ArgsTuple*>(tpl)->~ArgsTuple();
            }

            ::new (tpl) ArgsTuple();
        }
    };

    void (*msgArgsHelper)(void*,void*) = nullptr;
    void (*scpiArgsHelper)(const QStringList&,void*) = nullptr;
    void (*initHelper)(void*&,const std::type_info*&,void*&,const std::type_info*&) = nullptr;
    void (*deleteHelper)(void*,void*) = nullptr;

public:
    MessageWapper() = delete ;

    MessageWapper(const MessageWapper&) = delete ;

    MessageWapper(MessageWapper&&) = delete ;

    MessageWapper& operator = (const MessageWapper&) = delete ;

    MessageWapper& operator = (MessageWapper&&) = delete ;

    template<typename Func,typename Obj = typename FunctionTraits<Func>::Class,
             typename Enable = typename std::enable_if<std::is_same<Obj,typename FunctionTraits<Func>::Class>::value>::type>
    MessageWapper(Func func,Obj* obj = nullptr)
    {
        msgArgsHelper = &Manager<Func>::setMsgArgs;
        scpiArgsHelper = &Manager<Func>::setStringArgs;
        deleteHelper = &Manager<Func>::deleteMembers;
        initHelper = &Manager<Func>::initMembers;

        initHelper(argsTuple,argTupleInfo,result,resultInfo);

        functor = [this,func,obj](){
            callHelper(func,obj);
        };
    }

    ~MessageWapper()
    {
        deleteHelper(argsTuple,result);
    }

    template<typename...Args>
    void setMsgArgs(Args&&...args)
    {
        using Tuple = typename std::tuple<typename std::remove_cv_t<typename std::remove_reference_t<Args>>...>;
        if(typeid (Tuple) != *argTupleInfo)
        {
            throw std::invalid_argument("Argument type or number mismatch");
        }
        Tuple tpl = std::make_tuple(std::forward<Args>(args)...);
        this->msgArgsHelper(&tpl,argsTuple);
    }

    void setScpiArgs(const QStringList& args)
    {
        this->scpiArgsHelper(args,argsTuple);
    }

    template<typename...Args>
    void callByMsg(Args&&...args)
    {
        setMsgArgs(std::forward<Args>(args)...);
        functor();
    }

    //这里不能使用重载来调用call，因为call在使用模板传参的时候可能会出现单个参数且参数类型是QStringList的时候，这种情况下会导致参数转发错误，所以需要给这两个函数改名
    void callByScpi(const QStringList& strList)
    {
        setScpiArgs(strList);
        functor();
    }

    template<std::size_t Index,typename RT = typename MessageReturnType<Index>::type>
    typename std::enable_if<!std::is_void<RT>::value,RT>::type getResult()
    {
        if(typeid(RT) != *resultInfo)
        {
            throw std::invalid_argument("return value type mismatch");
        }

        return *static_cast<RT*>(result);
    }

    template<std::size_t Index,typename RT = typename MessageReturnType<Index>::type>
    typename std::enable_if<std::is_void<RT>::value>::type getResult(){}

    QString getResultString() const noexcept
    {
        return resultString;
    }

private:
    template<int Index, typename Tuple>
    struct TupleHelper;

    template<typename Tuple>
    struct TupleHelper<-1, Tuple>
    {
        static void set(Tuple& , const QStringList& ) {}
    };

    template<int Index, typename Tuple>
    struct TupleHelper
    {
        static void set(Tuple& tpl, const QStringList& strList)
        {
            if  (Index < strList.size())
            {
                convertStringToArg(strList.at(Index),std::get<Index>(tpl));
                TupleHelper<Index - 1, Tuple>::set(tpl, strList);
            }
            else
            {
                throw std::out_of_range("Index out of range for QStringList");
            }
        }
    };

    ///调用成员函数
    template<typename Func,typename Obj>
    typename std::enable_if<std::is_member_function_pointer<Func>::value>::type
    callHelper(Func func,Obj* obj)
    {
        callImpl(func,obj,std::make_index_sequence<FunctionTraits<Func>::Arity>{});
    }

    ///调用非成员函数
    template<typename Func,typename Obj>
    typename std::enable_if<!std::is_member_function_pointer<Func>::value>::type
    callHelper(Func func,Obj*)
    {
        callImpl(func,std::make_index_sequence<FunctionTraits<Func>::Arity>{});
    }

    template<typename Func,std::size_t... Index,
             typename Ret = typename FunctionTraits<Func>::ReturnType,
             typename Tuple = typename FunctionTraits<Func>::BareTupleType>
    typename std::enable_if<std::is_void<Ret>::value>::type
    callImpl(Func func,std::index_sequence<Index...>)
    {
        Tuple* tpl = static_cast<Tuple*>(argsTuple);
        (func)(std::get<Index>(std::forward<Tuple>(*tpl))...);
    }

    template<typename Func,std::size_t... Index,
             typename Ret = typename FunctionTraits<Func>::ReturnType,
             typename Tuple = typename FunctionTraits<Func>::BareTupleType>
    typename std::enable_if<!std::is_void<Ret>::value>::type
    callImpl(Func func,std::index_sequence<Index...>)
    {
        Tuple* tpl = static_cast<Tuple*>(argsTuple);
        Ret value = (func)(std::get<Index>(std::forward<Tuple>(*tpl))...);
        resultString = convertArgToString(value);
        (*static_cast<Ret*>(result)) = value;
    }

    template<typename Func,typename Obj,std::size_t... Index,
             typename Ret = typename FunctionTraits<Func>::ReturnType,
             typename Tuple = typename FunctionTraits<Func>::BareTupleType>
    typename std::enable_if<std::is_void<Ret>::value>::type
    callImpl(Func func,Obj* obj,std::index_sequence<Index...>)
    {
        Tuple* tpl = static_cast<Tuple*>(argsTuple);
        (obj->*func)(std::get<Index>(std::forward<Tuple>(*tpl))...);
    }

    template<typename Func,typename Obj,std::size_t... Index,
             typename Ret = typename FunctionTraits<Func>::ReturnType,
             typename Tuple = typename FunctionTraits<Func>::BareTupleType>
    typename std::enable_if<!std::is_void<Ret>::value>::type
    callImpl(Func func,Obj* obj,std::index_sequence<Index...>)
    {
        Tuple* tpl = static_cast<Tuple*>(argsTuple);
        Ret value = (obj->*func)(std::get<Index>(std::forward<Tuple>(*tpl))...);
        resultString = convertArgToString(value);
        (*static_cast<Ret*>(result)) = value;
    }

private:
    std::function<void()> functor;

    void* argsTuple = nullptr;
    const std::type_info* argTupleInfo = nullptr;

    void* result = nullptr;
    const std::type_info* resultInfo = nullptr;

    QString resultString;
};

//临时定义的模板和宏，用于单元测试
#define MSG_FUNC1 1
#define MSG_FUNC2 2
#define MSG_FUNC3 3
#define MSG_FUNC4 4
#define MSG_FUNC5 5
#define MSG_FUNC6 6
#define MSG_FUNC7 7

template <std::size_t N>
struct MessageRT{using type = void;};

template <>
struct MessageRT<MSG_FUNC3>{using type = double;};

template <>
struct MessageRT<MSG_FUNC4>{using type = int;};

template <>
struct MessageRT<MSG_FUNC5>{using type = int;};

class TestClass
{
public:
    TestClass(){qDebug()<<" new TestClass ";}
    ~TestClass(){qDebug()<<" delete TestClass ";}
    void func1(){qDebug()<<"TestClass::func1: ";}
    void func2(int a,double b){qDebug()<<"TestClass::func2: "<<a<<b;}
    double func3(double a){qDebug()<<"TestClass::func3: "<<a;return a;}
    static int func4(TestClass& obj){qDebug()<<"TestClass::func4: ";return obj.value;}
    int operator () (int a, int b){qDebug()<<"TestClass::operator()(int,int): "<<a<<b;return a+b;}

    friend std::ostream& operator << (std::ostream& os,const TestClass& obj)
    {
        return os;
    }

    friend std::istream& operator >> (std::istream& is,TestClass& obj)
    {
        return is;
    }

private:
    int value = 30;
};

inline void testFunc(int a,int b,double c)
{
    qDebug()<<"testFunc: "<<a<<b<<c;
}

inline TestClass* testFunc2(int a,TestClass*)
{
    qDebug()<<"testFunc: "<<a;
}

inline void testUnit()
{
    TestClass* t = MemoryPool::GlobalPool->allocate<TestClass>();
    MemoryPool::GlobalPool->deallocate(t);

    TestClass* obj = new TestClass();

    //测试MessageWapper构造函数的默认参数
    //MessageWapper msg_001(testFunc,obj);//类型推导错误，编译器报错
    //MessageWapper msg_002(&TestClass::func1,obj);//类型推导正确,编译通过
    //MessageWapper msg_003(&TestClass::func1);//类型推导正确,编译通过

    MessageWapper msg_004(testFunc);
    //msg_004.callByMsg(10,20);//传入错误数量的参数,抛出异常
    //msg_004.callByMsg(10,20.5,30.5);//传入类型错误的参数,抛出异常
    msg_004.callByMsg(10,20,20.5);//传入正确的参数,输出testFunc:  10 20 20.5
    int a = 10;
    int& b = a;
    msg_004.callByMsg(a,b,20.5);//传入正确的参数,输出testFunc:  10 20 20.5

    {
      MessageWapper ms(testFunc2);
    }
    {
      MessageWapper ms(testFunc2);
    }
    MessageWapper msg_005(testFunc2);


    msg_005.callByMsg(10,obj);
    msg_005.callByMsg(10,obj);
    msg_005.callByMsg(10,obj);
    msg_005.callByMsg(10,obj);

    //测试返回值为void和非void的函数
    //测试普通函数、成员函数、static函数、lambda、std::function、callable
    //测试MSG调用变量传参
    //测试SCPI调用字符串传参
}

#endif // MESSAGEWAPPER_H
