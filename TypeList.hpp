#ifndef TYPELIST_H
#define TYPELIST_H

#include <tuple>

namespace MetaUtility{

    ///参数列表产生一个tuple类型
    template<typename...Types>
    struct TypeList
    {
        enum {Arity = sizeof... (Types)};
        using type = std::tuple<Types...>;
    };

    ///将新的Type添加到TypeList最后面
    template<typename List,typename T>
    struct AppendType;

    template<typename...Types,typename T>
    struct AppendType<TypeList<Types...>,T>
    {
        using type = TypeList<Types...,T>;
    };

    ///将新的Type添加到TypeList最前面
    template<typename List,typename T>
    struct PrependType;

    template<typename...Types,typename T>
    struct PrependType<TypeList<Types...>,T>
    {
        using type = TypeList<T,Types...>;
    };

    ///移除TypeList最前面的一个Type
    template<typename T>
    struct RemoveFirstType;

    template<typename Head,typename...Types>
    struct RemoveFirstType<TypeList<Head,Types...>>
    {
        using type = TypeList<Types...>;
    };

    /// 移除TypeList最后面的一个Type,空缺,无法通过直接定义一个模板实现此功能

    ///获取参数包中索引为N的参数类型
    template<size_t Index,typename...Types>
    struct GetNthType;

    template<size_t N,typename First,typename...Types>
    struct GetNthType<N,First,Types...>
    {
        static_assert ( N <= TypeList<First,Types...>::Arity, "N is larger than the size of ArgsPackage");
        using type = typename GetNthType<N-1,Types...>::type;
    };

    template<typename First,typename...Types>
    struct GetNthType<0,First,Types...>
    {
        using type = First;
    };

    ///获取TypeList索引为N以前的全部参数(包括索引为N的参数)
    template<size_t N,typename List,typename...Types>
    struct FrontArgsHelper
    {
        using NewType = typename GetNthType<N,Types...>::type;//将参数包中索引为N的参数取出来作为NewType
        using NewList = typename PrependType<List,NewType>::type;//将NewType添加到TypeList的最前面,生成一个新的TypeList
        using type = typename FrontArgsHelper<N-1,NewList,Types...>::type;//索引减1,将新的TypeList作为参数进行下一次递归
    };

    template<typename List,typename...Types>
    struct FrontArgsHelper<0,List,Types...>
    {
        using NewType = typename GetNthType<0,Types...>::type;//获取参数包中第一个参数类型
        using type = typename PrependType<List,NewType>::type;//将NewType添加到TypeList最前面,结束递归
    };

    ///获取参数包的最前面N个Type
    template<size_t N,typename...Types>
    struct FrontArgs
    {
        static_assert ( N <= sizeof... (Types) && N > 0 , "N is larger than the size of ArgsPackage or N is zero");
        using type = typename FrontArgsHelper<N - 1,TypeList<>,Types...>::type;
    };

    template<typename...Types>
    struct FrontArgs<0,Types...>
    {
        using type = TypeList<>;
    };

    ///获取TypeList索引为N以后的全部参数(包括索引为N的参数)
    template<size_t N,typename List>
    struct BehindArgsHelper
    {
        using NewList = typename RemoveFirstType<List>::type;
        using type = typename BehindArgsHelper<N-1,NewList>::type;
    };

    template<typename List>
    struct BehindArgsHelper<0,List>
    {
        using type =  List;
    };

    ///获取参数包的最后面N个Type
    template<size_t N,typename...Types>
    struct BehindArgs
    {
        static_assert (N <= sizeof... (Types) , "N is larger than the size of ArgsPackage or N is zero");
        enum {Index = sizeof... (Types) - N};
        using type = typename BehindArgsHelper<Index,TypeList<Types...>>::type;
    };

    template<typename...Types>
    struct BehindArgs<0,Types...>
    {
        using type = TypeList<>;
    };
}

#endif // TYPELIST_H
