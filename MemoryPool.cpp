#include "MemoryPool.hpp"

MemoryPool* MemoryPool::GlobalPool = new MemoryPool(4096);

MemoryPool::MemoryPool(std::size_t size)
{
    length = size;
    allocateNewBlock();
}

MemoryPool::~MemoryPool()
{
    std::vector<char*>::iterator it = poolVec.begin();
    while (it != poolVec.end())
    {
        ::operator delete(*it);
        ++it;
    }
}

void MemoryPool::allocateNewBlock()
{
    char* buf = static_cast<char*>(::operator new(length));
    if(buf)
    {
        pool = buf;
        offset = 0;
        poolVec.push_back(buf);
    }
}
