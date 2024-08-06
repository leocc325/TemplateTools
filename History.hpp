#ifndef HISTORY_H
#define HISTORY_H

#include <tuple>
#include <stack>
#include <mutex>
#include <functional>
#include <type_traits>

#include "FunctionTraits.hpp"
#include "TypeList.hpp"

/**
 * @brief The History class
 */
/*
#define STOREHISTORY(...) std::stringstream ss ;\
ss << std::hex <<__PRETTY_FUNCTION__<<this;\
const std::string id = ss.str();\
History::store(id,__VA_ARGS__);
*/
using namespace MetaUtility;

class History{
    ///IsCopyConstructable用于判断这一组参数是否全部都能拷贝构造
    template<typename...Args>
    struct IsCopyConstructable;

    ///获取参数包前N个参数对应的元组,同时将参数类型退化为原始类型,以副本形式将参数保存到元组中
    template<size_t N,typename...Args>
    using FrontTuple = typename FrontArgs<N,typename std::decay_t<Args>...>::type::type;

    ///获取参数包后N个参数对应的元组,同时将参数类型退化为原始类型,以副本形式将参数保存到元组中
    template<size_t N,typename...Args>
    using BehindTuple = typename BehindArgs<N,typename std::decay_t<Args>...>::type::type;

    ///IsPassable检测参数数量和类型是否和目标函数匹配
    template<typename FirstFunc,typename SecondFunc,typename...Args>
    struct IsPassable
    {
        using FirFuncArgs = typename FunctionTraits<FirstFunc>::BareTupleType;
        using SecFuncArgs = typename FunctionTraits<SecondFunc>::BareTupleType;
        using FirPartArgs = FrontTuple<FunctionTraits<FirstFunc>::Arity,Args...>;
        using SecPartArgs = BehindTuple<FunctionTraits<SecondFunc>::Arity,Args...>;

        static constexpr int inputArity = FunctionTraits<FirstFunc>::Arity + FunctionTraits<SecondFunc>::Arity;
        static constexpr bool value = (inputArity == sizeof... (Args)) &&//传入参数数量是否等于两个函数的参数数量的和
                                                            std::is_same<FirFuncArgs,FirPartArgs>::value &&//第一部分的参数数量、类型是否和第一个函数的参数数量、类型相同
                                                            std::is_same<SecFuncArgs,SecPartArgs>::value &&//第二部分的参数数量、类型是否和第二个函数的参数数量、类型相等
                                                            IsCopyConstructable<typename std::decay<Args>::type...>::value;//传入的参数是否全部都支持拷贝构造
    };

public:
    ///撤销
    static void undo();

    ///重做
    static void redo();

    ///undo和redo是同一个函数,且来自同一个对象
    template<typename Func,typename Obj,typename...Args>
    static typename std::enable_if<IsPassable<Func,Func,Args...>::value,void>::type
    store(Func func,Obj* obj,Args&&...args)
    {
        constexpr int Arity = FunctionTraits<Func>::Arity;
        std::tuple<std::decay_t <Args>...> EntireTuple = std::make_tuple(std::forward<Args>(args)...);//退化参数类型，以副本的形式将参数保存到元组

        std::function<void()> undoFunc = [=]() mutable {
            FrontTuple<Arity,Args...> tpl = getSubTuple<0>(EntireTuple,std::make_index_sequence<Arity>{});
            callHelper(func,obj,tpl,std::make_index_sequence<Arity>{});
        };

        std::function<void()> redoFunc = [=]() mutable {
            BehindTuple<Arity,Args...> tpl = getSubTuple<sizeof... (Args) - Arity>(EntireTuple,std::make_index_sequence<Arity>{});
            callHelper(func,obj,tpl,std::make_index_sequence<Arity>{});
        };

        update(std::move(undoFunc),std::move(redoFunc));
    }

