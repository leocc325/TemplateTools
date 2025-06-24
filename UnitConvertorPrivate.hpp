#ifndef UNITCONVERTORPRIVATE_H
#define UNITCONVERTORPRIVATE_H

#include <algorithm>
#include <ratio>
#include <string>
#include <locale>
#include <vector>

#define USE_QSTRING 0

#if USE_QSTRING
#include <QStringList>
#endif

namespace UnitConvertor
{
    ///这里重新定义了标准库std::ratio别名的原因是增加了部分比例类型
    using Atto   =   std::atto;       //10^-18 阿  a
    using Femto  =   std::femto;      //10^-15 飞  f
    using Pico   =   std::pico;       //10^-12 皮  p
    using Nano   =   std::nano;       //10^-9  纳  n
    using Micro  =   std::micro;      //10^-6  微  μ
    using Milli  =   std::milli;      //10^-3  毫  m
    using Centi  =   std::centi;      //10^-2  厘  c
    using Deci   =   std::deci;       //10^-1  分  d
    using One    =   std::ratio<1>;   //1      个
    using Deca   =   std::deca;       //10^1   十  da
    using Hecto  =   std::hecto;      //10^2   百  h
    using Kilo   =   std::kilo;       //10^3   千  k
    using Mega   =   std::mega;       //10^6   兆  M
    using Giga   =   std::giga;       //10^9   吉  G
    using Tera   =   std::tera;       //10^12  太  T
    using Peta   =   std::peta;       //10^15  拍  P
    using Exa    =   std::exa;        //10^18  艾  E

    enum ExpEnum {EnumAtto,EnumFemto,EnumPico,EnumNano,EnumMicro,EnumMilli,EnumCenti,EnumDeci,EnumOne,
                  EnumDeca,EnumHecto,EnumKilo,EnumMega,EnumGiga,EnumTera,EnumPeta,EnumExa,EnumExpNum};

    static const std::string Exps[EnumExpNum] = {"a","f","p","n","u","m","c","d"," ","da","h","k","M","G","T","P","E"};

    enum UnitEnum {EnumNull,EnumFrequency,EnumVoltage,EnumCurrent,EnumTime,EnumUnitNum};

    static const std::string Units[EnumUnitNum] = {" ","Hz","V","A","s"};

    ///根据ratio类型获取数量级字符
    template<typename T> struct ExpToEnum{static constexpr ExpEnum  E =  EnumExpNum ;};

    template<> struct ExpToEnum<Atto>{static constexpr ExpEnum  E =  EnumAtto ;};

    template<> struct ExpToEnum<Femto>{static constexpr ExpEnum  E =  EnumFemto ;};

    template<> struct ExpToEnum<Pico>{static constexpr ExpEnum  E =  EnumPico ;};

    template<> struct ExpToEnum<Nano>{static constexpr ExpEnum  E =  EnumNano ;};

    template<> struct ExpToEnum<Micro>{static constexpr ExpEnum  E =  EnumMicro ;};

    template<> struct ExpToEnum<Milli>{static constexpr ExpEnum  E =  EnumMilli ;};

    template<> struct ExpToEnum<Centi>{static constexpr ExpEnum  E =  EnumCenti ;};

    template<> struct ExpToEnum<Deci>{static constexpr ExpEnum  E =  EnumDeci ;};

    template<> struct ExpToEnum<One>{static constexpr ExpEnum  E =  EnumOne ;};

    template<> struct ExpToEnum<Deca>{static constexpr ExpEnum  E =  EnumDeca ;};

    template<> struct ExpToEnum<Hecto>{static constexpr ExpEnum  E =  EnumHecto ;};

    template<> struct ExpToEnum<Kilo>{static constexpr ExpEnum  E =  EnumKilo ;};

    template<> struct ExpToEnum<Mega>{static constexpr ExpEnum  E =  EnumMega ;};

    template<> struct ExpToEnum<Giga>{static constexpr ExpEnum  E =  EnumGiga ;};

    template<> struct ExpToEnum<Tera>{static constexpr ExpEnum  E =  EnumTera ;};

