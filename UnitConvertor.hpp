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

    ///将以From为单位的数值转换为以T为单位表示的数值
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,double>::type
    toValue(double value)
    {
        using RatioType = typename std::ratio_divide<From,To>::type;
        double newRatio = static_cast<double>(RatioType::num) / static_cast<double>(RatioType::den);
        return value * newRatio;
    }

    ///将以From为单位的数值转换为以T为单位表示的字符串
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,double>::type
    toString(double value)
    {

    }

    ///将数值自动转换为一个恰当单位表示的字符串
    template<typename From>
    static typename std::enable_if<IsRatio<From>::value,double>::type
    toProperString(double value)
    {
        using RatioType = typename std::ratio_divide<From,One>::type;
    }
};

#endif // UNITCONVERTOR_HPP
