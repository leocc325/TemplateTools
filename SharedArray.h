#ifndef SHAREDARRAY_H
#define SHAREDARRAY_H

#include <QDebug>
#include <string.h>
#include <memory>

template <typename T>
class SharedArray
{
public:
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

    operator const T* () const noexcept
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
        if(data.use_count() > 1)
            detach();

        return (data.get()[index]);
    }

    const T& operator[] (std::size_t index) const
    {
        qDebug()<<"const []";
        return (data.get()[index]);
    }

private:
    void detach()
    {
        //detach无需判空,对空指针的写操作会直接导致段错误,所以如果能写就必定能拷贝
        qDebug()<<"detach";
        std::shared_ptr<T> newData = std::shared_ptr<T>(new T[size],std::default_delete<T[]>());
        memcpy(newData.get(),data.get(),sizeof (T) * size);
        data = newData;
    }
    public:
    std::shared_ptr<T> data = nullptr;
    size_t size = 0;
};

#endif // SHAREDARRAY_H
