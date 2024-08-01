#ifndef THROTTLER_HPP
#define THROTTLER_HPP

#include <unordered_map>
#include "ThrottlePrivate.hpp"

class Throttler
{
public:
    static Throttler* getInstance()
    {
        if(instance == nullptr)
            instance = new Throttler();
        return instance;
    }

    ///传入的函数会在单独的线程中执行,不建议调用GUI相关的函数
    template<typename Func,typename Obj,typename...Args>
    static typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    async(std::size_t time, Func func,Obj* obj,Args&&...args)
    {
        AsyncThrottle* t = funcMapCheck<AsyncThrottle,Func,Obj>(time,func,obj);
        t->call(func,obj,std::forward<Args>(args)...);
    }

    ///传入的函数会在单独的线程中执行,不建议调用GUI相关的函数
    template<typename Func,typename...Args>
    static typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    async(std::size_t time, Func func,Args&&...args)
    {
        AsyncThrottle* t = funcMapCheck<AsyncThrottle,Func>(time,func);
        t->call(func,std::forward<Args>(args)...);
    }

    ///捕获时间间隔内最后一次被调用的函数和参数，并将其交还给捕获动作发生所在的线程，被捕获的函数会在其自身线程的下一次事件循环中被调用
    template<typename Func,typename Obj,typename...Args>
    static typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    sync(std::size_t time, Func func,Obj* obj,Args&&...args)
    {
        AsyncThrottle* t = funcMapCheck<SyncThrottle,Func,Obj>(time,func,obj);
        t->call(func,obj,std::forward<Args>(args)...);
    }

    ///捕获时间间隔内最后一次被调用的函数和参数，并将其交还给捕获动作发生所在的线程，被捕获的函数会在其自身线程的下一次事件循环中被调用
    template<typename Func,typename...Args>
    static typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    sync(std::size_t time,Func func,Args&&...args)
    {
        AsyncThrottle* t = funcMapCheck<SyncThrottle,Func>(time,func);
        t->call(func,std::forward<Args>(args)...);
    }

    ///移除对成员函数的节流控制
    template<typename Func,typename Obj>
    static typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    remove(Func func,Obj* obj)
    {
        std::string funcAddress = getFunctionAddress(func,obj);
        std::size_t funcHash = std::hash<std::string>{}(funcAddress);
        if(funcMap.count(funcHash))
            funcMap.erase(funcHash);
    }

    ///移除对自由函数的节流控制
    template<typename Func>
    static typename std::enable_if<IsFunctionPointer<Func>::value && ReturnVoid<Func>::value>::type
    remove(Func func)
    {
        std::string funcAddress = getFunctionAddress(func);
        std::size_t funcHash = std::hash<std::string>{}(funcAddress);
        if(funcMap.count(funcHash))
            funcMap.erase(funcHash);
    }

private:
    Throttler(){}

    ///获取自由函数指针的地址
    template <typename ReturnType,typename...Args>
    static std::string getFunctionAddress(ReturnType(*ptr)(Args...))
    {
        std::stringstream ss;
        ss << std::hex <<reinterpret_cast<uintptr_t>(ptr);
        return ss.str();
    }

    ///获取成员函数指针的地址
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

    ///检查hash表中是否已经存在自由函数Func的管理者,如果不存在则创建,最后返回Func对应的管理者
    template<typename ThrottleType,typename Func>
    static ThrottleType* funcMapCheck(std::size_t time,Func func)
    {
        std::string funcAddress = getFunctionAddress(func);
        std::size_t funcHash = std::hash<std::string>{}(funcAddress);

        if(!funcMap.count(funcHash)){
            funcMap.emplace(funcHash,new ThrottleType(time));
        }

        ThrottleType* ptr = dynamic_cast<ThrottleType*>(funcMap.at(funcHash));
        return ptr;
    }

    ///检查hash表中是否已经存在成员函数Func的管理者,如果不存在则创建,最后返回Func对应的管理者
    template<typename ThrottleType,typename Func,typename Obj>
    static ThrottleType* funcMapCheck(std::size_t time,Func func,Obj* obj)
    {
        std::string funcAddress = getFunctionAddress(func,obj);
        std::size_t funcHash = std::hash<std::string>{}(funcAddress);

        if(!funcMap.count(funcHash)){
            funcMap.emplace(funcHash,new ThrottleType(time));
        }

        ThrottleType* ptr = dynamic_cast<ThrottleType*>(funcMap.at(funcHash));
        return ptr;
    }

private:
    static Throttler* instance;

    static std::unordered_map<std::size_t,AbstractThrottle*> funcMap;
};

#endif // THROTTLER_HPP
