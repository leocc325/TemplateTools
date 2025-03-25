#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include "FrameSerializerPrivate.hpp"

namespace FrameSerializer
{
namespace
{
    enum ByteMode {
        Big,
        Little
    };

    ///计算总的字节长度
    template<unsigned Byte,unsigned...RemainBytes>
    struct Length{
        static constexpr unsigned value = Byte + Length<RemainBytes...>::value;
    };

    template<unsigned Byte>
    struct Length<Byte>{
        static constexpr unsigned value = Byte;
    };

    ///判断是否每一个变量都能被转换为无符号整形变量
    template<typename First,typename...Args>
    struct IsNumeric{
        static constexpr bool value = std::is_convertible<First,unsigned>::value && IsNumeric<Args...>::value;
    };

    template<typename First>
    struct IsNumeric<First>{
        static constexpr bool value = std::is_convertible<First,unsigned>::value;
    };

    ///当前允许的校验结果最大字节数
    static constexpr unsigned maxCheckSize = 8;

    ///每一个字节的长度,固定为8位
    static constexpr unsigned CharBit = 8;

    enum DataSize{oneByte = 1,twoByte = 2,fourByte = 4,eightByte = 8};

    static constexpr  DataSize ByteWidthArray[maxCheckSize] = {oneByte,twoByte,fourByte,fourByte,eightByte,eightByte,eightByte,eightByte};

    ///根据当前数据所占字节数返回一个能容纳数据长度的变量类型
    template<unsigned Bytes>struct DataType{using type = void;};

    template<> struct DataType<oneByte>{using type = unsigned char;};

    template<> struct DataType<twoByte>{using type = unsigned short;};

    template<> struct DataType<fourByte>{using type = unsigned int;};

    template<> struct DataType<eightByte>{using type = unsigned long long;};

    ///别名模板,简化DataType的代码长度
    template<unsigned Bytes>
    using DT =  typename DataType<ByteWidthArray[Bytes-1]>::type;

    ///判断当前校验结果的长度是否超出8字节(一般不会用到超出8个字节的整形变量)或者小于最短字节数,超出8字节屏蔽模板
    template<unsigned Bytes,unsigned Min>
    using FunctionReturn = typename std::enable_if<(Bytes <= maxCheckSize) && (Bytes >= Min),DT<Bytes>>::type;
}

template<unsigned...BytePerArg>
struct Trans
{
    ///如果帧的长度和参数数量相等,则说明是一个数据占一个字节.如果帧的长度大于参数数量,则说明至少一个一个数据占用了两个以上的字节
    template<unsigned ArgsNum,unsigned TotalLength>
    struct OneBytePerArg{
        static constexpr bool value =  (ArgsNum == TotalLength);
    };

    template<typename...Args>
    constexpr static bool OneBytePerArg_v = OneBytePerArg<sizeof... (Args),Length<BytePerArg...>::value>::value;

    ///根据通信协议将传入的参数转换为数据帧,适用于每个数据对应的字节数均是1的情况,这种情况下不区分大小端
    template<ByteMode Mode = Big, typename...Args>
    static typename std::enable_if<OneBytePerArg_v<Args...>,Frame>::type
    byProtocol(Args...args)
    {
        static_assert (IsNumeric<Args...>::value, "All input parameters should be convertible to integer types.");

        Frame data(Length<BytePerArg...>::value);
        TransByProtocolSingle<BytePerArg...>::transImpl(data,0,std::forward<Args>(args)...);
        return data;
    }

    ///根据通信协议将传入的参数转换为数据帧,适用于数据所占字节数不一致的情况，这种情况下需要区分大小端,默认为大端
    template<ByteMode Mode = Big,typename...Args>
    static typename std::enable_if<!OneBytePerArg_v<Args...>,Frame>::type
    byProtocol(Args...args)
    {
        //数据所占字节数大于1个数据1字节时需要额外判断传入的参数数量是否和描述通信协议的模板参数数量一致
        static_assert (sizeof... (BytePerArg) == sizeof... (Args), "the number of class template should be equal with the number of this function");
        static_assert (IsNumeric<Args...>::value, "All input parameters should be convertible to integer types.");

        Frame data(Length<BytePerArg...>::value);
        TransByProtocol<BytePerArg...>::transImpl(Mode,data,0,std::forward<Args>(args)...);
        return data;
    }

    ///将容器中的数据按顺序转换为占BytePerArg字节的char数组,容器必须存在size函数可获取容器内元素数量而且BytePerArg的数量只能为1,转换后的char数组默认为大端保存
    template<ByteMode mode = Big,typename T,template<typename...Element> class Array,typename...Args>
    static Frame fromArray(const Array<T,Args...>& array)
    {
        static_assert (sizeof... (BytePerArg) == 1, "the number of class template should be one");
        static_assert (IsNumeric<Args...>::value, "All input parameters should be convertible to integer types.");

        unsigned byteLength = Length<BytePerArg...>::value;
        Frame data(byteLength * array.size());

        unsigned argIndex = 0;
        for(const T& value : array)
        {
            for(unsigned i = 0; i < byteLength; i++)
            {
                unsigned offset = (mode == Big) ? (byteLength - i - 1) * CharBit : (i *CharBit );
                data[argIndex*byteLength+i] = static_cast<unsigned char>(value >> offset) & 0xFF ;
            }
            ++argIndex;
        }

        return data;
    }

