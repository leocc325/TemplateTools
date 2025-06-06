#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

//#define USE_QSTRING

#ifdef USE_QSTRING
#include <QStringList>
#endif
#include "UnitConvertorPrivate.hpp"

namespace UC = UnitConvertor;

namespace UnitConvertor
{
    struct ValuePack
    {
        operator double () {
            return value;
        };

        operator std::string() {
            return std::to_string(value)+std::string(Exps[exp])+std::string(Units[unit]);
        };

#ifdef USE_QSTRING
        operator QString() {
            return QString("%1 %2%3").arg(value).arg(Exps[exp]).arg(Units[unit]);
        };
#endif

        double value  = 20;
        ExpEnum exp   = EnumExpNum;
        UnitEnum unit = EnumUnitNum;
    };

    ///将以From为单位的数值转换为以To为单位表示的数据包
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,ValuePack>::type
    fromNumeric(double value,UnitEnum unit = EnumNull)
    {
        using RatioType = typename std::ratio_divide<From,To>::type;
        double newRatio = static_cast<double>(RatioType::num) / static_cast<double>(RatioType::den);
        return ValuePack{value*newRatio,ExpHelper<RatioType>::E,unit};
    }

    ///将数值自动转换为一个恰当单位表示的数值(1～999之间的值),函数内部递归
    ///如果递归导致ratio溢出或者参数已经是一个恰当的值则返回当前的值和ratio
    template<typename From>
    static typename std::enable_if<IsRatio<From>::value,ValuePack>::type
    properNumeric(double value,UnitEnum unit = EnumNull)
    {
        if(value > 1000)//如果数据大于1000则将其除以1000并上调数量级
            return properNumeric<std::ratio_multiply<From,Kilo>>(value/1000,unit);
        else if(value < 1)//如果数据小于1则将其乘以1000并上调数量级
            return properNumeric<std::ratio_divide<From,Kilo>>(value*1000,unit);
        else//如果数据刚好在[1,1000)区间内,则认为这已经是一个恰当的值
            return ValuePack{value,ExpHelper<From>::E,unit};
    }

    ///模板本身会无限地归导致std::ratio分子/分母溢出,所以这里需要特化模板专门处理溢出的情况
    template<> ValuePack properNumeric<Atto>(double value,UnitEnum unit)
    {
        return ValuePack{value,ExpHelper<Atto>::E,unit};
    }

    template<> ValuePack properNumeric<Exa>(double value,UnitEnum unit)
    {
        return ValuePack{value,ExpHelper<Exa>::E,unit};
    }

    ///将字符串转换为以To为单位表示的数据包,显示给出第二个参数(单位类型)可以提升函数效率
    template<typename To>
    static typename std::enable_if<IsRatio<To>::value,ValuePack>::type
    fromString(const std::string& valueString,UnitEnum unit = EnumNull)
    {
        //如果字符串为空则直接返回
        if(valueString.empty())
            return ValuePack{0,ExpHelper<One>::E,unit};

        //去除字符串中多余的空格,并且将英文字符转换为小写
        std::string s = simplified(valueString);
    }

#ifdef USE_QSTRING
    ///将字符串转换为以To为单位表示的数据包,显示给出第二个参数(单位类型)可以提升函数效率
    template<typename To>
    static typename std::enable_if<IsRatio<To>::value,ValuePack>::type
    fromString(const QString& valueString,UnitEnum unit = EnumNull)
    {

    }

    ///将字符串自动转换为一个恰当单位表示的数值(1～999之间的值),函数内部递归
    ///如果递归导致ratio溢出或者参数已经是一个恰当的值则返回当前的值和ratio
    template<typename From>
    static typename std::enable_if<IsRatio<From>::value,ValuePack>::type
    properString(const QString& value)
    {

    }
#endif
};

#endif // UNITCONVERTOR_HPP
