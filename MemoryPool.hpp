#ifndef MEMORYPOOL_HPP
#define MEMORYPOOL_HPP

#include <cstdint>
#include <vector>
#include <list>
#include <unordered_map>
#include <iostream>

/**
 * @brief The MemoryPool class : 专门为消息包装器定制的内存池
 * 这是一个简陋的内存池,是为了代替::operator new分配地址,确保给***生命周期伴随整个程序运行***的小对象分配的内存是连续的,减少内存散布
 * 内存池会返回一个地址,为了避免内存频繁分配和回收,依然需要在外部配合placement new使用
 *
 * 临时对象不要使用这个内存池，这个内存池没有真正意义上实现内存复用
 */
class MemoryPool
{
public:
    static MemoryPool* GlobalPool;

    MemoryPool(std::size_t size = 1024);

    ~MemoryPool();

    template<typename T,typename...Args>
    T* allocate(Args&&...args)
    {
        if(sizeof(T) > length)
            return nullptr;

        //计算指针的内存对齐位置
        auto aligneCalculate = [this]()
        {
            uintptr_t addr = reinterpret_cast<uintptr_t>(pool+offset);
            if(addr % std::alignment_of<T>::value != 0)
                offset = offset + (std::alignment_of<T>::value - addr % std::alignment_of<T>::value);
        };
        aligneCalculate();

        if(offset + sizeof (T) > length)
        {
            allocateNewBlock();
            //重新分配内存之后要重新计算对齐位置
            aligneCalculate();
        }

        //如果重新分配内存块之后还是不够，则直接返回空指针
        if(offset + sizeof (T) <= length)
        {
            if(pool != nullptr)
            {
                void* address = pool + offset;
                offset += sizeof (T);
                return ::new (address) T(std::forward<Args>(args)...);
            }
        }

        return nullptr;
    }

    template<typename T> typename std::enable_if<!std::is_void<T>::value>::type
    deallocate(T*& ptr) const noexcept
    {
        if(ptr)
        {
            ptr->~T();
            //::operator delete(ptr);//这里不能delete指针,因为这个指针是placement new在内存块的这个地址上调用了构造函数,并没有分配内存
        }
    }

    template<typename T> typename std::enable_if<std::is_void<T>::value>::type
    deallocate(T*& ptr) const noexcept
    {
        //::operator delete(ptr);//这里不能delete指针,因为这个指针是placement new在内存块的这个地址上调用了构造函数,并没有分配内存
    }

private:
    void allocateNewBlock();

private:
    std::vector<char*> poolVec;
    char* pool = nullptr;
    std::size_t length = 0;
    std::size_t offset = 0;
};

template<typename T>
class Allocator
{
public:
    Allocator(std::size_t num)
    {
        poolSize = sizeof (T) * num;
        allocateNewBlock();
    }

    ~Allocator()
    {
        for(void* ptr : usedBlocks)
        {
            T* obj = static_cast<T*>(ptr);
            obj->~T();
        }

        for(void* pool : pools)
        {
            ::operator delete(pool);
        }
    }

    template<typename...Args>
    T* allocate(Args&&...args)
    {
        if(freeBlocks.empty())
            allocateNewBlock();

        T* ptr = static_cast<T*>(freeBlocks.front());
        freeBlocks.pop_front();
        usedBlocks.push_back(ptr);
        return ::new (ptr) T(std::forward<Args>(args)...);
    }

    void deallocate(T* ptr)
    {
        ptr->~T();
        usedBlocks.remove(ptr);
        freeBlocks.push_back(ptr);
    }

private:
    void allocateNewBlock()
    {
        void* buffer = nullptr;
        while (!buffer)
        {
            buffer = ::operator new(poolSize);//这里分配的内存有可能没有对齐
        }

        for(int i = 0; i < poolSize/sizeof (T); i++)
        {
            void* ptr = (static_cast<T*>(buffer) + i);
            freeBlocks.push_back(ptr);
        }
    }

private:
    std::size_t poolSize;//size of single pool
    std::vector<void*> pools;//collection of pool
    std::list<void*> usedBlocks;
    std::list<void*> freeBlocks;
};