    ///将数组中的数据按顺序转换为占BytePerArg字节的char数组,而且BytePerArg的数量只能为1,转换后的char数组默认为大端保存
    template<ByteMode mode = Big,size_t N,typename T>
    static   Frame fromArray(const T(&array)[N])
    {
        static_assert (sizeof... (BytePerArg) == 1 , "the number of class template should be one");
        static_assert (IsNumeric<T>::value, "All input parameters should be convertible to integer types.");

        unsigned byteLength = Length<BytePerArg...>::value;
        Frame data(byteLength * N);

        for(int index = 0; index < N; index++)
        {
            for(unsigned i = 0; i < byteLength; i++)
            {
                unsigned offset = (mode == Big) ? (byteLength - i - 1) * CharBit : (i *CharBit );
                data[index*byteLength+i] = static_cast<unsigned char>(array[index] >> offset) & 0xFF ;
            }
        }

        return data;
    }

    ///将指针数组中的数据按顺序转换为占BytePerArg字节的char数组,而且BytePerArg的数量只能为1,转换后的char数组默认为大端保存
    template<ByteMode mode = Big,typename T>
    static  Frame fromArray(const T* array,std::size_t length)
    {
        static_assert (sizeof... (BytePerArg) == 1, "the number of class template should be one");
        static_assert (IsNumeric<T>::value, "All input parameters should be convertible to integer types.");

        unsigned byteLength = Length<BytePerArg...>::value;
        Frame data(byteLength * length);

        for(unsigned index = 0; index < length; index++)
        {
            for(unsigned i = 0; i < byteLength; i++)
            {
                unsigned offset = (mode == Big) ? (byteLength - i - 1) * CharBit : (i *CharBit );
                data[index*byteLength+i] = static_cast<unsigned char>(array[index] >> offset) & 0xFF ;
            }
        }

        return data;
    }

private:
    ///数据帧长度固定为N,每一个数据占用1字节的长度时调用此递归模板
    template<unsigned Byte> struct TransByProtocolSingle
    {
        template<typename First,typename...Args>
        static void transImpl(unsigned char* data,unsigned pos,First first,Args...args)
        {
            data[pos] = static_cast<unsigned char>(first) & 0xFF ;
            TransByProtocolSingle<Byte>::transImpl(data,pos+1,std::forward<Args>(args)...);
        }

        template<typename Arg>
        static void transImpl(unsigned char* data,unsigned pos,Arg arg)
        {
            data[pos] = static_cast<unsigned char>(arg) & 0xFF ;
        }
    };

    ///数据帧长度固定为N,每一个数据占用的字节长度不等时调用此递归模板
    template<unsigned Byte,unsigned...RemainBytes>
    struct TransByProtocol
    {
        template<typename First,typename...Args> static void transImpl(ByteMode mode,unsigned char* data,unsigned pos,First first,Args...args)
        {
            for(unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (mode == Big) ? (Byte - i - 1) * CharBit : (i *CharBit );
                data[pos+i] = static_cast<unsigned char>(first >> offset) & 0xFF ;
            }
            TransByProtocol<RemainBytes...>::transImpl(mode,data,pos+Byte,std::forward<Args>(args)...);
        }

        template<typename Arg> static void transImpl(ByteMode mode,unsigned char* data,unsigned pos,Arg arg)
        {
            for(int unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (mode == Big) ? (Byte - i - 1) * CharBit : (i * CharBit );
                data[pos+i] = static_cast<unsigned char>(arg >> offset) & 0xFF ;
            }
        }
    };

};

class FrameCheck
{
    ///位反转模板函数
    template<typename T>
    static T reverse_bits(T value)
    {
        const size_t bits = sizeof(T) * CharBit;
        T reversed = 0;
        for (size_t i = 0; i < bits; ++i)
        {
            reversed = static_cast<T>(reversed << 1) | ((value >> i) & 1);
        }
        return reversed;
    }

