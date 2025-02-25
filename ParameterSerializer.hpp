#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include <string.h>
#include <malloc.h>
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

    template<unsigned Byte,unsigned...RemainBytes>
    struct TransImpl{
        template<typename First,typename...Args>
        static unsigned char* trans(unsigned char* data,unsigned pos,First first,Args...args)
        {
            unsigned char* ch= static_cast<unsigned char*>(alloca(Byte));
            ch = reinterpret_cast<unsigned char*>(first);
            for(unsigned i = 0; i < Byte; i++){
                data[pos+i] = ch[i];
            }
            return TransImpl<RemainBytes...>::trans(data,pos+Byte,std::forward<Args>(args)...);
        }
    };

    template<unsigned Byte>
    struct TransImpl<Byte>{
        template<typename Arg>
        static unsigned char* trans(unsigned char* data,unsigned pos,Arg arg)
        {
            unsigned char* ch= static_cast<unsigned char*>(alloca(Byte));
            ch = reinterpret_cast<unsigned char*>(arg);
            for(int unsigned i = 0; i < Byte; i++){
                data[pos+i] = ch[i];
            }
            return data;
        }
    };
public:
    template<typename...Args>
    static typename std::enable_if<Matchable<sizeof... (Args),sizeof... (Bytes),Args...>::value,unsigned char*>::type
    trans(Args...args)
    {
        unsigned char* data = new unsigned char[Length<Bytes...>::value];
        return TransImpl<Bytes...>::trans(data,0,std::forward<Args>(args)...);
    }

};

#endif // PARAMETERSERIALIZER_HPP
