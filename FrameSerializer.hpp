#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include <type_traits>
#include <utility>
#include <memory>
#include <vector>

#include <QDebug>
namespace FrameSerializer
{
    namespace  {
        enum ByteMode {
            Big,
            Little
        };

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

        ///验证类模板参数数量和函数模板参数数量一致，而且每一个实参都是整数
        template<unsigned ClassArgNum,unsigned FuncArgNum,typename...Args>
        struct Matchable{
            static constexpr bool value = (ClassArgNum==FuncArgNum) && IsInteger<Args...>::value;
        };

        template<unsigned ClassArgNum,unsigned BytesLength,typename...Args>
        struct OneBytePerArg{
            static constexpr bool value = (ClassArgNum==1) && (BytesLength == sizeof... (Args)) && IsInteger<Args...>::value;
        };

        ///当前允许的校验结果最大字节数
        static constexpr unsigned maxCheckSize = 8;

        enum DataSize{oneByte,twoByte,fourByte,eightByte};

        static constexpr  DataSize ByteWidthArray[maxCheckSize] = {oneByte,twoByte,fourByte,fourByte,eightByte,eightByte,eightByte,eightByte};

        template<unsigned Bytes>struct DataType{using type = void;};

        template<> struct DataType<oneByte>{using type = unsigned char;};

        template<> struct DataType<twoByte>{using type = unsigned short;};

        template<> struct DataType<fourByte>{using type = unsigned int;};

        template<> struct DataType<eightByte>{using type = unsigned long long;};

        template<unsigned Bytes>
        using FunctionReturn = typename std::enable_if<(Bytes <= maxCheckSize),void>::type;

        template<unsigned Bytes>
        using DT =  typename DataType<ByteWidthArray[Bytes]>::type;
    }

