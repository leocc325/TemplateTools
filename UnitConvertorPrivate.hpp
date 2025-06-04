#ifndef UNITCONVERTORPRIVATE_H
#define UNITCONVERTORPRIVATE_H

#include <ratio>

namespace UnitConvertor
{
    ///这里重新定义了标准库std::ratio别名的原因是增加了部分比例类型
    using ExpAtto      =   std::atto; //10^-18 阿 a
    using ExpFemto  =   std::femto;//10^-15飞 f
    using ExpPico     =   std::pico;//10^-12 皮 p
    using ExpNano   =   std::nano;//10^-9 纳 n
    using ExpMicro  =   std::micro;//10^-6 微 μ
    using ExpMilli   =   std::milli;//10^-3 毫 m
    using ExpCenti  =   std::centi;//10^-2 厘 c
    using ExpDeci   =   std::deci;//10^-1 分 d
    using ExpOne    =   std::ratio<1,1>;//1 个
    using ExpDeca   =   std::deca;//10^1 十 da
    using ExpHecto  =   std::hecto;//10^2 百 h
    using ExpKilo   =   std::kilo;//10^3 千 k
    using ExpMega   =   std::mega;//10^6兆M
    using ExpGiga   =   std::giga;//10^9吉G
    using ExpTera   =   std::tera;//10^12太T
    using ExpPeta   =   std::peta;//10^15拍P
    using ExpExa    =   std::exa;//10^18艾E

    enum ExpEnum {
        EnumAtto,
        EnumFemto,
        EnumPico,
        EnumNano,
        EnumMicro,
        EnumMilli,
        EnumCenti,
        numDeci,
        EnumOne,
        EnumDeca,
        EnumHecto,
        EnumKilo,
        EnumMega,
        EnumGiga,
        EnumTera,
        EnumPeta,
        EnumExa,
        EnumExpNum
    };

    enum UnitEnum
    {
        UnitNum
    };

    template<typename T> struct ExpChar{static constexpr char  c =  '?' ;};

    template<> struct ExpChar<ExpAtto>{static constexpr char  c =  'a' ;};

    template<> struct ExpChar<ExpFemto>{static constexpr char  c =  'f' ;};

    template<> struct ExpChar<ExpPico>{static constexpr char  c =  'p' ;};

    template<> struct ExpChar<ExpNano>{static constexpr char  c =  'n' ;};

    template<> struct ExpChar<ExpMicro>{static constexpr char  c =  'u' ;};

    template<> struct ExpChar<ExpMilli>{static constexpr char  c =  'm' ;};

    template<> struct ExpChar<ExpCenti>{static constexpr char  c =  'c' ;};

    template<> struct ExpChar<ExpDeci>{static constexpr char  c =  'd' ;};

    template<> struct ExpChar<ExpOne>{static constexpr char  c =  ' ' ;};

    template<> struct ExpChar<ExpDeca>{static constexpr char  c =  'D' ;};//实际上应该是da

    template<> struct ExpChar<ExpHecto>{static constexpr char  c =  'h' ;};

    template<> struct ExpChar<ExpKilo>{static constexpr char  c =  'k' ;};

    template<> struct ExpChar<ExpMega>{static constexpr char  c =  'M' ;};

    template<> struct ExpChar<ExpGiga>{static constexpr char  c =  'G' ;};

    template<> struct ExpChar<ExpTera>{static constexpr char  c =  'T' ;};

    template<> struct ExpChar<ExpPeta>{static constexpr char  c =  'P' ;};

    template<> struct ExpChar<ExpExa>{static constexpr char  c =  'E' ;};

    struct ValuePack{
        double value;
        ExpEnum exp;
        UnitEnum unit;
    };

    ///判断给定类型是否是std::ratio类型
    template<typename T>
    struct IsRatio:public std::false_type{};

    template<intmax_t Num,intmax_t Den>
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
}

#endif // UNITCONVERTORPRIVATE_H
