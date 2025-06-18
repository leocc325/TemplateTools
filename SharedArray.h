#ifndef SHAREDARRAY_H
#define SHAREDARRAY_H

#include <QDebug>
#include <string.h>
#include <memory>
#include <vector>

template <typename T>
class SharedArray
{
public:
    // stl compatibility
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

    inline iterator begin();
    inline const_iterator begin() const;
    inline const_iterator cbegin() const;
    inline const_iterator constBegin() const;
    inline iterator end();
    inline const_iterator end() const;
    inline const_iterator cend() const;
    inline const_iterator constEnd() const;
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
    const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
    const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }


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

    inline operator const T* () const noexcept
    {
        //这个函数不需要detach,因为返回的是指针的拷贝而不是指针的引用
        //外部直接修改指针地址不会影响data所指向的地址
        return data.get();
    };

    void printUseCount() const
    {
        qDebug()<<"shared count:"<<data.use_count();
    }

    T& operator[] (std::size_t index)
    {
        return (data.get()[index]);
    }

    const T& operator[] (std::size_t index) const
    {
        return (data.get()[index]);
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
