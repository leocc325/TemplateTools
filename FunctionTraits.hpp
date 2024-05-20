#ifndef FUNCTIONTRAITS_H
#define FUNCTIONTRAITS_H

#include <functional>

namespace MetaUtility {

    ///基础模板
    template<typename T>
    struct FunctionTraits;

    ///普通函数
    template<typename RT,typename...Args>
    struct FunctionTraits<RT(Args...)>
    {
        enum {Arity = sizeof... (Args)};
        using ReturnType = RT;
        using FunctionType = RT(Args...);
        using StlFunction = std::function<FunctionType>;
        using Pointer = ReturnType(*)(Args...);
        using TupleType = typename std::tuple<Args...>;
        using BareTupleType = typename std::tuple<typename std::remove_cv_t<typename std::remove_reference_t<Args>>...>;

        template<size_t I>
        struct FunctionArgs
        {
            static_assert(I < Arity, "index is out of range, index must less than sizeof Args");
            using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
        };
    };

    ///函数指针
    template<typename RT,typename...Args>
    struct FunctionTraits<RT(*)(Args...)>:FunctionTraits<RT(Args...)>{};

    ///std::function
    template<typename RT,typename...Args>
    struct FunctionTraits<std::function<RT(Args...)>>:FunctionTraits<RT(Args...)>{};

    ///普通成员函数
    template<typename RT,typename ClassType,typename...Args>
    struct FunctionTraits<RT(ClassType::*)(Args...)> :FunctionTraits<RT(Args...)>{};

    ///const成员函数
    template<typename RT,typename ClassType,typename...Args>
    struct FunctionTraits<RT(ClassType::*)(Args...) const> :FunctionTraits<RT(Args...)>{};

    ///volatile成员函数
    template<typename RT,typename ClassType,typename...Args>
    struct FunctionTraits<RT(ClassType::*)(Args...) volatile> :FunctionTraits<RT(Args...)>{};

    ///const volatile成员函数
    template<typename RT,typename ClassType,typename...Args>
    struct FunctionTraits<RT(ClassType::*)(Args...) const volatile> :FunctionTraits<RT(Args...)>{};

    ///函数对象
    template<typename Callable>
    struct FunctionTraits:FunctionTraits<decltype (&Callable::operator())>{};

}

#endif // FUNCTIONTRAITS_H
