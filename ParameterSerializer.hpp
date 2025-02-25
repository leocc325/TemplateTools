#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

//template<unsigned...Bitss>
struct ParameterSerializer
{
    template<unsigned Bit,unsigned...Bits>
    struct Length{
        static constexpr unsigned value = Bit + Length<Bits...>::value;
    };

    template<unsigned Bit>
    struct Length<Bit>{
        static constexpr unsigned value = Bit;
    };

    template<unsigned Bit,unsigned...Bits>
    static constexpr unsigned len()
    {
        return Bit + len<Bits...>();
    }

    template<>
    static constexpr unsigned len<unsigned Bit>()
    {
        return Bit;
    }

//    template<typename...Args>
//    static unsigned char* trans(Args...args)
//    {
//        unsigned char* data = new unsigned char[Length<Bitss...>::value];
//    }

//    template<unsigned Pos,unsigned...Bit,typename First,typename...Args>
//    static unsigned char* transImpl(unsigned char* data,const First& first,const Args&...args)
//    {

//    }
};

#endif // PARAMETERSERIALIZER_HPP
