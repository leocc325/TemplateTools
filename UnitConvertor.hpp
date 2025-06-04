#ifndef UNITCONVERTOR_HPP
#define UNITCONVERTOR_HPP

#include <ratio>
#include <QString>
namespace UnitConvertor
{
    namespace
    {
        ///判断给定类型是否是std::ratio类型
        template<typename T>
        struct IsRatio:public std::false_type{};

        template<long int Num,long int Den>
        struct IsRatio<std::ratio<Num,Den>>:public std::true_type{};

        ///判断两个数相乘是否会溢出,这里参考了标准库(GNU)的std::ratio的__safe_multiply模板
        template<intmax_t Pn, intmax_t Qn>
        struct SafeMultiplyHelper
        {
        private:
            static const uintmax_t c = uintmax_t(1) << (sizeof(intmax_t) * 4);

            static const uintmax_t a0 = std::__static_abs<Pn>::value % c;
            static const uintmax_t a1 = std::__static_abs<Pn>::value / c;
            static const uintmax_t b0 = std::__static_abs<Qn>::value % c;
            static const uintmax_t b1 = std::__static_abs<Qn>::value / c;

            static constexpr bool flag1 = (a1 == 0 || b1 == 0);
            static constexpr bool flag2 = (a0 * b1 + b0 * a1 < (c >> 1));
            static constexpr bool flag3 = (b0 * a0 <= __INTMAX_MAX__);
            static constexpr bool flag4 = ((a0 * b1 + b0 * a1) * c <= __INTMAX_MAX__ -  b0 * a0);

        public:
            static constexpr bool value = flag1 && flag2 && flag3 && flag4;
        };

        ///判断两个ratio相乘是否会发生溢出,这个模板本来是用于toProperValue函数递归的SFINAE条件的,后来发现特化之后用不上这个模板,暂时保留该模板
        template<typename R1,typename R2>
        struct IsSafeMultiply
        {
        private:
            //先检测传入的两个类型是否是合法的std::ratio类型
            static_assert (IsRatio<R1>::value && IsRatio<R2>::value , "illegal ratio type");

            //因为ratio会自动将分子分母化为最简形式,所以R1、R2在传入的时候就已经是最简形式了
            //交换两个ratio R1和R2的分子,得到的Rs1和Rs2也是最简形式的分数
            //之后再将Rs1和Rs2相乘得到的结果就是最简形式的ratio
            using Rs1 = std::ratio<R2::num,R1::den>;
            using Rs2 = std::ratio<R1::num,R2::den>;

        public:
            //由于Rs1和Rs2已经是最简形式,相乘不会再发生化简行为
            //所以在相乘之前判断两个分母相乘是否会溢出即可
            static constexpr bool value = SafeMultiplyHelper<Rs1::den,Rs2::den>::value;
        };

        ///判断两个ratio相除是否会发生溢出,这个模板本来是用于toProperValue函数递归的SFINAE条件的,后来发现特化之后用不上这个模板,暂时保留该模板
        template<typename R1,typename R2>
        struct IsSafeDivide
        {
            //这里不检测R1和R2是否是合法的std::ratio,因为在IsSafeMultiply模板中会进行一次判断
        private:
            //由于这里是除法,所以需要先将R2的分子分母交换得到R2的倒数R2r
            //再判断R1和R2的倒数相乘是否会发生溢出
            using R2r = std::ratio<R2::den,R2::num>;
        public:
            static constexpr bool value = IsSafeMultiply<R1,R2r>::value;
        };

        template<typename R>
        struct Unit{
            const char* unit = " ?";
        };

        template<>
        struct Unit<std::atto>{
            static constexpr char unit = 'a';
        };

    }

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

    enum ExpEnum{ExpNum};

    enum UnitEnum{UnitNum};

    struct ValuePack{
      double value;
      ExpEnum exp;
      UnitEnum unit;
    };

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
            return toProperValue<std::ratio_multiply<From,Kilo>>(value/1000);
        else if(value < 1)//如果数据小于1则将其乘以1000并上调数量级
            return toProperValue<std::ratio_divide<From,Kilo>>(value*1000);
        else//如果数据刚好在[1,1000)区间内,则认为这已经是一个恰当的值
            return ValuePack{.value = value , .exp = ExpNum, .unit = UnitNum};
    }

    ///模板本身会无限地归导致std::ratio分子/分母溢出,所以这里需要特化模板专门处理溢出的情况
    template<> ValuePack toProperValue<std::atto>(double value)
    {
        return ValuePack{.value = value , .exp = ExpNum, .unit = UnitNum};
    }

    template<> ValuePack toProperValue<std::exa>(double value)
    {
        return ValuePack{.value = value , .exp = ExpNum, .unit = UnitNum};
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
