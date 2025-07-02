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

using namespace MetaUtility;

/**
 * @brief The MemoryPool class : 专门为消息包装器定制的内存池
 * 这是一个简陋的内存池,是为了代替::operator new分配地址,确保给生命周期伴随整个程序运行的小对象分配的内存是连续的,减少内存散布
 * 内存池会返回一个地址,为了避免内存频繁分配和回收,依然需要在外部配合placement new使用
 */
class MemoryPool
{
public:
    MemoryPool(std::size_t size = 1024)
    {
        length = size;
        allocateNewBlock();
    }

    ~MemoryPool()
    {
        std::vector<char*>::iterator it = poolVec.begin();
        while (it != poolVec.end())
        {
            ::operator delete(*it);
            ++it;
        }
    }

    template<typename T,typename...Args>
    T* allocate(Args&&...args)
    {
        if(sizeof(T) > length)
            return nullptr;

        if(offset + sizeof (T) > length)
            allocateNewBlock();

        if(pool != nullptr)
        {
            void* address = pool + offset;
            offset += sizeof (T);
            return ::new (address) T(std::forward<Args>(args)...);
        }
    }

    template<typename T>
    void deallocate(T* ptr) const noexcept
    {
        if(ptr)
        {
            ptr->~T();
        }
    }

private:
    void allocateNewBlock()
    {
        char* buf = static_cast<char*>(::operator new(length));
        if(buf)
        {
            pool = buf;
            offset = 0;
            poolVec.push_back(buf);
        }
    }

private:
    std::vector<char*> poolVec;
    char* pool = nullptr;
    std::size_t length = 0;
    std::size_t offset = 0;
};

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
                ptr->~T();
                ::operator delete(ptr);
                ptr = nullptr;
            }
        }

        template<typename T> static typename std::enable_if<std::is_void<T>::value>::type
        deleter(T* ptr)
        {
            if(ptr != nullptr)
            {
                ::operator delete(ptr);
                ptr = nullptr;
            }
        }

        template<typename T> static typename std::enable_if<!std::is_void<T>::value>::type
        init(void*& ptr,const std::type_info*& info)
        {
            if(ptr == nullptr)
            {
                ptr =  static_cast<T*>(::operator new(sizeof (T)));
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
                tpl = ::operator new(sizeof (ArgsTuple));
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
            qDebug()<<typeid (Tuple).name()<<argTupleInfo->name();
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

inline void testFunc(int a,int b,double c)
{
    qDebug()<<"testFunc: "<<a<<b<<c;
}

class TestClass
{
public:
    void func1(){qDebug()<<"TestClass::func1: ";}
    void func2(int a,double b){qDebug()<<"TestClass::func2: "<<a<<b;}
    double func3(double a){qDebug()<<"TestClass::func3: "<<a;return a;}
    static int func4(TestClass& obj){qDebug()<<"TestClass::func4: ";return obj.value;}
    int operator () (int a, int b){qDebug()<<"TestClass::operator()(int,int): "<<a<<b;return a+b;}
private:
    int value = 30;
};

inline void testUnit()
{
    qDebug()<<sizeof (std::type_info) ;
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

    //测试返回值为void和非void的函数
    //测试普通函数、成员函数、static函数、lambda、std::function、callable
    //测试MSG调用变量传参
    //测试SCPI调用字符串传参
}

#endif // MESSAGEWAPPER_H
