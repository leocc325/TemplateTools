#ifndef PARAMETERSERIALIZER_HPP
#define PARAMETERSERIALIZER_HPP

#include <type_traits>
#include <utility>
#include <memory>
#include <vector>
#include <bitset>
#include <queue>
#include <string.h>

namespace FrameSerializer
{
namespace  {
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

    ///判断是否每一个变量都是整形,只支持内建的整形变量,不支持隐式转换:例如枚举变量、浮点值、或者其他可以隐式转换为整数的类型
    ///传入错误类型会导致编译报错,如果要传入这些变量,需要在传入之前自行转换类型为内建类型
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

    ///传入的通信协议是否是每个数据占用一个字节
    template<unsigned ClassArgNum,unsigned BytesLength,typename...Args>
    struct OneBytePerArg{
        static constexpr bool value = (ClassArgNum==1) && (BytesLength == sizeof... (Args)) && IsInteger<Args...>::value;
    };

    ///当前允许的校验结果最大字节数
    static constexpr unsigned maxCheckSize = 8;

    ///每一个字节的长度,固定为8位
    static constexpr unsigned CharBit = 8;

    enum DataSize{oneByte,twoByte,fourByte,eightByte};

    static constexpr  DataSize ByteWidthArray[maxCheckSize] = {oneByte,twoByte,fourByte,fourByte,eightByte,eightByte,eightByte,eightByte};

    ///根据当前数据所占字节数返回一个能容纳数据长度的变量类型
    template<unsigned Bytes>struct DataType{using type = void;};

    template<> struct DataType<oneByte>{using type = unsigned char;};

    template<> struct DataType<twoByte>{using type = unsigned short;};

    template<> struct DataType<fourByte>{using type = unsigned int;};

    template<> struct DataType<eightByte>{using type = unsigned long long;};

    ///别名模板,简化DataType的代码长度
    template<unsigned Bytes>
    using DT =  typename DataType<ByteWidthArray[Bytes]>::type;

    ///判断当前校验结果的长度是否超出8字节(一般不会用到超出8个字节的整形变量),超出8字节屏蔽模板
    template<unsigned Bytes>
    using FunctionReturn = typename std::enable_if<(Bytes <= maxCheckSize),DT<Bytes>>::type;
}

///数据帧
class Frame
{
    ///判断是否所有的参数都是Frame类型
    template<typename T,typename...Args>
    struct IsFrame{
        static constexpr bool value = std::is_same<T,Frame>::value && IsFrame<Args...>::value;
    };

    template<typename T>
    struct IsFrame<T>{
        static constexpr bool value = std::is_same<T,Frame>::value;
    };

    ///将所有的Frame放入一个容器,用于组合
    template<typename T,typename...Args>
    static unsigned long long frameVector(std::vector<Frame*>& vec,T& f,Args&...args)
    {
        unsigned long long size = f.size();
        vec.push_back(f);
        size += frameVector(vec,args...);
        return size;
    }

    template<typename T>
    static unsigned long long frameVector(std::vector<Frame*>& vec,T& f)
    {
        vec.push_back(f);
        return f.size();
    }

public:
    Frame(){}

    Frame(unsigned long long size)
    {
        m_Data = new unsigned char[size];
        this->m_Size = size;
    }

    Frame(unsigned char* buf,unsigned long long size)
    {
        m_Data = new unsigned char[size];
        memcpy(m_Data,buf,size);
        this->m_Size = size;
    }

    Frame(char* data,unsigned long long size)
    {
        m_Data = new unsigned char[size];
        memcpy(m_Data,data,size);
        this->m_Size = size;
    }

    ~Frame()
    {
        if(m_Data != nullptr)
        {
            delete [] m_Data;
            m_Data = nullptr;
        }
    }

    Frame(Frame&& other)
    {
        this->moveFrame(std::move(other));
    }

    Frame& operator = (Frame&& other)
    {
        this->moveFrame(std::move(other));
        return *this;
    }

    Frame(const Frame&) = delete;

    Frame& operator = (const Frame&) = delete ;

