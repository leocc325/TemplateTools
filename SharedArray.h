#ifndef SHAREDARRAY_H
#define SHAREDARRAY_H

#include <string.h>
#include <memory>
#include <vector>

/**
 *SharedArray是一个隐式共享数据类,通过拷贝、移动、赋值所创建的多个副本共享同一数据
 *任何可能修改数据的行为都会导致数据分离,创建独立的拷贝
 *在不使用SharedArray&或者SharedArray*的前提下,这个类是线程安全的
 */
template <typename T>
class SharedArray
{
public:
    typedef size_t size_type;
    typedef const T& const_reference;
    typedef T& reference;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T value_type;

    typedef T* iterator;
    typedef const T* const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    SharedArray(){}

    SharedArray(T* array,std::size_t length) noexcept
    {
        data = std::shared_ptr<T>(array);
        size = length;
    }

    SharedArray(const SharedArray& other) noexcept
    {
        data = other.data;
        size = other.size;
    }

    SharedArray(SharedArray&& other) noexcept
    {
        data = std::move(other.data);
        size = other.size;
    }

    SharedArray& operator = (const SharedArray& other) noexcept
    {
        data = other.data;
        size = other.size;
        return *this;
    }

    SharedArray& operator = (SharedArray&& other) noexcept
    {
        data = std::move(other.data);
        size = other.size;
    }

    void* operator new (size_t) = delete ;

    void* operator new[](size_t) = delete;

    inline operator T*() noexcept
    {
        detach();
        return data.get();
    }

    inline operator const T* () const noexcept
    {
        return data.get();
    };

    inline T& operator[] (std::size_t index)
    {
        detach();
        return (data.get()[index]);
    }

    inline const T& operator[] (std::size_t index) const
    {
        return (data.get()[index]);
    }

    inline iterator begin()
    {
        detach();
        return data.get();
    }

    inline const_iterator begin() const noexcept
    {
        return data.get();
    }

    inline const_iterator cbegin() const noexcept
    {
        return data.get();
    }

    inline iterator end()
    {
        detach();
        return data.get()+size;
    }

    inline const_iterator end() const
    {
        return data.get()+size;
    }

    inline const_iterator cend() const
    {
        return data.get()+size;
    }

    inline reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    inline reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    inline const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }

    inline const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    inline const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(end());
    }

    inline const_reverse_iterator crend() const
    {
        return const_reverse_iterator(begin());
    }

private:
    void detach()
    {
        //detach无需判空,对空指针的写操作会直接导致段错误,所以如果能写就必定能拷贝
        if(data.use_count() > 1)
        {
            std::shared_ptr<T> newData = std::shared_ptr<T>(new T[size],std::default_delete<T[]>());
            memcpy(newData.get(),data.get(),sizeof (T) * size);
            data = newData;
        }
    }

private:
    std::shared_ptr<T> data = nullptr;
    size_t size = 0;
};

#endif // SHAREDARRAY_H