    ///undo和redo不是同一个函数,但来自同一个对象
    template<typename Undo,typename Redo,typename Obj,typename...Args>
    static typename std::enable_if<IsPassable<Undo,Redo,Args...>::value,void>::type
    store(Undo undoFunc,Redo redoFunc,Obj* obj,Args&&...args)
    {
        constexpr int undoArity = FunctionTraits<Undo>::Arity;
        constexpr int redoArity = FunctionTraits<Redo>::Arity;
        std::tuple<std::decay_t <Args>...> EntireTuple = std::make_tuple(std::forward<Args>(args)...);

        std::function<void()> undo = [=]() mutable {
            FrontTuple<undoArity,Args...> tpl = getSubTuple<0>(EntireTuple,std::make_index_sequence<undoArity>{});
            callHelper(undoFunc,obj,tpl,std::make_index_sequence<undoArity>{});
        };

        std::function<void()> redo = [=]() mutable {
            BehindTuple<redoArity,Args...> tpl = getSubTuple<sizeof... (Args) - undoArity>(EntireTuple,std::make_index_sequence<redoArity>{});
            callHelper(redoFunc,obj,tpl,std::make_index_sequence<redoArity>{});
        };

        update(std::move(undo),std::move(redo));
    }

    ///undo和redo不是同一个函数,且来自于不同的对象
    template<typename Undo,typename UndoObj,typename Redo,typename RedoObj,typename...Args>
    static typename std::enable_if<IsPassable<Undo,Redo,Args...>::value,void>::type
    store(Undo undoFunc,UndoObj* undoObj,Redo redoFunc,RedoObj* redoObj,Args&&...args)
    {
        constexpr int undoArity = FunctionTraits<Undo>::Arity;
        constexpr int redoArity = FunctionTraits<Redo>::Arity;
        std::tuple<std::decay_t <Args>...> EntireTuple = std::make_tuple(std::forward<Args>(args)...);

        std::function<void()> undo = [=]() mutable {
            FrontTuple<undoArity,Args...> tpl = getSubTuple<0>(EntireTuple,std::make_index_sequence<undoArity>{});
            callHelper(undoFunc,undoObj,tpl,std::make_index_sequence<undoArity>{});
        };

        std::function<void()> redo = [=]() mutable {
            BehindTuple<redoArity,Args...> tpl = getSubTuple<sizeof... (Args) - undoArity>(EntireTuple,std::make_index_sequence<redoArity>{});
            callHelper(redoFunc,redoObj,tpl,std::make_index_sequence<redoArity>{});
        };

        update(std::move(undo),std::move(redo));
    }

private:
    static void update(std::function<void()>&& undoFunc,std::function<void()>&& redoFunc);

    ///传入元组
    template<typename Func, typename Obj, typename Tuple,size_t...N>
    static void callHelper(Func func,Obj *obj,Tuple tpl,std::index_sequence<N...>){
        (obj->*func)(std::get<N>(std::forward<Tuple>(tpl))...);
    }

    ///从一个元组中获取给定索引上的子元组
    template<size_t StartIndex,typename Tuple,size_t...Counts>
    static auto getSubTuple(const Tuple& full,std::index_sequence<Counts...>){
        return std::make_tuple(std::get<StartIndex+Counts>(full)...);
    }

private:
    ///栈在压入和弹出的时候加锁,除此以外这个锁还有一个很重要的重用,就是防止undo/redo出现递归现象,详细说明见cpp文件#note1
    static std::mutex m_StackMutex;

    ///状态栈,用于保存撤销动作,用deque而不使用stack的原因是:在栈满了的情况下继续添加对象,需要将最末位的(最早添加的)元素移除,并将最新的元素压入栈
    static std::deque<std::function<void()>> m_UndoStack;

    ///redo函数
    static std::function<void()> m_RedoFunc;

    ///最大栈深度,以每个函数需要两个参数(int+double)为准,存10000个std::fucntion在栈中实际大约占用内存1.4M,理论值应该是0.5M左右
    static const std::size_t m_MaxSize = 10000;

    ///在撤销或重做时禁止入栈的flag
    static bool m_HistoryFlag;
};

template<>
struct History::IsCopyConstructable<>{
    static constexpr bool value = true;
};

template<typename First,typename...Args>
struct History::IsCopyConstructable<First,Args...>{
    static constexpr bool value = std::is_copy_constructible<First>::value && IsCopyConstructable<Args...>::value;
};

#endif // HISTORY_H
