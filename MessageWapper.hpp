#ifndef MESSAGEWAPPER_H
#define MESSAGEWAPPER_H

#include "TemplateTools/FunctionTraits.hpp"
#include "TemplateTools/StringConvertorQ.hpp"
#include <any>
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
    MemoryPool(std::size_t size)
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

//是不是可以使用消息id获取返回值类型
template <std::size_t N>
struct MessageReturnType{using type = void;};

//在excel中填写函数的返回值类型，然后用python脚本自动生成MessageReturnType模板用来获取函数的返回值
#define megRegister(msg, func) \
template <> \
struct MessageReturnType<msg>{using type = func;};

class MessageWapper
{
public:
    template<typename Func>
    MessageWapper(Func func)
    {
        using Tuple =typename FunctionTraits<Func>::BareTupleType;
        using Ret = typename FunctionTraits<Func>::ReturnType;
        argTupleInfo = &typeid(Tuple);
        resultInfo = &typeid (Ret);

        functor = [this,func]()
        {
            if(!argsList.isEmpty())
            {
                const unsigned arity = FunctionTraits<Func>::Arity;
                //判断字符串参数链表是否为空,不为空的情况下将字符串参数链表转换为tuple
                if(arity != argsList.count())
                {
                    qDebug()<<QString("error:argments number mismatch,%1/%2").arg(arity).arg(argsList.count());
                    return;
                }

                reconstruct<Tuple>();

                //将字符串参数转换为tuple,之后清除字符串参数链表
                TupleHelper<arity - 1, Tuple>::set(*static_cast<Tuple*>(argsTuple), argsList);
                argsList.clear();
            }

            std::promise<Ret> promise;
            std::future<Ret> future = promise.get_future();
            callHelper(promise,func,std::make_index_sequence<FunctionTraits<Func>::Arity>{});

        };
    }

    ~MessageWapper()
    {
        if(argsTuple != nullptr)
            ::operator delete(argsTuple);//直接调用operator delete会导致内存泄漏,因为没有执行类的析构函数
    }

    //将参数报转换成tuple(这里需要根据type_info判断tuple和函数参数构成的tuple类型是否一致，如果不一致则直接提示参数类型不匹配，函数参数组成的tuple的type_info在构造的时候保存为类成员变量)
    //然后再强制将tuple转换为void*作为类成员变量，最后在调用的时候通过函数指针类型提取将void*转换为对应的tuple，由此完成参数包类型擦除和转发
    template<typename...Args>
    void call(Args&&...args)
    {
        using Tuple = typename std::tuple<typename std::remove_cv_t<typename std::remove_reference_t<Args>>...>;
        if(typeid (Tuple) != *argTupleInfo)
        {
            qDebug()<<QString("error:argments number or type mismatch");
            return;
        }
        else
        {
            reconstruct<Tuple>(std::forward<Args>(args)...);
            functor();
        }
    }

    //这里不能使用重载来调用call，因为call在使用模板传参的时候可能会出现单个参数且参数类型是QStringList的时候，这种情况下会导致参数转发错误，所以需要给这两个函数改名
    void callStr(const QStringList& strList)
    {
        argsList = strList;
        functor();
    }

    //这里需要判断返回值是不是void，对于void返回值特化一个模板
    template<std::size_t Index,typename RT = typename MessageReturnType<Index>::type>
    RT get()
    {
        if(typeid(RT) != *resultInfo)
        {
            throw std::bad_cast();
        }

        return *static_cast<RT*>(result);
    }

private:
    template<typename Tuple,typename...Args>
    void* reconstruct(Args&&...args)
    {
        if(argsTuple == nullptr)
        {
            argsTuple = ::operator new(sizeof (Tuple));
        }
        else
        {
            static_cast<Tuple*>(argsTuple)->~Tuple();
        }

        return ::new (argsTuple) Tuple(std::forward<Args>(args)...);
    }

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

    template<typename Func,std::size_t... Index,
             typename Ret = typename FunctionTraits<Func>::ReturnType>
    void callHelper(std::promise<Ret>& promise,Func func,std::index_sequence<Index...>)
    {
        using Tuple =typename FunctionTraits<Func>::BareTupleType;
        Tuple* tpl = static_cast<Tuple*>(argsTuple);

        promise.set_value( (func)(std::get<Index>(std::forward<Tuple>(*tpl))...) );
    }

    template<typename Func,typename Obj,std::size_t... Index,
             typename Ret = typename FunctionTraits<Func>::ReturnType>
    void callHelper(std::promise<Ret>& promise,Func func,Obj* obj,std::index_sequence<Index...>)
    {
        using Tuple =typename FunctionTraits<Func>::BareTupleType;
        Tuple* tpl = static_cast<Tuple*>(argsTuple);

        promise.set_value( (obj->*func)(std::get<Index>(std::forward<Tuple>(*tpl))...) );
    }

private:
    std::function<void()> functor;

    QStringList argsList;

    void* argsTuple = nullptr;//这里需要void*抹除tuple类型
    const std::type_info* argTupleInfo = nullptr;//判断tuple类型是否一致

    void* result = nullptr;
    const std::type_info* resultInfo = nullptr;
};

#endif // MESSAGEWAPPER_H
