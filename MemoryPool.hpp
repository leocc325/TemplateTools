#ifndef MEMORYPOOL_HPP
#define MEMORYPOOL_HPP

#include <cstdint>
#include <vector>
#include <list>

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

    }

    template<typename...Args>
    void* allocate(Args&&...args)
    {
        void* ptr = freeBlocks.front();
        freeBlocks.pop_front();
        return ::new (ptr) T(std::forward<Args>(args)...);
    }

    void deallocate(void* ptr)
    {
        ::operator delete(ptr);
    }

private:
    void allocateNewBlock()
    {

    }

private:
    std::vector<void*> pools;
    std::list<void*> usedBlocks;
    std::list<void*> freeBlocks;
};

#endif // MEMORYPOOL_HPP