    unsigned char& operator [] (unsigned long long index) const
    {
        return m_Data[index];
    }

    operator unsigned char* () const noexcept
    {
        return m_Data;
    }

    operator void* () const noexcept
    {
        return m_Data;
    }

    operator char* () const noexcept
    {
        return reinterpret_cast<char*>(m_Data);
    }

    unsigned char* data() const noexcept
    {
        return this->m_Data;
    }

    unsigned long long size() const noexcept
    {
        return m_Size;
    }

    ///将帧头、数据包、帧尾组合成一个完整的帧
    ///HeadSize:帧头长度、DataSize:数据部分大小、TailSize:帧尾大小。单位全部为字节。
    static Frame combineFrame(const unsigned char* head,unsigned HeadSize,
                              const unsigned char* data,unsigned DataSize,
                              const unsigned char* tail,unsigned TailSize)
    {
        Frame frame(HeadSize+DataSize+TailSize);
        memcpy(frame.data(),head,HeadSize);
        memcpy(frame.data()+HeadSize,data,DataSize);
        memcpy(frame.data()+HeadSize+DataSize,tail,TailSize);
        return frame;
    }

    template<typename...T>
    static typename std::enable_if< IsFrame<std::decay_t<T>...>::value,Frame>::type
    combine(T&&...frames)
    {
        std::vector<Frame*> vec;
        unsigned long long totalSize = generate(vec,frames...);

        Frame frame(totalSize);
        unsigned long long index = 0;
        for(Frame* f : vec)
        {
            memcpy(frame.data() + index,f,f->size());
            index += f->size();
        }

        return frame;
    }

private:
    void moveFrame(Frame&& other)
    {
        this->m_Data = other.m_Data;
        this->m_Size = other.m_Size;
        other.m_Data = nullptr;
        other.m_Size = 0;
    }

private:
    unsigned char* m_Data = nullptr;
    unsigned long long m_Size = 0;
};

template<unsigned BytePerArg>
class CharConverter{

    ///将传入的容器转换为对应的char数组,容器必须存在size函数可获取容器内元素数量
    ///mode:数据大端表示还是小端表示,默认为大端
    ///BytePerData:每一个数据在转换之后占用多少个字节
    template<ByteMode mode = Big,typename T,template<typename...Element> class Array,typename...Args>
    static Frame setDataPack(const Array<T,Args...>& array)
    {
        std::size_t bufferSize = BytePerArg * array.size();
        Frame data(bufferSize);

        unsigned argIndex = 0;
        for(const T& value : array)
        {
            for(unsigned i = 0; i < BytePerArg; i++)
            {
                unsigned offset = (mode == Big) ? (BytePerArg - i - 1) * CharBit : (i *CharBit );
                data[argIndex*BytePerArg+i] = static_cast<unsigned char>(value >> offset) & 0xFF ;
            }
            ++argIndex;
        }

        return data;
    }

    template<ByteMode mode = Big,size_t N,typename T>
    static Frame setDataPack(const T(&array)[N])
    {
        std::size_t bufferSize = BytePerArg * N;
        Frame data(bufferSize);

        for(int i = 0; i < N; i++)
        {
            for(unsigned i = 0; i < BytePerArg; i++)
            {
                unsigned offset = (mode == Big) ? (BytePerArg - i - 1) * CharBit : (i *CharBit );
                data[i*BytePerArg+i] = static_cast<unsigned char>(array[i] >> offset) & 0xFF ;
            }
        }

        return data;
    }

