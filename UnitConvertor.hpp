#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

#include  "UnitConvertorPrivate.hpp"
#include <QString>

namespace UnitConvertor
{
    ///将以From为单位的数值转换为以To为单位表示的数值
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,double>::type
    toValue(double value)
    {
        using RatioType = typename std::ratio_divide<From,To>::type;
        double newRatio = static_cast<double>(RatioType::num) / static_cast<double>(RatioType::den);
        return value * newRatio;
    }

    ///将以From为单位的数值转换为以To为单位表示的字符串
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,QString>::type
    toString(double value)
    {
        using RatioType = typename std::ratio_divide<From,To>::type;
        double newValue = toValue<From,To>(value);
        return QString("%1 %2%3").arg(QString::number(newValue)).arg(2).arg(3);
    }

    ///将数值自动转换为一个恰当单位表示的数值(1～999之间的值),函数内部递归
    ///如果递归导致ratio溢出或者参数已经是一个恰当的值则返回当前的值和ratio
    template<typename From>
    static typename std::enable_if<IsRatio<From>::value,ValuePack>::type
    toProperValue(double value)
    {
        if(value > 1000)//如果数据大于1000则将其除以1000并上调数量级
            return toProperValue<std::ratio_multiply<From,ExpKilo>>(value/1000);
        else if(value < 1)//如果数据小于1则将其乘以1000并上调数量级
            return toProperValue<std::ratio_divide<From,ExpKilo>>(value*1000);
        else//如果数据刚好在[1,1000)区间内,则认为这已经是一个恰当的值
            return ValuePack{value , EnumExpNum, UnitNum};
    }

    ///模板本身会无限地归导致std::ratio分子/分母溢出,所以这里需要特化模板专门处理溢出的情况
    template<> ValuePack toProperValue<ExpAtto>(double value)
    {
        return ValuePack{value , EnumExpNum, UnitNum};
    }

    template<> ValuePack toProperValue<ExpExa>(double value)
    {
        return ValuePack{value ,  EnumExpNum, UnitNum};
    }

    ///将数值自动转换为一个恰当单位表示的字符串
    template<typename From>
    static typename std::enable_if<IsRatio<From>::value,QString>::type
    toProperString(double value)
    {
        ValuePack pack = toProperValue<From>(value);
    }
};

#endif // UNITCONVERTOR_HPP
