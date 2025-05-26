#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

#include <ratio>
#include <QString>
namespace UnitConvertor
{
    ///这里重新定义了标准库std::ratio别名的原因是为了增加表示个位数量级的类型
    using Atto   =   std::atto;
    using Femto  =   std::femto;
    using Pico   =   std::pico;
    using Nano   =   std::nano;
    using Micro  =   std::micro;
    using Milli  =   std::milli;
    using Centi  =   std::centi;
    using Deci   =   std::deci;
    using One    =   std::ratio<1,1>;//new add
    using Deca   =   std::deca;
    using Hecto  =   std::hecto;
    using Kilo   =   std::kilo;
    using Mega   =   std::mega;
    using Giga   =   std::giga;
    using Tera   =   std::tera;
    using Peta   =   std::peta;
    using Exa    =   std::exa;

    ///判断给定类型是否是std::ratio类型
    template<typename T>
    struct IsRatio:public std::false_type{};

    template<long int Num,long int Den>
    struct IsRatio<std::ratio<Num,Den>>:public std::true_type{};

    template<template<int,int> class R>
    struct ss:public std::false_type{};

    ///将数值转换为字符串

    ///将数值转换为另一个单位表示的数值

    ///将数值自动转换为一个恰当单位表示的字符串


    ///
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,void>::type
    toString(double value)
    {

    }
};

#endif // UNITCONVERTOR_HPP