    ///将指针数组转换为char数组
    template<ByteMode mode = Big,typename T>
    static Frame setDataPack(const T* array,std::size_t length)
    {
        std::size_t bufferSize = BytePerArg * length;
        Frame data(bufferSize);

        for(unsigned i = 0; i < length; i++)
        {
            for(unsigned i = 0; i < BytePerArg; i++)
            {
                unsigned offset = (mode == Big) ? (BytePerArg - i - 1) * CharBit : (i *CharBit );
                data[i*BytePerArg+i] = static_cast<unsigned char>(array[i] >> offset) & 0xFF ;
            }
        }

        return data;
    }
};

template<unsigned...BytePerArg>
class FrameFix
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
                unsigned offset = (mode == Big) ? (Byte - i - 1) * CharBit : (i *CharBit );
                data[pos+i] = static_cast<unsigned char>(first >> offset) & 0xFF ;
            }
            FixTrans<RemainBytes...>::transImpl(mode,data,pos+Byte,std::forward<Args>(args)...);
        }

        ///只剩下最后一个参数时递归模板终止
        template<typename Arg> static void transImpl(ByteMode mode,unsigned char* data,unsigned pos,Arg arg)
        {
            for(int unsigned i = 0; i < Byte; i++)
            {
                unsigned offset = (mode == Big) ? (Byte - i - 1) * CharBit : (i * CharBit );
                data[pos+i] = static_cast<unsigned char>(arg >> offset) & 0xFF ;
            }
        }
    };

public:
    ///总长度固定,单个数据长度可变的转换
    template<ByteMode Mode = Big,typename...Args>
    static typename std::enable_if<!OneBytePerArg<sizeof... (BytePerArg),Length<BytePerArg...>::value,Args...>::value,Frame>::type
    trans(Args...args)
    {
        static_assert (Matchable<sizeof... (BytePerArg),sizeof... (Args),Args...>::value,"error");

        Frame data(Length<BytePerArg...>::value);
        FixTrans<BytePerArg...>::transImpl(Mode,data,0,std::forward<Args>(args)...);
        return data;
    }

    ///总长度固定,单个数据长度1字节的转换
    template<ByteMode Mode = Big, typename...Args>
    static typename std::enable_if<OneBytePerArg<sizeof... (BytePerArg),Length<BytePerArg...>::value,Args...>::value,Frame>::type
    trans(Args...args)
    {
        static_assert (sizeof... (Args) == Length<BytePerArg...>::value,"error");

        Frame data(Length<BytePerArg...>::value);
        FixTransSingle<BytePerArg...>::transImpl(data,0,std::forward<Args>(args)...);
        return data;
    }
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
    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> sum(unsigned char* data,unsigned start,unsigned end,int pos)
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
    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc4_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<4,unsigned char,0x03,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc5_epc(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<5,unsigned char,0x09,0x09,0x00,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc5_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<5,unsigned char,0x15,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc5_usb(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<5,unsigned char,0x05,0x1F,0x1F,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc6_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<6,unsigned char,0x03,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc7_mmc(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<7,unsigned char,0x09,0x00,0x00,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc8(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x07,0x00,0x00,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc8_itu(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x07,0x00,0x55,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc8_rohc(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x07,0xFF,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 1,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc8_maxim(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<8,unsigned char,0x31,0x00,0x00,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_ibm(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0x000,0x000,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_maxim(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0x0000,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_usb(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0xFFFF,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_modbus(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x8005,0xFFFF,0x0000,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_ccitt(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0x0000,0x0000,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_ccitt_false(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0xFFFF,0x0000,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_x25(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0xFFFF,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_xmodem(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x1021,0x0000,0x0000,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 2,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc16_dnp(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<16,unsigned short,0x3d65,0x0000,0xFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 4,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc32(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x04C11DB7,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 4,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc32_c(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x01EDC6F41,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 4,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc32_koopman(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x741B8CD7,0xFFFFFFFF,0xFFFFFFFF,true,true>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }

    template <unsigned ResultByte = 4,ByteMode mode = Big>
    static FunctionReturn<ResultByte> crc32_mpeg2(unsigned char* data,unsigned start,unsigned end,int pos)
    {
        DT<ResultByte> crc =  crcImpl<32,unsigned int,0x04C11DB7,0xFFFFFFFF,0x00000000,false,false>(data,start,end);
        writeCheckValue<ResultByte>(mode,data,crc,pos);
        return crc;
    }
};

static void test(){
    Frame data = FrameFix<8>::trans<Big>(0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88);

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
