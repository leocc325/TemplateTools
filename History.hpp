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

using namespace MetaUtility;

namespace History {
    ///IsCopyConstructable用于判断这一组参数是否全部都能拷贝构造
    template<typename...Args>
    struct IsCopyConstructable{};

    template<>
    struct IsCopyConstructable<>
    {
        static constexpr bool value = true;
    };

    template<typename First,typename...Args>
    struct IsCopyConstructable<First,Args...>
    {
        static constexpr bool value = std::is_copy_constructible<First>::value && IsCopyConstructable<Args...>::value;
    };

    ///IsPassable检测参数数量和类型是否和目标函数匹配
    template<typename FirstFunc,typename SecondFunc,typename...Args>
    struct IsPassable
    {
        using FirFuncArgs = typename FunctionTraits<FirstFunc>::BareTupleType;
        using SecFuncArgs = typename FunctionTraits<SecondFunc>::BareTupleType;
        using FirPartArgs = typename FrontArgs<FunctionTraits<FirstFunc>::Arity,typename std::decay_t<Args>...>::type::type;
        using SecPartArgs = typename BehindArgs<FunctionTraits<SecondFunc>::Arity,typename std::decay_t<Args>...>::type::type;

        static constexpr bool value = std::is_same<FirFuncArgs,FirPartArgs>::value &&
                                                            std::is_same<SecFuncArgs,SecPartArgs>::value &&
                                                            IsCopyConstructable<typename std::decay<Args>::type...>::value;
    };

    class StateStack
    {
    public:
        StateStack(){}

        ///撤销
        void undo();

        ///重做
        void redo();

        ///undo和redo是同一个函数,且来自同一个对象
        template<typename Func,typename Obj,typename...Args>
        typename std::enable_if<IsPassable<Func,Func,Args...>::value,void>::type
        add(Func func,Obj* obj,Args&&...args);

        ///undo和redo不是同一个函数,但来自同一个对象
        template<typename Undo,typename Redo,typename Obj,typename...Args>
        typename std::enable_if<IsPassable<Undo,Redo,Args...>::value,void>::type
        add(Undo undoFunc,Redo redoFunc,Obj* obj,Args&&...args);

        ///undo和redo不是同一个函数,且来自于不同的对象
        template<typename Undo,typename UndoObj,typename Redo,typename RedoObj,typename...Args>
        typename std::enable_if<IsPassable<Undo,Redo,Args...>::value,void>::type
        add(Undo undoFunc,UndoObj* undoObj,Redo redoFunc,RedoObj* redoObj,Args&&...args);

    private:
        void update(std::function<void()>&& undoFunc,std::function<void()>&& redoFunc);

        ///传入元组
        template<typename Func, typename Obj, typename Tuple,size_t...N>
        void callHelper(Func func,Obj *obj,Tuple tpl,std::index_sequence<N...>)
        {
            (obj->*func)(std::get<N>(std::forward<Tuple>(tpl))...);
        }

        ///从一个元组中获取给定索引上的子元组
        template<size_t StartIndex,typename Tuple,size_t...Counts>
        auto getSubTuple(const Tuple& full,std::index_sequence<Counts...>)
        {
            return std::make_tuple(std::get<StartIndex+Counts>(full)...);
        }

    private:
        ///栈在压入和弹出的时候加锁,除此以外这个锁还有一个很重要的重用,就是防止undo/redo出现递归现象,详细说明见cpp文件#note1
        std::mutex m_StackMutex;

        ///状态栈,用于保存撤销动作,用deque而不使用stack的原因是:在栈满了的情况下继续添加对象,需要将最末位的(最早添加的)元素移除,并将最新的元素压入栈
        ///考虑问题:当注册的函数需要的参数为指针的时候是否存在风险？比如执行撤销的时候,这个指针已经在外部被删除
        std::deque<std::function<void()>> m_UndoStack;

        ///redo函数
        std::function<void()> m_RedoFunc;

        ///最大栈深度,以每个函数需要两个参数(int+double)为准,存10000个std::fucntion在栈中大约占用内存0.5M
        const std::size_t m_MaxSize = 10000;
    };

    static StateStack stack;

