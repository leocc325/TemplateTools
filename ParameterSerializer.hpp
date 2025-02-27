#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include <type_traits>
#include <utility>
#include <memory>
#include <vector>

/**
 *需要分三种通信协议情况：
 * 情况一、数据帧长度固定为N,每一个数据占用1字节的长度,即长度为N的数据帧包含了N个数据
 * 帧头：1字节
 * 命令码：1字节
 * 指令：1字节
 * 校验：1字节
 * 帧尾：1字节
 *
 *
 * 情况二、数据帧长度固定为N,每一个数据占用的字节长度不等,长度N等于各个数据所占长度的和
 * 帧头：1字节
 * 命令码：1字节
 * 指令：2字节
 * 数据：4字节
 * 校验：1字节
 * 帧尾：1字节
 *
 *
 * 情况三、数据帧长度不固定,中间会有一个长度变化的数据帧,例如下面的通信协议
 * 帧头：1 字节
 * 命令码：1 字节
 * 数据长度：2 字节
 * 数据：可变长度
 * 校验和：2 字节
 * 帧尾：1字节
 *
 *
 * 以上三种情况的具体实现还需要分类，比如：数据按大端保存还是小端保存，是转换为数据帧还是从数据帧中读取某一个数据
 */

enum ByteMode {Big,Little};

template<unsigned...Bytes>
class Fixed
{
private:
    template<unsigned Byte,unsigned...RemainBytes>
    struct Length{
        static constexpr unsigned value = Byte + Length<RemainBytes...>::value;
    };

    template<unsigned Byte>
    struct Length<Byte>{
        static constexpr unsigned value = Byte;
    };

    template<typename First,typename...Args>
    struct IsInteger{
        static constexpr bool value = std::is_integral<First>::value && IsInteger<Args...>::value;
    };

    template<typename First>
    struct IsInteger<First>{
        static constexpr bool value = std::is_integral<First>::value;
    };

    ///类模板参数数量和函数模板参数数量一致，而且每一个实参都是整数
    template<unsigned ClassArgNum,unsigned FuncArgNum,typename...Args>
    struct Matchable{
        static constexpr bool value = (ClassArgNum==FuncArgNum) && IsInteger<Args...>::value;
    };

    ///如果表示每一个数据所占字节大小的模板参数数量为1，而且字节长度总数等于实际参数数量，则认为这是符合情况1的
    template<unsigned ClassArgNum,unsigned BytesLength,typename...Args>
    struct OneBytePerArg{
        static constexpr bool value = (ClassArgNum==1) && (BytesLength == sizeof... (Args)) && IsInteger<Args...>::value;
    };

    ///情况1:数据帧长度固定为N,每一个数据占用1字节的长度时调用此递归模板
    template<unsigned Byte> struct FixTransSingle
    {
        template<typename First,typename...Args>
        static void transImpl(unsigned char* data,unsigned pos,First first,Args...args)
        {
            data[pos] = static_cast<unsigned char>(first) & 0xFF ;
            FixTransSingle<Byte>::transImpl(data,pos+1,std::forward<Args>(args)...);
        }

        template<typename Arg>
        static void transImpl(unsigned char* data,unsigned pos,Arg arg)
        {
            data[pos] = static_cast<unsigned char>(arg) & 0xFF ;
        }
    };

    ///情况2:数据帧长度固定为N,每一个数据占用的字节长度不等时调用此递归模板FixTrans
    template<unsigned Byte,unsigned...RemainBytes>
    struct FixTrans
    {
        template<typename First,typename...Args> static void transImpl(ByteMode mode,unsigned char* data,unsigned pos,First first,Args...args)
        {
            for(unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (mode == Big) ? (Byte - i - 1) * 8 : (i *8 );
                data[pos+i] = static_cast<unsigned char>(first >> offset) & 0xFF ;
            }
            FixTrans<RemainBytes...>::transImpl(mode,data,pos+Byte,std::forward<Args>(args)...);
        }

        ///只剩下最后一个参数时递归模板终止
        template<typename Arg> static void transImpl(ByteMode mode,unsigned char* data,unsigned pos,Arg arg)
        {
            for(int unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (mode == Big) ? (Byte - i - 1) * 8 : (i * 8 );
                data[pos+i] = static_cast<unsigned char>(arg >> offset) & 0xFF ;
            }
        }
    };

public:
    ///总长度固定,单个数据长度可变的转换
    template<ByteMode Mode = Big,typename...Args>
    static typename std::enable_if<!OneBytePerArg<sizeof... (Bytes),Length<Bytes...>::value,Args...>::value,unsigned char*>::type
    trans(Args...args)
    {
        ///判断参数数量、字节长度、和模板参数长度是否一致
        static_assert (Matchable<sizeof... (Bytes),sizeof... (Args),Args...>::value,"error");

        unsigned char* data = new unsigned char[Length<Bytes...>::value];
        FixTrans<Bytes...>::transImpl(Mode,data,0,std::forward<Args>(args)...);
        return data;
    }

    ///总长度固定,单个数据长度1字节的转换
    template<ByteMode Mode = Big, typename...Args>
    static typename std::enable_if<OneBytePerArg<sizeof... (Bytes),Length<Bytes...>::value,Args...>::value,unsigned char*>::type
    trans(Args...args)
    {
        ///判断参数数量和模板指定的帧长度是否一致
        static_assert (sizeof... (Args) == Length<Bytes...>::value,"error");

        unsigned char* data = new unsigned char[Length<Bytes...>::value];
        FixTransSingle<Bytes...>::transImpl(data,0,std::forward<Args>(args)...);
        return data;
    }

};

template<template<unsigned...N1> class F1,typename T,template<unsigned...N2> class F2>
class Variable {
};

void test(){
    auto d1 = Fixed<4>::trans<Big>(0x04,0x05,0x06,0x07);
    auto d2 = Fixed<1,1,2,3>::trans<Big>(0x12,0x34,0x56,0x78);
    auto d3 = Fixed<1,1,2,3>::trans(0x12,0x34,0x5678,0x112233);
    auto d4 = Fixed<2,1,2,3>::trans<Little>(0x12,0x34,0x5678,0x112233);
    auto d5 = Fixed<1,2,2,3>::trans<Little>(0x12,0x34,0x5678,0x112233);
    auto d6 = Fixed<1,1,2,2>::trans<Little>(0x12,0x34,0x5678,0x112233);
    auto d7 = Fixed<1,1,2,4>::trans<Little>(0x12,0x34,0x5678,0x112233);
    auto d8 = Fixed<1,3,2,3>::trans<Little>(0x12,0x34,0x5678,0x112233);
    auto d9 = Fixed<1,4,2,3>::trans<Little>(0x12,0x34,0x5678,0x112233);
    int a = 10;
}

#endif // PARAMETERSERIALIZER_HPP
