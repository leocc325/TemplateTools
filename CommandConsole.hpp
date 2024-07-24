#ifndef COMMANDCONSOLE_HPP
#define COMMANDCONSOLE_HPP

#include <iostream>
#include <memory>
#include <functional>

#include <QHash>
#include <QString>
#include <QThread>
#include <QDebug>

#include "StringConvertorQ.hpp"
#include "FunctionTraits.hpp"

#define CmdConsole CommandConsole::getInstance()

using namespace MetaUtility;

/**
 * @brief The Console class
 * C++14和C++17版本均可使用,windows中的byte类型(rpcndr.h头文件内部使用了byte)和c++17 cstddef文件中定义的std::byte会产生编译冲突
 * 该冲突会在windows.h和std同时被引用的时候出现,不存在该冲突的情况下可自行选择C++运行版本
 *
 * 关于函数调用主要被分为4种类型:无返回值的普通函数,有返回值的普通函数,无返回值的成员函数,有返回值的成员函数
 * 为了兼容C++14 和 C++ 17,又为以上四种函数增加了两个版本下的不同调用实现,总共8种
 */
class CommandConsole
{
    template<typename Func>
    struct ReturnVoid
    {
        using RT = typename FunctionTraits<Func>::ReturnType;
        constexpr static bool value = std::is_same<RT,void>::value;
    };
public:
    enum CallType{DefaultThread,GuiThread,IndependentThread};

    static CommandConsole* getInstance();

    QString getReturn(){return returnValue;}

    QString getErrorInfo(){return errorInfo;}

    QString getHelpinfo(){return helpInfo;}

    QString handleCommand(const QString& cmd);

    ///四个参数分别为:指令,函数指针,指令备注,函数调用线程。当CallType为GuiThread时,被注册的函数一定会在GUI线程中被执行。
    template<typename Func>
    void registerFunction(const QString& command,Func func,const QString& cmdTips = "",CallType calltype= GuiThread)
    {
        //只有第一次出现的键值对会被添加到hash中，后续同样的键添加会失败
        if(funcHash.contains(command.toLower()))
            return;

        std::function<void()> function = [=](){
            callHelper(func);
        };

        funcHash.insert(command.toLower(),function);
        threadHash.insert(command.toLower(),calltype);
        //当指令备注为空时,不会在显示备注信息的时候显示该指令
        if(!cmdTips.isEmpty())
            helpInfo.append(command + ":" + cmdTips+ "\n");
    }

    ///五个参数分别为:指令,函数指针,对象指针,指令备注,函数调用线程。当CallType为GuiThread时,被注册的函数一定会在GUI线程中被执行。
    template<typename Func,typename Obj>
    void registerFunction(const QString& command,Func func,Obj* obj,const QString& cmdTips = "",CallType calltype= GuiThread)
    {
        //只有第一次出现的键值对会被添加到hash中，后续同样的键添加会失败
        if(funcHash.contains(command.toLower()))
            return;

        std::function<void()> function = [=](){
            callHelper(func, obj);
        };

        funcHash.insert(command.toLower(),function);
        threadHash.insert(command.toLower(),calltype);
        //当指令备注为空时,不会在显示备注信息的时候显示该指令
        if(!cmdTips.isEmpty())
            helpInfo.append(command + ":" + cmdTips+ "\n");
    }

private:
    CommandConsole(){}

    template<int Index, typename Tuple>
    struct TupleHelper;

    template<typename Tuple>
    struct TupleHelper<-1, Tuple>{
        static void set(Tuple& , const QStringList& ) {}
    };

    template<int Index, typename Tuple>
    struct TupleHelper{
        static void set(Tuple& tpl, const QStringList& strList)
        {
            if  (Index < strList.size())
            {
                convertStringToArg(strList.at(Index),std::get<Index>(tpl));
                TupleHelper<Index - 1, Tuple>::set(tpl, strList);
            }
            else
                throw std::out_of_range("Index out of range for QStringList");
        }
    };

    ///void非成员函数
    template<typename Func,typename Tuple>
    typename std::enable_if<ReturnVoid<Func>::value && !std::is_member_function_pointer<Func>::value>::type
    callImpl(Func func,Tuple&& argTpl)
    {
#if __cplusplus > 201402L
        std::apply(std::forward<Func>(func),std::forward<Tuple>(argTpl));
#else
        call(std::forward<Func>(func),std::forward<Tuple>(argTpl));
#endif
        returnValue = "void";
    }

    ///non void非成员函数
    template<typename Func,typename Tuple>
    typename std::enable_if<!ReturnVoid<Func>::value && !std::is_member_function_pointer<Func>::value>::type
    callImpl(Func func,Tuple&& argTpl)
    {
#if __cplusplus > 201402L
        auto ret = std::apply(std::forward<Func>(func),std::forward<Tuple>(argTpl));
#else
        auto ret = call(std::forward<Func>(func),std::forward<Tuple>(argTpl));
#endif
        returnValue = convertArgToString(ret);
    }