    template<typename Func,typename Obj,typename...Args>
    typename std::enable_if<IsPassable<Func,Func,Args...>::value,void>::type
    StateStack::add(Func func,Obj* obj ,Args&&...args)
    {
        constexpr int Arity = FunctionTraits<Func>::Arity;
        std::tuple<Args...> EntireTuple = std::make_tuple(std::forward<Args>(args)...);

        std::function<void()> undoFunc = [&]() mutable {
            typename FrontArgs<Arity,typename std::decay_t<Args>...>::type::type tpl = getSubTuple<0>(EntireTuple,std::make_index_sequence<Arity>{});//将参数类型退化为原始类型,以副本形式将参数保存到元组中
            callHelper(func,obj,tpl,std::make_index_sequence<Arity>{});
        };

        std::function<void()> redoFunc = [&]() mutable {
            typename BehindArgs<Arity,typename std::decay_t<Args>...>::type::type tpl = getSubTuple<sizeof... (Args) - Arity>(EntireTuple,std::make_index_sequence<Arity>{});//将参数类型退化为原始类型,以副本形式将参数保存到元组中
            callHelper(func,obj,tpl,std::make_index_sequence<Arity>{});
        };

        update(std::move(undoFunc),std::move(redoFunc));
    }

    template<typename Undo, typename Redo, typename Obj, typename...Args>
    typename std::enable_if<IsPassable<Undo,Redo,Args...>::value, void>::type
    StateStack::add(Undo undoFunc, Redo redoFunc, Obj *obj, Args&&...args)
    {
        constexpr int undoArity = FunctionTraits<Undo>::Arity;
        constexpr int redoArity = FunctionTraits<Redo>::Arity;
        std::tuple<Args...> EntireTuple = std::make_tuple(std::forward<Args>(args)...);

        std::function<void()> undo = [&]() mutable {
            typename FrontArgs<undoArity,typename std::decay_t<Args>...>::type::type tpl = getSubTuple<0>(EntireTuple,std::make_index_sequence<undoArity>{});//将参数类型退化为原始类型,以副本形式将参数保存到元组中
            callHelper(undoFunc,obj,tpl,std::make_index_sequence<undoArity>{});
        };

        std::function<void()> redo = [&]() mutable {
            typename BehindArgs<redoArity,typename std::decay_t<Args>...>::type::type tpl = getSubTuple<sizeof... (Args) - redoArity>(EntireTuple,std::make_index_sequence<redoArity>{});//将参数类型退化为原始类型,以副本形式将参数保存到元组中
            callHelper(redoFunc,obj,tpl,std::make_index_sequence<redoArity>{});
        };

        update(std::move(undo),std::move(redo));
    }

    template<typename Undo, typename UndoObj, typename Redo, typename RedoObj, typename...Args>
    typename std::enable_if<IsPassable<Undo,Redo,Args...>::value, void>::type
    StateStack::add(Undo undoFunc, UndoObj *undoObj, Redo redoFunc, RedoObj *redoObj,Args&&...args)
    {
        constexpr int undoArity = FunctionTraits<Undo>::Arity;
        constexpr int redoArity = FunctionTraits<Redo>::Arity;
        std::tuple<Args...> EntireTuple = std::make_tuple(std::forward<Args>(args)...);

        std::function<void()> undo = [&]() mutable {
            typename FrontArgs<undoArity,typename std::decay_t<Args>...>::type::type tpl = getSubTuple<0>(EntireTuple,std::make_index_sequence<undoArity>{});//将参数类型退化为原始类型,以副本形式将参数保存到元组中
            callHelper(undoFunc,undoObj,tpl,std::make_index_sequence<undoArity>{});
        };

        std::function<void()> redo = [&]() mutable {
            typename BehindArgs<redoArity,typename std::decay_t<Args>...>::type::type tpl = getSubTuple<sizeof... (Args) - redoArity>(EntireTuple,std::make_index_sequence<redoArity>{});//将参数类型退化为原始类型,以副本形式将参数保存到元组中
            callHelper(redoFunc,redoObj,tpl,std::make_index_sequence<redoArity>{});
        };

        update(std::move(undo),std::move(redo));
    }

    //这里补充:使用宏在这个函数前面添加一个默认的this参数，判断this对象是否继承于QWidget来启用模板，因为撤销/重做只针对UI操作
    template<typename Func,typename Obj,typename...Args>
    inline void store(Func func,Obj* obj,Args&&...args){
        History::stack.add(func,obj,std::forward<Args>(args)...);
    }

    template<typename Undo, typename Redo, typename Obj, typename...Args>
    inline void store(Undo undoFunc, Redo redoFunc, Obj *obj, Args&&...args){
        History::stack.add(undoFunc,redoFunc,obj,std::forward<Args>(args)...);
    }

    template<typename Undo, typename UndoObj, typename Redo, typename RedoObj, typename...Args>
    inline void store(Undo undoFunc, UndoObj *undoObj, Redo redoFunc, RedoObj *redoObj,Args&&...args){
        History::stack.add(undoFunc,undoObj,redoFunc,redoObj,std::forward<Args>(args)...);
    }

    inline void undo(){
        History::stack.undo();
    }

    inline void redo(){
        History::stack.redo();
    }
}

#endif // HISTORY_H
