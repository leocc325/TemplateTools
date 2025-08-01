#ifndef MEMORYPOOLANY_H
#define MEMORYPOOLANY_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <list>
#include <unordered_map>
#include <iostream>
#include <atomic>
#include <thread>
/**
 * @brief The MemoryPoolAny class **需要加锁保证线程安全
 * MemoryPoolAny被分为一级分配器和二级分配器,一级分配器用来分配大于256字节的内存块,二级分配器用来分配小于256字节的内存,分配器分配的内存都会被添加到哈希表中复用
 *
 * 分配内存时优先从哈希表的空闲内存块中寻找可用的内存块,没有可用的内存块就从当前内存页中分配
 * 如果内存页剩余的空间不足以分配一个对应的内存块,则内存页剩余的空间按使用情况切割成块,添加到哈希表中,避免内存浪费
 * 随后新申请一个内存页,从新的内存页中分配内存块给申请方
 */
#define ALLOC

class MemoryPoolAny
{
    class SpinLock
    {
    public:
        void lock()
        {
            while (flag.test_and_set(std::memory_order_acquire)){}
        }

        void unlock()
        {
            flag.clear(std::memory_order_release);
        }
    private:
        std::atomic_flag flag;
    };

    enum {ALIGN = alignof(std::max_align_t)};

    enum {MAXBYTES = 256};//一级分配器和二级分配器的界限

    enum {PAGESIZE = 4096};//内存页的大小

public:
    MemoryPoolAny(){}

    ~MemoryPoolAny();

    MemoryPoolAny(const MemoryPoolAny&) = delete ;

    MemoryPoolAny(MemoryPoolAny&&);

    MemoryPoolAny& operator = (const MemoryPoolAny&) = delete ;

    MemoryPoolAny& operator = (MemoryPoolAny&&);

    template<typename T,typename...Args>
    typename std::enable_if<!std::is_void<T>::value , T*>::type
    construct(Args&&...args)
    {
        void* buf  = allocateBuffer<T>();
        if(buf != nullptr)
            ::new (buf) T(std::forward<Args>(args)...);
        return static_cast<T*>(buf);
    }

    template<typename T,typename...Args>
    typename std::enable_if<std::is_void<T>::value , T*>::type
    construct(Args&&...){return nullptr;}

    template<typename T>
    void destruct(T* obj)
    {

    }

private:
    void* allocateNewPage()
    {
        void* buffer = ::operator new(PAGESIZE);
        if(buffer == nullptr)
            throw std::bad_alloc();

        return buffer;
    }

    template<typename T,typename std::enable_if< (sizeof(T)>MAXBYTES) >::type>
    void* allocateBuffer()
    {
        return ::operator new(sizeof(T));//这里需要处理内存对齐
    }

    template<typename T,typename std::enable_if< (sizeof(T)<=MAXBYTES) >::type>
    void* allocateBuffer()
    {
        void* buf = findFromHashTable<T>();

        if(buf == nullptr)
            buf = findFromMemoryPage<T>();

        return buf;
    }

    ///从哈希表中查找一块合适的内存并返回
    template<typename T,std::size_t Size = sizeof(T)>
    void* findFromHashTable()
    {
        std::unordered_multimap<int,void*>::iterator it = freeBlocks.find(Size);
        if(it == freeBlocks.end())
        {
            return nullptr;
        }
        else
        {
            freeBlocks.erase(it);
            usedBlocks.insert(*it);
            return (*it).second;
        }
    }

    ///从内存页中查找一块合适的内存并且返回
    template<typename T,std::size_t Size = sizeof(T)>
    void* findFromMemoryPage()
    {
        //如果内存页剩余大小不足,则将内存页剩余空间切割挂载到哈希表中,切割完毕之后重新申请内存页
        if(PAGESIZE - offset < Size)
        {
            cutPage(PAGESIZE - offset);

            void* newPage = allocateNewPage();
            offset = 0;
            pages.push_back(newPage);
        }

        void* buf = pages.back();
        offset += Size;
        return buf;
    }

    ///将内存页切割,按2的N次幂切割,直到大小不足系统最小对齐字节
    void cutPage(std::size_t remainSize)
    {
        if(PAGESIZE - offset <= ALIGN)
        {
            void* block = static_cast<char*>(pages.back()) + offset;
            freeBlocks.emplace(PAGESIZE-offset,block);
        }
        else
        {
            std::size_t blockSize = sizeHelper(remainSize);

            void* block = static_cast<char*>(pages.back()) + offset;
            freeBlocks.emplace(PAGESIZE-offset,block);
            offset += blockSize;

            cutPage(PAGESIZE - offset);
        }
    }

    ///计算小于size的最大的2的N次幂的值,默认从系统最小对齐字节数开始查找
    std::size_t sizeHelper(const std::size_t target,std::size_t now = ALIGN)
    {
        if(now <= target && now*2 > target)
            return now;
        else
            return sizeHelper(target,now*2);
    }

    void printPoolStatus()
    {
        auto freeIt = freeBlocks.cbegin();
        auto usedIt = usedBlocks.cbegin();

        std::string info  = "FreeBlocks status:";
        while (freeIt != freeBlocks.cend())
        {
            int key = (*freeIt).first;
            if( key > MAXBYTES)
                break;

            int count = freeBlocks.count(key);
            info += std::to_string(key) + "-" + std::to_string(count) + "   ";
            ++freeIt;
        }
        std::cout<<info;

        info = "UsedBlock status";
        while (usedIt != usedBlocks.cend())
        {
            int key = (*usedIt).first;
            if( key > MAXBYTES)
                break;

            int count = usedBlocks.count(key);
            info += std::to_string(key) + "-" + std::to_string(count) + "   ";
            ++usedIt;
        }
        std::cout<<info;
    }

private:
    std::size_t offset = 0;//内存页地址偏移,用于计算当前内存页是空余大小
    std::vector<void*> pages;//内存页容器
    std::unordered_multimap<int,void*> freeBlocks;//空闲的内存块
    std::unordered_multimap<int,void*> usedBlocks;//使用中的内存块
};

#endif // MEMORYPOOLANY_H