    template<> struct ExpToEnum<Peta>{static constexpr ExpEnum  E =  EnumPeta ;};

    template<> struct ExpToEnum<Exa>{static constexpr ExpEnum  E =  EnumExa ;};

    template<ExpEnum> struct ExpFromEnum{using type = One;};

    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

//    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

//    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

//    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

//    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

//    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

//    template<> struct ExpFromEnum<EnumAtto>{using type = Atto;};

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

    ///专门处理string的函数

    struct ValuePack
    {
        operator double () {
            return value;
        };

        operator std::string() {
            return std::to_string(value)+" "+std::string(Exps[exp])+std::string(Units[unit]);
        };

#if USE_QSTRING
        operator QString() {
            return QString("%1 %2%3").arg(value).arg(Exps[exp]).arg(Units[unit]);
        };
#endif

        double value  = 0;
        ExpEnum exp   = EnumOne;
        UnitEnum unit = EnumNull;
    };

    ///***findExp和findUnit存在缺陷,只要两端数量级和单位正确,就算中间夹杂了额外的字符,也会正确解析。而且如果单位A是单位B的子集,那么有可能错误的将B解析成A。***

    ///查找数量级
    inline ExpEnum findExp(const std::string& input)
    {
        if(input.empty())
            return EnumOne;

        for(int i = 0; i < EnumExpNum; i++)
        {
            if(input.size() >= Exps[i].size())
            {
                //只对开头n个字节做比较
                if(input.compare(0,Exps[i].size(),Exps[i]) == 0)
                    return ExpEnum(i);
            }
        }
        //如果没有查询到匹配的数量级则认为数量级为1
        return EnumOne;
    }

    inline UnitEnum findUnit(const std::string& input)
    {
        if(input.empty())
            return EnumNull;

        for(int i = 0; i < EnumUnitNum; i++)
        {
            if(input.size() >= Units[i].size())
            {
                //只对最后n个字节做比较
                std::size_t pos = input.size()-Units[i].size();
                std::size_t len = Units[i].size();
                if(input.compare(pos,len,Units[i]) == 0)
                    return UnitEnum(i);
            }
        }
        //如果没有查询到匹配的单位则认为传入的字符串是不带单位的
        return EnumNull;
    }

    ///将字符串中的英文转换为小写
    inline void toLower(std::string& input)
    {
        if(input.empty())
            return;

        std::locale loc = std::locale::classic();
        std::use_facet<std::ctype<char>>(loc).tolower(&input[0],&input[0]+input.size());
    }

    ///将字符串中的英文转换为大写
    inline void toUpper(std::string& input)
    {
        if(input.empty())
            return;

        std::locale loc = std::locale::classic();
        std::use_facet<std::ctype<char>>(loc).toupper(&input[0],&input[0]+input.size());
    }

    ///按空格分割字符串
    inline std::vector<std::string> split(const std::string& input)
    {
        std::vector<std::string> strVec;

        if(input.empty())
            return strVec;

        std::string::const_iterator inputBeg = input.cbegin();
        std::string::const_iterator inputEnd = input.cend();

        std::string::const_iterator it1 = input.cbegin();
        std::string::const_iterator it2 = input.cbegin();

        while (it1 != inputEnd)
        {
            //先使用第一个迭代器查找非空格的字符,将结果赋值给第一个迭代器
            it1 = std::find_if(it1,inputEnd,[](char ch){return !std::isspace(ch);});
            if(it1 == inputEnd)
                break;

            //随后从第一个迭代器的位置开始查找后续会遇到的第一个空格,并赋值给第二个迭代器
            it2 = std::find_if(it1,inputEnd,[](char ch){return std::isspace(ch);});

            //计算两个迭代器之间的距离,将两个迭代器之间的数据拷贝到新的string中
            std::string s = input.substr(std::distance(inputBeg,it1),std::distance(it1,it2));
            strVec.push_back(std::move(s));

            //拷贝完毕之后将第一个迭代器位置移动到第二个迭代器位置处,开始下一轮查找
            it1 = it2;
        }

        return strVec;
    }
}

#endif // UNITCONVERTORPRIVATE_H
