#ifndef FRAMESERIALIZERPRIVATE_H
#define FRAMESERIALIZERPRIVATE_H

#include <type_traits>
#include <utility>
#include <vector>
#include <string.h>

namespace FrameSerializer
{
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
                memcpy(frame.data() + index,frameTmp->data(),frameTmp->size());
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