/**
 * @brief The MemoryPoolAny class **需要加锁保证线程安全
 * MemoryPoolAny被分为一级分配器和二级分配器,一级分配器用来分配大于256字节的内存块,二级分配器用来分配小于256字节的内存,分配器分配的内存都会被添加到哈希表中复用
 *
 * 内存池每一次分配的池会按8,16,32,64,128,256的大小被切割成若干个内存块挂载到哈希表的桶中,由哈希表负责分配和回收内存块
 * 内存池在初始化时会给每个桶分配相同大小的内存,当某一个桶的内存块耗尽之后会再次向系统申请一个池
 * 内存池初始化申请内存的原则如下:
 * 内存块较大的桶获得的数量少,内存块较大的桶获得的数量多,例如初始化时给256对应的桶分配了10个内存块
 * 那么128对应的桶就分配20个,64对应的桶分配40个...以此类推,最后保证每一个桶中的内存块总大小一致
 *
 * 初始化之后的内存池申请大小是动态的,根据当前哈希表的内存块使用情况来确定新申请的内存池代大小
 * 内存池申请完毕再按照当前内存块的使用情况按比例切割成块,重新补充到哈希表中用于后续内存分配,后续内存申请原则如下:
 *
 * 1.新申请的内存池大小不少于初始化时分配的大小(为了尽可能保证内存的连续性减少内存碎片)
 * 2.对于耗尽的桶,将桶的大小拓展到现在的50%,根据被扩展的桶计算其他桶应该拓展的大小
 * 3.计算所有桶应该被拓展的大小
 */
class MemoryPoolAny
{
public:
    MemoryPoolAny();
    ~MemoryPoolAny();
    MemoryPoolAny(const MemoryPoolAny&) = delete ;
    MemoryPoolAny(MemoryPoolAny&&);
    MemoryPoolAny& operator = (const MemoryPoolAny&) = delete ;
    MemoryPoolAny& operator = (MemoryPoolAny&&);

    template<typename T,typename...Args,std::size_t Size = sizeof(T)>
    typename std::enable_if<(Size>256) , void*>::type
    construct(Args&&...args)
    {
        std::unordered_multimap<int,void*>::iterator it = freeBlocks.find(Size);
        if(it == freeBlocks.end())
        {

        }
        else
        {
            freeBlocks.erase(it);
            usedBlocks.insert(*it);
        }


    }

    template<typename T,typename...Args>
    typename std::enable_if< (sizeof(T) <= 256) , void* >::type
    construct(Args&&...args)
    {

    }

    template<typename T>
    void destruct(T* obj)
    {
        obj->~T();
    }

private:
    void allocateNewBlocks()
    {
        //
        //再次分配内存时根据每个大小内存块的使用比例计算新的空间大小,申请完成之后按比例切割内存块,将切割好的内存块分别挂到对应的桶
    }

    ///将需要分配的内存对齐到8的N倍,以便分配的内存块可以保存到哈希表
    ///调用这个函数的地方T的size必然不会大于256(模板里面已经判断过一次size的大小了)
    ///所以最后一定会退出而不是无限递归
    template<typename T>
    unsigned alignToBlocks(unsigned size = 8)
    {
        if(sizeof(T) > size)
            return alignToBlocks<T>(size * 2);
        else
            return size;
    }

    void printPoolStatus()
    {
        auto freeIt = freeBlocks.cbegin();
        auto usedIt = usedBlocks.cbegin();

        std::string info  = "FreeBlocks status:";
        while (freeIt != freeBlocks.cend())
        {
            int key = (*freeIt).first;
            if( key > 256)
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
            if( key > 256)
                break;

            int count = usedBlocks.count(key);
            info += std::to_string(key) + "-" + std::to_string(count) + "   ";
            ++usedIt;
        }
        std::cout<<info;
    }

private:
    std::unordered_map<void*,std::size_t> pool;//分配的内存池和池对应的大小,归还内存时需要判断内存地址是否属于内存池？这样更安全但是对效率有影响
    std::unordered_multimap<int,void*> freeBlocks;
    std::unordered_multimap<int,void*> usedBlocks;
};

#endif // MEMORYPOOL_HPP