    template<unsigned...Bytes>
    class FixedFrame
    {
    private:
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
                    unsigned offset = (mode == Big) ? (Byte - i - 1) * CHAR_BIT : (i *CHAR_BIT );
                    data[pos+i] = static_cast<unsigned char>(first >> offset) & 0xFF ;
                }
                FixTrans<RemainBytes...>::transImpl(mode,data,pos+Byte,std::forward<Args>(args)...);
            }

            ///只剩下最后一个参数时递归模板终止
            template<typename Arg> static void transImpl(ByteMode mode,unsigned char* data,unsigned pos,Arg arg)
            {
                for(int unsigned i = 0; i < Byte; i++)
                {
                    unsigned offset = (mode == Big) ? (Byte - i - 1) * CHAR_BIT : (i * CHAR_BIT );
                    data[pos+i] = static_cast<unsigned char>(arg >> offset) & 0xFF ;
                }
            }
        };

    public:
        ///总长度固定,单个数据长度可变的转换
        template<ByteMode Mode = Big,typename...Args>
        static typename std::enable_if<!OneBytePerArg<sizeof... (Bytes),Length<Bytes...>::value,Args...>::value,std::unique_ptr<unsigned char[]>>::type
        trans(Args...args)
        {
            static_assert (Matchable<sizeof... (Bytes),sizeof... (Args),Args...>::value,"error");

            std::unique_ptr<unsigned char[]> data(new unsigned char[Length<Bytes...>::value]);
            FixTrans<Bytes...>::transImpl(Mode,data.get(),0,std::forward<Args>(args)...);
            return data;
        }

        ///总长度固定,单个数据长度1字节的转换
        template<ByteMode Mode = Big, typename...Args>
        static typename std::enable_if<OneBytePerArg<sizeof... (Bytes),Length<Bytes...>::value,Args...>::value,std::unique_ptr<unsigned char[]>>::type
        trans(Args...args)
        {
            static_assert (sizeof... (Args) == Length<Bytes...>::value,"error");

            std::unique_ptr<unsigned char[]> data(new unsigned char[Length<Bytes...>::value]);
            FixTransSingle<Bytes...>::transImpl(data.get(),0,std::forward<Args>(args)...);
            return data;
        }

    };

    ///unused
    template<template<unsigned...N1> class F1,typename T,template<unsigned...N2> class F2>
    class VariableFrame {};

    class FrameCheck
    {
        ///位反转模板函数
        template<typename T>
        static T reverse_bits(T value)
        {
            const size_t bits = sizeof(T) * CHAR_BIT;
            T reversed = 0;
            for (size_t i = 0; i < bits; ++i)
            {
                reversed = static_cast<T>(reversed << 1) | ((value >> i) & 1);
            }
            return reversed;
        }

        template<unsigned Bytes>
        static void writeCheckValue(ByteMode mode,unsigned char* data,DT<Bytes> check,unsigned pos)
        {
            for(unsigned index = 0; index < Bytes; index++)
            {
                unsigned char value = static_cast<unsigned char>(check >> (CHAR_BIT * index)) & 0xFF;
                if(mode == Big)
                     data[pos+Bytes-index-1] = value;
                else
                    data[pos+index] = value;
            }
        }

        /**
         *crcbits:crc寄存器位宽
         *CrcDT:crc寄存器变量类型
         *poly:多项式,不同的CRC标准使用不同的多项式。
         *init:crc初始值
         *xorOut:最终异或值,计算完成后与结果异或的值。
         *refIn:输入是否反转,即每个字节是否需要按位反转后再处理。
         *refOut:输出是否反转,即最终的CRC值是否需要按位反转。
         *
         * note:这个模板不是通过crc原理完成的,模板最初只完成了针对8位及8位以上的crc校验,后续为了将不足8位的crc校验也兼容进来
         * 后续是参考https://github.com/whik/crc-lib-c 一边debug一边修改函的
         * 所以我也不清楚有些地方为什么要那样写代码,代码中用note标记的地方就是我也不太清楚原理的部分
         * but! 代码能正常工作!
         */
        template<int crcbits,typename CrcDT,CrcDT polynomial,CrcDT init,CrcDT xorOut,bool refIn,bool refOut>
        static CrcDT crcImpl(unsigned char* data,unsigned start,unsigned end)
        {
            CrcDT crc =  init;
            CrcDT poly = polynomial;

            constexpr unsigned crcSize = sizeof (CrcDT) * CHAR_BIT;//crc变量总位数
            constexpr CrcDT topbit = static_cast<CrcDT>(1) << (crcSize - 1); //crc最高位为1时的值

            //如果crc位数不是8的整倍数,需要将crc右端补0
            constexpr int crcOffset = (crcbits % CHAR_BIT) ?  CHAR_BIT - (crcbits % CHAR_BIT) : 0;
            //对输入数据每一个字节右端补0
            constexpr int byteOffset = crcSize - CHAR_BIT;

            //如果crc位宽不是8的整倍数,需要对多项式和crc初始值左移crcOffset位
            poly =  static_cast<CrcDT>(poly << crcOffset);/**note**/
            crc = static_cast<CrcDT>(crc << crcOffset);/**note**/

            // 遍历数据
            for (unsigned i = start; i <= end; ++i)
            {
                unsigned char byte = refIn ? reverse_bits<unsigned char>(data[i]) : data[i];// 输入反转处理
                crc  = static_cast<CrcDT>( crc ^ (static_cast<CrcDT>(byte) << byteOffset) );// 将当前字节反转之后再进行左移和CRC对齐(相当于右端补零),随后与crc异或

                for (int j = 0; j < CHAR_BIT; ++j)
                {
                    if(crc & topbit)
                        crc =  static_cast<CrcDT>((crc << 1) ^ poly);
                    else
                        crc =  static_cast<CrcDT>(crc << 1);
                }
            }

            // 输出反转处理
            crc = refOut ? reverse_bits<CrcDT>(crc) : crc;

            // 最终异或
            crc = crc ^ xorOut;

            //输入不反转而且crc位数不为8的整数倍的而左移过的结果需要将其右移回去
            crc = refIn ? crc : (crc >> crcOffset);/**note**/
            return crc;
        }

    public:
        ///对给定的char数组计算校验和:计算给定数组data从start处开始到end处结尾所有字节的校验和,校验结果占用Bytes字节大小,放置到数组pos处,Mode表明校验结果是大端存储还是小端存储
        template <unsigned Bytes,ByteMode mode = Big>
        static FunctionReturn<Bytes> sum(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes>  sum = 0;
            for(; start <= end; ++start){
                sum += data[start];
            }

            writeCheckValue<Bytes>(mode,data,sum,pos);
        }

        /// 对给定的char数组计算crc校验:计算给定数组data从start处开始到end处结尾所有字节的crc,校验结果占用Bytes字节大小,放置到数组pos处,Mode表明校验结果是大端存储还是小端存储
        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc4_itu(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<4,unsigned char,0x03,0x00,0x00,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc5_epc(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<5,unsigned char,0x09,0x09,0x00,false,false>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc5_itu(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<5,unsigned char,0x15,0x00,0x00,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc5_usb(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<5,unsigned char,0x05,0x1F,0x1F,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc6_itu(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<6,unsigned char,0x03,0x00,0x00,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc7_mmc(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<7,unsigned char,0x09,0x00,0x00,false,false>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc8(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<8,unsigned char,0x07,0x00,0x00,false,false>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc8_itu(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<8,unsigned char,0x07,0x00,0x55,false,false>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc8_rohc(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<8,unsigned char,0x07,0xFF,0x00,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 1,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc8_maxim(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<8,unsigned char,0x31,0x00,0x00,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_ibm(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x8005,0x000,0x000,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_maxim(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x8005,0x0000,0xFFFF,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_usb(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x8005,0xFFFF,0xFFFF,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_modbus(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x8005,0xFFFF,0x0000,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_ccitt(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x1021,0x0000,0x0000,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_ccitt_false(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x1021,0xFFFF,0x0000,false,false>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_x25(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x1021,0xFFFF,0xFFFF,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_xmodem(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x1021,0x0000,0x0000,false,false>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 2,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc16_dnp(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
           DT<Bytes> crc =  crcImpl<16,unsigned short,0x3d65,0x0000,0xFFFF,true,true>(data,start,end);
           writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 4,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc32(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<32,unsigned int,0x04C11DB7,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 4,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc32_c(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<32,unsigned int,0x01EDC6F41,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 4,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc32_koopman(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<32,unsigned int,0x741B8CD7,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }

        template <unsigned Bytes = 4,ByteMode mode = Big>
        static FunctionReturn<Bytes> crc32_mpeg2(unsigned char* data,unsigned start,unsigned end,unsigned pos)
        {
            DT<Bytes> crc =  crcImpl<32,unsigned int,0x04C11DB7,0xFFFFFFFF,0x00000000,false,false>(data,start,end);
            writeCheckValue<Bytes>(mode,data,crc,pos);
        }
    };

    static void test(){
        auto d1 = FixedFrame<8>::trans<Big>(0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88);
        unsigned char* data = d1.get();

        FrameCheck::crc4_itu(data,0,7,7);//0x05
        FrameCheck::crc5_epc(data,0,7,7);//0x0f
        FrameCheck::crc5_itu(data,0,7,7);//0x05
        FrameCheck::crc5_usb(data,0,7,7);
        FrameCheck::crc6_itu(data,0,7,7);
        FrameCheck::crc7_mmc(data,0,7,7);
        FrameCheck::crc8(data,0,7,7);
        FrameCheck::crc8_itu(data,0,7,7);
        FrameCheck::crc8_rohc(data,0,7,7);
        FrameCheck::crc8_maxim(data,0,7,7);
        FrameCheck::crc16_dnp(data,0,7,7);
        FrameCheck::crc16_ibm(data,0,7,7);
        FrameCheck::crc16_usb(data,0,7,7);
        FrameCheck::crc16_x25(data,0,7,7);
        FrameCheck::crc16_ccitt(data,0,7,7);
        FrameCheck::crc16_maxim(data,0,7,7);
        FrameCheck::crc16_modbus(data,0,7,7);
        FrameCheck::crc16_xmodem(data,0,7,7);
        FrameCheck::crc16_ccitt_false(data,0,7,7);
        FrameCheck::crc32(data,0,7,7);
        FrameCheck::crc32_c(data,0,7,7);
        FrameCheck::crc32_mpeg2(data,0,7,7);
        FrameCheck::crc32_koopman(data,0,7,7);
    }

}

#endif // PARAMETERSERIALIZER_HPP
