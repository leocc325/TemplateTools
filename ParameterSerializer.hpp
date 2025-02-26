
#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

template <unsigned...Bytes>
struct ParameterSerializer
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

    template<unsigned ArgNum,unsigned BytesNum,typename...Args>
    struct Matchable{
        static constexpr bool value = (ArgNum==BytesNum) && IsInteger<Args...>::value;
    };

    /**
     *转换时始终从低位地址开始写入，所以大端就是先写入数据的高位，小端就是先写入数据的低位
     */
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

#endif // PARAMETERSERIALIZER_HPP
