#ifndef FRAMESERIALIZERPRIVATE_H
#define FRAMESERIALIZERPRIVATE_H

#include <type_traits>
#include <utility>
#include <vector>
#include <string.h>

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
        using FunctionReturn = typename std::enable_if<(Bytes < maxCheckSize) && (Bytes >= Min),DT<Bytes>>::type;
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
        static unsigned long long generateFrameVector( std::vector<Frame*>& vec,T& frame,Args&...args)
        {
            vec.push_back(&frame);
            unsigned long long size = frame.size() + generateFrameVector(vec,args...);
            return size  ;
        }

        template<typename T>
        static unsigned long long generateFrameVector(std::vector<Frame*>& vec,T& frame)
        {
            vec.push_back(&frame);
            return frame.size();
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

        template<typename...T>
        static typename std::enable_if< IsFrame<std::decay_t<T>...>::value,Frame>::type
        combine(T&&...frames)
        {
            std::vector<Frame*> vec;
            unsigned long long totalSize = generateFrameVector(vec,frames...);

            Frame frame(totalSize);

            unsigned long long index = 0;
            for(Frame* frameTmp : vec)
            {
                memcpy(frame.data() + index,frameTmp,frameTmp->size());
                index += frameTmp->size();
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

}

#endif // FRAMESERIALIZERPRIVATE_H
