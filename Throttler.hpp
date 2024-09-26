#ifndef THROTTLER_HPP
#define THROTTLER_HPP

#include <unordered_map>
#include "ThrottlePrivate.hpp"

class ThrottleError:public std::exception
{
public:
    ThrottleError(const char* info);

    ThrottleError(const std::string& info);

    ThrottleError(std::string&& info);

    virtual const char * what() const noexcept;

private:
    std::string errorString;
};

class Throttler
{
    template<typename Func>
    using RawFunc = typename std::remove_reference<std::remove_cv_t<Func>>::type;

    template<typename Func>
    struct IsFreeFunc:std::false_type{};

    template<typename ReturnType,typename...Args>
    struct IsFreeFunc<ReturnType(*)(Args...)>:std::true_type{};

    template<typename Func>
    struct IsMemberFunction:std::false_type{};

    template<typename ClassType,typename ReturnType,typename...Args>
    struct IsMemberFunction<ReturnType(ClassType::*)(Args...)>:std::true_type{};

    template<typename Func>
    struct IsStdFunction:std::false_type{};

    template<typename ReturnType,typename...Args>
    struct IsStdFunction<std::function<ReturnType(Args...)>>:std::true_type{};
    
    ///如果C++版本大于14,就使用17中自带的std::void_t,否则使用自定义的VoidType
    template<typename...T>
#if __cplusplus > 201402L
    using VoidType = std::void_t<T...>;
#else
    using VoidType = void;
#endif

    template<typename,typename = VoidType<>>
    struct IsCallable:std::false_type{};
    
    template<typename T>
    struct IsCallable<T,VoidType<decltype(&T::operator())>>:std::true_type{};

public:
    ///传入的函数会在单独的线程中执行,不建议调用GUI相关的函数
    template<typename Func,typename...Args>
    static typename std::enable_if<IsFreeFunc<RawFunc<Func>>::value || IsStdFunction<RawFunc<Func>>::value || IsCallable<Func>::value>::type
    async(std::size_t time, Func func,Args&&...args)
    {
        AsyncThrottle* t = funcMapCheck<AsyncThrottle,Func>(time,func);
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        t->addTask(std::move(task));
    }

    ///传入的函数会在单独的线程中执行,不建议调用GUI相关的函数
    template<typename Func,typename Obj,typename...Args>
    static typename std::enable_if<IsMemberFunction<RawFunc<Func>>::value>::type
    async(std::size_t time, Func func,Obj* obj,Args&&...args)
    {
        AsyncThrottle* t = funcMapCheck<AsyncThrottle,Func,Obj>(time,func,obj);
        std::function<void()> task = std::bind(std::forward<Func>(func),obj,std::forward<Args>(args)...);
        t->addTask(std::move(task));
    }

    ///捕获时间间隔内最后一次被调用的函数和参数，并将其交还给捕获动作发生所在的线程，被捕获的函数会在其自身线程的下一次事件循环中被调用
    template<typename Func,typename...Args>
    static typename std::enable_if<IsFreeFunc<Func>::value || IsStdFunction<Func>::value || IsCallable<Func>::value>::type
    sync(std::size_t time,Func func,Args&&...args)
    {
        SyncThrottle* t = funcMapCheck<SyncThrottle,Func>(time,func);
        std::function<void()> task = std::bind(std::forward<Func>(func),std::forward<Args>(args)...);
        t->addTask(std::move(task));
    }

    ///捕获时间间隔内最后一次被调用的函数和参数，并将其交还给捕获动作发生所在的线程，被捕获的函数会在其自身线程的下一次事件循环中被调用
    template<typename Func,typename Obj,typename...Args>
    static typename std::enable_if<IsMemberFunction<Func>::value>::type
    sync(std::size_t time, Func func,Obj* obj,Args&&...args)
    {
        SyncThrottle* t = funcMapCheck<SyncThrottle,Func,Obj>(time,func,obj);
        std::function<void()> task = std::bind(std::forward<Func>(func),obj,std::forward<Args>(args)...);
        t->addTask(std::move(task));
    }

private:
    ///获取自由函数指针的地址,对于函数指针,通过获取函数指针地址的方式来获取唯一的标识符
    template <typename ReturnType,typename...Args>
    static std::string getFunctionAddress(ReturnType(*ptr)(Args...))
    {
        std::stringstream ss;
        ss << std::hex <<reinterpret_cast<uintptr_t>(ptr);
        return ss.str();
    }