    template<unsigned BytePerArg>
    static void writeCheckValue(ByteMode mode,unsigned char* data,DT<BytePerArg> check,int pos)
    {
        if(pos < 0)
            return ;

        for(unsigned index = 0; index < BytePerArg; index++)
        {
            unsigned char value = static_cast<unsigned char>(check >> (CharBit * index)) & 0xFF;
            if(mode == Big)
                data[pos+BytePerArg-index-1] = value;
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
         * note:这个模板最初只完成了针对8位及8位以上的crc校验,后续为了将不足8位的crc校验也兼容进来,参考https://github.com/whik/crc-lib-c 一边debug一边修改的
         * 所以我也不清楚有些地方为什么要那样写代码,代码中用note标记的地方就是我也没太想明白原理的部分, but! 代码能正常工作!
    */
    template<int crcbits,typename CrcDT,CrcDT polynomial,CrcDT init,CrcDT xorOut,bool refIn,bool refOut>
    static CrcDT crcImpl(unsigned char* data,unsigned start,unsigned end)
    {
        CrcDT crc =  init;
        CrcDT poly = polynomial;

        constexpr unsigned crcSize = sizeof (CrcDT) * CharBit;//crc变量总位数
        constexpr CrcDT topbit = static_cast<CrcDT>(1) << (crcSize - 1); //crc最高位为1时的值

        //如果crc位数不是8的整倍数,需要将crc右端补0
        constexpr int crcOffset = (crcbits % CharBit) ?  CharBit - (crcbits % CharBit) : 0;
        //对输入数据每一个字节右端补0
        constexpr int byteOffset = crcSize - CharBit;

        //如果crc位宽不是8的整倍数,需要对多项式和crc初始值左移crcOffset位
        poly =  static_cast<CrcDT>(poly << crcOffset);/**note**/
        crc = static_cast<CrcDT>(crc << crcOffset);/**note**/

        // 遍历数据
        for (unsigned i = start; i <= end; ++i)
        {
            unsigned char byte = refIn ? reverse_bits<unsigned char>(data[i]) : data[i];// 输入反转处理
            crc  = static_cast<CrcDT>( crc ^ (static_cast<CrcDT>(byte) << byteOffset) );// 将当前字节反转之后再进行左移和CRC对齐(相当于右端补零),随后与crc异或

            for (unsigned j = 0; j < CharBit; ++j)
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

        //输入不反转而且crc位数不为8的整数倍的而且左移过的结果需要将其右移回去
        crc = refIn ? crc : (crc >> crcOffset);/**note**/
        return crc;
    }

public:
    ///对给定的char数组计算校验和:计算给定数组data从start处开始到end处结尾所有字节的校验和,校验结果占用ResultByte字节大小,放置到数组pos处,Mode表明校验结果是大端存储还是小端存储
    ///当pos小于0时不会在源数据中写入校验结果(相当于仅仅计算并返回校验值)
    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> sum(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte>  sum = 0;
        for(; start <= end; ++start){
            sum += data[start];
        }

        writeCheckValue<ResultByte>(mode,data,sum,pos);
        return sum;
    }

    /// 对给定的char数组计算crc校验:计算给定数组data从start处开始到end处结尾所有字节的crc,校验结果占用ResultByte字节大小,放置到数组pos处,Mode表明校验结果是大端存储还是小端存储
    /// 当pos小于0时不会在源数据中写入校验结果(相当于仅仅计算并返回校验值)
    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc4_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<4,unsigned char,0x03,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc5_epc(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<5,unsigned char,0x09,0x09,0x00,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc5_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<5,unsigned char,0x15,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc5_usb(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<5,unsigned char,0x05,0x1F,0x1F,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc6_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<6,unsigned char,0x03,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc7_mmc(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<7,unsigned char,0x09,0x00,0x00,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc8(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x07,0x00,0x00,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc8_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x07,0x00,0x55,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc8_rohc(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x07,0xFF,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = oneByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,oneByte> crc8_maxim(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x31,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_ibm(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0x000,0x000,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_maxim(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0x0000,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_usb(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0xFFFF,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_modbus(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0xFFFF,0x0000,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_ccitt(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0x0000,0x0000,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_ccitt_false(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0xFFFF,0x0000,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_x25(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0xFFFF,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_xmodem(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0x0000,0x0000,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = twoByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,twoByte> crc16_dnp(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x3d65,0x0000,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = fourByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,fourByte> crc32(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x04C11DB7,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = fourByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,fourByte> crc32_c(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x01EDC6F41,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = fourByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,fourByte> crc32_koopman(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x741B8CD7,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = fourByte,ByteMode mode = Big>
    static FunctionReturn<ResultByte,fourByte> crc32_mpeg2(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x04C11DB7,0xFFFFFFFF,0x00000000,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }
};

#include <vector>
#include <QByteArray>

static void test()
{
    auto print = [](Frame& frame){
        qDebug()<<QByteArray(frame,frame.size()).toHex(' ').toUpper()<<sizeof (frame);
    };

    unsigned char arr[12] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC};
    Frame frameS(arr,12);
    print(frameS);

    FrameCheck::crc16_modbus(frameS,1,7,-1);

    FrameCheck::crc16_modbus(frameS,1,7,8);

    FrameCheck::crc16_modbus<4,Big>(frameS,1,8,9);

    int a = 10;
}

}

#endif // PARAMETERSERIALIZER_HPP
