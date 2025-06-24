#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

#include "UnitConvertorPrivate.hpp"

//*检查是否所有函数都是inline的

namespace UC = UnitConvertor;

namespace UnitConvertor
{
    ///将以From为数量级的数值转换为以To为数量级表示的数据包
    template<typename From,typename To>
    static typename std::enable_if<IsRatio<From>::value&&IsRatio<To>::value,ValuePack>::type
    fromNumeric(double value,UnitEnum unit = EnumNull)
    {
        using RatioType = typename std::ratio_divide<From,To>::type;
        double newRatio = static_cast<double>(RatioType::num) / static_cast<double>(RatioType::den);
        return ValuePack{value*newRatio,ExpToEnum<RatioType>::E,unit};
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
            return ValuePack{value,ExpToEnum<From>::E,unit};
    }

    ///模板本身会无限地归导致std::ratio分子/分母溢出,所以这里需要特化模板专门处理溢出的情况
    template<> ValuePack properNumeric<Atto>(double value,UnitEnum unit)
    {
        return ValuePack{value,ExpToEnum<Atto>::E,unit};
    }

    template<> ValuePack properNumeric<Exa>(double value,UnitEnum unit)
    {
        return ValuePack{value,ExpToEnum<Exa>::E,unit};
    }

    ///将字符串转换为以To为数量级表示的数据包,显示给出第二个参数(单位类型)可以提升函数效率
    template<typename To>
    static typename std::enable_if<IsRatio<To>::value,ValuePack>::type
    fromString(const std::string& valueString,UnitEnum unit = EnumNull)
    {
        //如果字符串为空则直接返回
        if(valueString.empty())
            return ValuePack();

        std::vector<std::string> vec = UC::split(valueString);
         //正确的字符串应该会被分割成两部分,如果超过两部分也按两部分处理
        if(vec.size() >= 2)
        {
            //将第一部分转换成数值
            double V = std::stod(vec.at(0));

            //将第二部分分割为指数量级和单位
            ExpEnum E = UC::findExp(vec.at(1));
            UnitEnum U = UC::findUnit(vec.at(1));

            using From = ExpFromEnum<E>::type;

            return ValuePack{V,E,U};
        }

        return ValuePack();
    }

#if USE_QSTRING
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