    ///获取成员函数指针的地址,对于函数指针,通过获取函数指针和类对象地址的方式来获取唯一的标识符
    template <typename Class, typename ReturnType, typename... Args>
    static std::string getFunctionAddress(ReturnType (Class::*ptr)(Args...),Class* obj)
    {
        union
        {
            ReturnType (Class::*memberFunc)(Args...);
            void* generalFunc;
        } converter;
        converter.memberFunc = ptr;
        std::stringstream ss;
        ss << std::hex <<reinterpret_cast<uintptr_t>(converter.generalFunc)<<obj;
        return ss.str();
    }

    ///获取std::function地址(typeinfo),对于std::function,通过获取std::function的operator的typeinfo来获取唯一标识符
    template <typename ReturnType,typename...Args>
    static std::string getFunctionAddress(const std::function<ReturnType(Args...)>& func)
    {
        std::stringstream ss;
        ss << func.target_type().name();
        return ss.str();
    }

    ///获取callable地址(typeinfo),对于callable,通过获取typeinfo来获取唯一标识符
    template <typename Callable>
    static std::string getFunctionAddress(const Callable& functor)
    {
        std::stringstream ss;
        ss << typeid (functor).name();
        return ss.str();
    }


    ///检查hash表中是否已经存在自由函数Func的管理者,如果不存在则创建,最后返回Func对应的管理者
    template<typename ThrottleType,typename Func>
    static ThrottleType* funcMapCheck(std::size_t time,Func func)
    {
        std::string funcAddress = getFunctionAddress(func);
        std::size_t funcHash = std::hash<std::string>{}(funcAddress);

        {
            std::lock_guard<std::mutex> locker(mapMutex);
            if(!funcMap.count(funcHash)){
                funcMap.emplace(funcHash,new ThrottleType(time));
            }
        }

        ThrottleType* ptr = dynamic_cast<ThrottleType*>(funcMap.at(funcHash));
        if(!ptr)
        {
            std::string funcInfo = "Throttler error:cannot bind function to sync<Func,Args...>\
                                    and async<Func,Args...> at the same time\n";
            funcInfo.append("Function:").append(typeid(func).name()).append("\n");
            throw ThrottleError(funcInfo);
        }
        return ptr;
    }

    ///检查hash表中是否已经存在成员函数Func的管理者,如果不存在则创建,最后返回Func对应的管理者
    template<typename ThrottleType,typename Func,typename Obj>
    static ThrottleType* funcMapCheck(std::size_t time,Func func,Obj* obj)
    {
        std::string funcAddress = getFunctionAddress(func,obj);
        std::size_t funcHash = std::hash<std::string>{}(funcAddress);

        {
            std::lock_guard<std::mutex> locker(mapMutex);
            if(!funcMap.count(funcHash)){
                funcMap.emplace(funcHash,new ThrottleType(time));
            }
        }

        ThrottleType* ptr = dynamic_cast<ThrottleType*>(funcMap.at(funcHash));
        if(!ptr)
        {
            std::string funcInfo = "Throttler error:cannot bind function to sync<Func,Obj,Args...>\
                                    and async<Func,Obj,Args...> at the same time\n";
            funcInfo.append("Function:").append(typeid(func).name()).append("\n");
            funcInfo.append("Class:").append(typeid(obj).name());
            throw ThrottleError(funcInfo);
        }
        return ptr;
    }

private:
    static std::unordered_map<std::size_t,AbstractThrottle*> funcMap;
    static std::mutex mapMutex;
};

#endif // THROTTLER_HPP
