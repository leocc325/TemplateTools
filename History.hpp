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
    ///获取原始参数类型
    template<typename...Args>
    using RawArgs = typename std::decay<Args...>::type;

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
        using FirPartArgs = typename FrontArgs<FunctionTraits<FirstFunc>::Arity,Args...>::type;
        using SecPartArgs = typename BehindArgs<FunctionTraits<SecondFunc>::Arity,Args...>::type;

        static constexpr bool value = std::is_same<FirFuncArgs,FirPartArgs>::value && std::is_same<SecFuncArgs,SecPartArgs>::value;
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
        void add(Func func,Obj* obj,Args&&...args);

        ///undo和redo不是同一个函数,但来自同一个对象
        template<typename Undo,typename Redo,typename Obj,typename...Args>
        void add(Undo undoFunc,Redo redoFunc,Obj* obj,Args&&...args);

        ///undo和redo不是同一个函数,且来自于不同的对象
        template<typename Undo,typename UndoObj,typename Redo,typename RedoObj,typename...Args>
        void add(Undo undoFunc,UndoObj* undoObj,Redo redoFunc,RedoObj* redoObj,Args&&...args);

    private:
        ///栈在压入和弹出的时候加锁,除此以外这个锁还有一个很重要的重用,就是防止undo/redo出现递归现象,详细说明见cpp文件#note1
        std::mutex m_StackMutex;

        ///状态栈,用于保存撤销动作,用deque而不使用stack的原因是:在栈满了的情况下继续添加对象,需要将最末位的(最早添加的)元素移除,并将最新的元素压入栈
        ///考虑问题:当注册的函数需要的参数为指针的时候是否存在风险？比如执行撤销的时候,这个指针已经在外部被删除
        std::deque<std::function<void()>> m_Stack;

        ///最大栈深度
        const std::size_t m_MaxSize = 3000;
    };

    static StateStack stack;

    template<typename Func,typename Obj,typename...Args>
    void StateStack::add(Func func,Obj* obj ,Args&&...args)
    {
        //lambda表达式在以=捕获参数的时候会将他们作为const副本捕获
        //而args与Args参数类型不配的时候，将导致参数无法被转发,mutable可以使捕获到的参数为非const
        std::function<void()> undoFunc = [=]() mutable {
            (obj->*func)(std::forward<Args...>(args...));
        };

        if(m_StackMutex.try_lock())
            return;

        if(m_Stack.size() >= m_MaxSize)
        {
            //当栈已经满了的情况下,删除最末端的元素
            m_Stack.pop_back();
        }

        m_Stack.push_front(undoFunc);
        m_StackMutex.unlock();
    }

    //这里补充:使用宏在这个函数前面添加一个默认的this参数，判断this对象是否继承于QWidget来启用模板，因为撤销/重做只针对UI操作
    template<typename Func,typename Obj,typename...Args>
    inline void store(Func func,Obj* obj,Args&&...args)
    {
        History::stack.add(func,obj,std::forward<Args...>(args...));
    }

    inline void undo()
    {
        History::stack.undo();
    }

    inline void redo()
    {
        History::stack.redo();
    }
}

#endif // HISTORY_H
