#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include <limits>
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

/**
 * @brief The DataFrame class:一个完整的数据帧
 */
class DataFrame{
    
};

/**
 * @brief The DataPack class:数据帧中的数据包部分
 */
class DataPack{
    
};

//template <unsigned Bytes>
//struct ParameterSerializer{
    
//};
        
template <unsigned...Bytes>
class ParameterSerializer
{
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

    template<unsigned ArgNum,unsigned BytesNum,typename...Args>
    struct Matchable{
        static constexpr bool value = (ArgNum==BytesNum) && IsInteger<Args...>::value;
    };

    template<unsigned Byte,unsigned...RemainBytes>
    struct TransImpl
    {
        template<typename First,typename...Args>
        static unsigned char* bigEndian(unsigned char* data,unsigned pos,First first,Args...args)
        {
            for(unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = i * __CHAR_BIT__;
                data[pos+i] = static_cast<unsigned char>(first >> offset) & 0xFF ;
            }
            return TransImpl<RemainBytes...>::bigEndian(data,pos+Byte,std::forward<Args>(args)...);
        }

        template<typename First,typename...Args>
        static unsigned char* littleEndian(unsigned char* data,unsigned pos,First first,Args...args)
        {
            for(unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (Byte - i - 1) * __CHAR_BIT__;
                data[pos+i] = static_cast<unsigned char>(first >> offset) & 0xFF ;
            }
            return TransImpl<RemainBytes...>::littleEndian(data,pos+Byte,std::forward<Args>(args)...);
        }
    };

    template<unsigned Byte>
    struct TransImpl<Byte>
    {
        template<typename Arg>
        static unsigned char* bigEndian(unsigned char* data,unsigned pos,Arg arg)
        {
            for(int unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = i * __CHAR_BIT__;
                data[pos+i] = static_cast<unsigned char>(arg >> offset) & 0xFF ;
            }
            return data;
        }

        template<typename Arg>
        static unsigned char* littleEndian(unsigned char* data,unsigned pos,Arg arg)
        {
            for(int unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (Byte - i - 1) * __CHAR_BIT__;
                data[pos+i] = static_cast<unsigned char>(arg >> offset) & 0xFF ;
            }
            return data;
        }
    };

public:
    template<typename...Args>
    static typename std::enable_if<Matchable<sizeof... (Args),sizeof... (Bytes),Args...>::value,unsigned char*>::type
    transToBigEndian(Args...args)
    {
        unsigned char* data = new unsigned char[Length<Bytes...>::value];
        return TransImpl<Bytes...>::bigEndian(data,0,std::forward<Args>(args)...);
    }

    template<typename...Args>
    static typename std::enable_if<Matchable<sizeof... (Args),sizeof... (Bytes),Args...>::value,unsigned char*>::type
    transToLittleEndian(Args...args)
    {
        unsigned char* data = new unsigned char[Length<Bytes...>::value];
        return TransImpl<Bytes...>::littleEndian(data,0,std::forward<Args>(args)...);
    }
};

template <typename... Args>
class Parameter {
};

template <unsigned...N,typename... Args>
class Parameter {
};

void test(){
    Parameter<1,2,3,double,4>;
}

#endif // PARAMETERSERIALIZER_HPP