    ///void成员函数
    template<typename Obj,typename Func,typename Tuple>
    typename std::enable_if<ReturnVoid<Func>::value && std::is_member_function_pointer<Func>::value>::type
    callImpl(Obj obj,Func func,Tuple&& argTpl)
    {
#if __cplusplus > 201402L
        std::tuple<Obj> objTuple = std::make_tuple(obj);
        auto tpl = std::tuple_cat(objTuple,argTpl);
        std::apply(std::forward<Func>(func),tpl);
#else
        call(std::forward<Obj>(obj),std::forward<Func>(func),std::forward<Tuple>(argTpl));
#endif
        returnValue = "void";
    }

    ///non void成员函数
    template<typename Func,typename Obj,typename Tuple>
    typename std::enable_if<!ReturnVoid<Func>::value && std::is_member_function_pointer<Func>::value>::type
    callImpl(Obj obj,Func func,Tuple&& argTpl)
    {
#if __cplusplus > 201402L
        std::tuple<Obj> objTuple = std::make_tuple(obj);
        auto tpl = std::tuple_cat(objTuple,argTpl);
        auto ret = std::apply(std::forward<Func>(func),tpl);
#else
        auto ret = call(std::forward<Obj>(obj),std::forward<Func>(func),std::forward<Tuple>(argTpl));
#endif
        returnValue = convertArgToString(ret);
    }

    template<typename Func> void callHelper(Func func)
    {
        using DecayFunc = typename std::decay<Func>::type;
        using Tuple = typename FunctionTraits<DecayFunc>::BareTupleType ;

        int ArgsNum = FunctionTraits<DecayFunc>::Arity;
        if(ArgsNum != argStringList.count())
        {
            errorInfo = QString("error:argments number mismatch,required number:%1,input number:%2").arg(ArgsNum).arg(argStringList.count());
            return;
        }

        Tuple tpl;
        TupleHelper<FunctionTraits<DecayFunc>::Arity - 1, Tuple>::set(tpl, argStringList);

        callImpl(func,tpl);
        errorInfo = QString("call successed,return value:%1").arg(returnValue);
    }

    template<typename Func, typename Obj> void callHelper( Func func, Obj obj)
    {
        using DecayFunc = typename std::decay<Func>::type;
        using ArgsTuple = typename FunctionTraits<DecayFunc>::BareTupleType ;

        int ArgsNum = FunctionTraits<DecayFunc>::Arity;
        if(ArgsNum != argStringList.count())
        {
            errorInfo = QString("error:argments number mismatch,required number:%1,input number:%2").arg(ArgsNum).arg(argStringList.count());
            return;
        }

        ArgsTuple argsTuple;
        TupleHelper<FunctionTraits<DecayFunc>::Arity - 1, ArgsTuple>::set(argsTuple, argStringList);

        callImpl(obj,func,argsTuple);
        errorInfo = QString("call successed,return value:%1").arg(returnValue);
    }

    template<typename Func, typename Tuple>
    auto call(Func func, Tuple&& tpl)->typename FunctionTraits<typename std::remove_reference<Func>::type>::ReturnType
    {
        return functionHelper(std::forward<Func>(func),
                              std::forward<Tuple>(tpl),
                              std::make_index_sequence<FunctionTraits<typename std::decay<Func>::type>::Arity>{});
    }

    template<typename Obj, typename Func, typename Tuple>
    auto call(Obj obj, Func func, Tuple&& tpl)->typename FunctionTraits<typename std::remove_reference<Func>::type>::ReturnType
    {
        return functionHelper(std::forward<Obj>(obj),
                              std::forward<Func>(func), std::forward<Tuple>(tpl),
                              std::make_index_sequence<FunctionTraits<typename std::decay<Func>::type>::Arity>{});
    }

    template<typename Func, typename Tuple, std::size_t... Index>
    auto functionHelper(Func func, Tuple&& tpl, std::index_sequence<Index...>)->typename FunctionTraits<typename std::remove_reference<Func>::type>::ReturnType
    {
        return std::forward<Func>(func)(std::get<Index>(std::forward<Tuple>(tpl))...);
    }

    template<typename Obj, typename Func, typename Tuple, std::size_t... Index>
    auto functionHelper(Obj obj, Func func, Tuple&& tpl, std::index_sequence<Index...>)-> typename FunctionTraits<typename std::remove_reference<Func>::type>::ReturnType
    {
        return (obj->*func)(std::get<Index>(tpl)...);
    }

private:
    static CommandConsole* instance;

    //帮助信息
    QString helpInfo = "usable commands below:\n";
    //函数执行之后的返回值
    QString returnValue;
    //函数调用过程中的错误信息
    QString errorInfo;
    //用来传递参数的字符串链表
    QStringList argStringList;
    //函数注册映射表,键在被存入映射表的时候会被全部转换为小写
    QHash<QString,std::function<void()>> funcHash;
    //调用类型
    QHash<QString,CallType> threadHash;
};
#endif // COMMANDCONSOLE_HPP
