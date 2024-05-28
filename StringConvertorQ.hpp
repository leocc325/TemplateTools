#ifndef STRINGCONVERTORQ_HPP
#define STRINGCONVERTORQ_HPP

#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <list>
#include <deque>
#include <forward_list>

#include <QList>
#include <QVector>
#include <QStringList>
#include <QMetaEnum>
#include <QRegularExpression>

namespace MetaUtility {

    ///数组分隔符
    static QString spliter("[SP]");

    ///数字量转换为string
    template<typename T,typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    inline QString convertArgToString(const T arg)
    {
        return QString::number(arg);
    }

    ///枚举转换为string
    template<typename T,typename std::enable_if<std::is_enum<T>::value,T>::type* = nullptr>
    inline QString convertArgToString(const T arg)
    {
        return QString::number(arg);
    }

    ///string转换为string
    template<typename T,typename std::enable_if<std::is_same<QString,T>::value,T>::type* = nullptr>
    inline QString convertArgToString(const T arg)
    {
        return arg;
    }

    ///判断T是否是char*,否则会和转换对象指针的模板产生二义性
    template<typename T>
    constexpr static bool IsCharPointer = std::is_same<const char*,T>::value || std::is_same<char*,T>::value;

    ///char*转换为string
    template<typename T,typename std::enable_if<IsCharPointer<T>,T>::type* = nullptr>
    inline QString convertArgToString(const T arg)
    {
        return QString(arg);
    }

    ///判断T是否是指针,同时不是char*
    template<typename T>
    constexpr static bool NonCharPointer = std::is_pointer<T>::value && !IsCharPointer<T>;

    ///class object pointer转换为string
    template<typename T,typename std::enable_if<NonCharPointer<T>, T>::type* = nullptr>
    inline QString convertArgToString(const T obj)
    {
        std::stringstream ss;
        ss << (*obj);
        return QString::fromStdString(ss.str());
    }

    ///容器转换为字符串
    template<typename T,template<typename...Element> class Array,typename...Args>
    inline QString convertArgToString(const Array<T,Args...>& array)
    {
        QString str;
        for(T item : array)
        {
            str.append( convertArgToString(item) );
            str.append(spliter);
        }
        return str;
    }

    ///数组转换为字符串
    template<size_t N,typename T>
    inline QString convertArgToString(const T(&array)[N])
    {
        QString str;
        for(T item : array)
        {
            str.append( convertArgToString(item) );
            str.append(spliter);
        }
        return str;
    }

    ///字符串转换为整数
    template <typename T,typename std::enable_if<std::is_integral<T>::value,T>::type* = nullptr>
    inline void convertStringToArg(const QString& str,T& arg)
    {
        arg = static_cast<T>(str.toLongLong());
    }

    ///字符串转枚举
    template<typename T,typename std::enable_if<std::is_enum<T>::value,T>::type* = nullptr >
    inline bool convertStringToArg(const QString& str,  T& arg)
    {
        //从字符串往枚举转换时分两种情况：1.数值类型的字符串转换为枚举 2.枚举名称对应的字符串转换为枚举变量(Q_Enum和对应字符串之间的转换)
        //查阅QMetaEnum源码的时候在fromType()函数中发现了QtPrivate::IsQEnumHelper<T>::Value这个模板
        return convertStringToEnum(std::integral_constant<bool,QtPrivate::IsQEnumHelper<T>::Value>{},str ,arg);
    }

    template<typename T>
    inline bool convertStringToEnum(std::true_type,const QString& str,  T& arg)
    {
        //当枚举变量为QMetaEnum时，将字符串读取为枚举需要区分两种情况
        //枚举转换为字符串的时候是没有区分枚举类型的，无论是QMetaEnum还是enum都被转换成了对应的数字，但是在字符串转换成QMetaEnum的时候存在两种情况
        //一种是从xml中读取QMetaEnum，此时的字符串时纯数值类型的字符串，还有一种情况就是控制台或者SCPI命令中的QMetaEnum字符串，此时的字符串就是枚举变量名对应的字符串
        //因此在处理QMetaEnum字符串的时候，要考虑到这两种情况，确保两种类型的QMetaEnum字符串都能被正确转换
        //首先判断字符串是否只由正负号和数字组成，如果不是，再尝试通过QMetaEnum将其转换为枚举，如果转换失败，则返回false
        QRegularExpression reg("^-?\\d*(\\.\\d+)?$");
        if(reg.match(str).hasMatch())
        {
            arg = static_cast<T>(str.toInt());
            return true;
        }
        else
        {
            QMetaEnum meta = QMetaEnum::fromType<T>();
            int ret = meta.keyToValue(str.toStdString().data());
            if(ret != -1)
            {
                arg = static_cast<T>(ret);
                return true;
            }
            else
                return false;
        }
    }

    ///字符串转Enum
    template<typename T>
    inline bool convertStringToEnum(std::false_type,const QString& str,  T& arg)
    {
        arg = static_cast<T>(str.toInt());
        return true;
    }

    ///字符串转换为浮点型
    template<typename T,typename std::enable_if<std::is_floating_point<T>::value,T>::type* = nullptr>
    inline void convertStringToArg(const QString& str,T& arg)
    {
        arg = static_cast<T>(str.toDouble());
    }

    inline bool convertStringToArg(const QString& str, QString& arg)
    {
        arg = str;
        return true;
    }

    ///字符串转换为char*
    inline void convertStringToArg(const QString& str,char* arg)
    {
        char* data = new char[str.length()];
        memcpy(data,str.toStdString().data(),str.length());

        arg  = data;
    }

    ///字符串转换为class object pointer
    template<typename T,typename std::enable_if<NonCharPointer<T*>, T>::type* = nullptr>
    inline void convertStringToArg(const QString& str,T* obj)
    {
        std::stringstream in;
        in << str.toStdString();
        in >> (*obj);
    }


    template<template<typename...> class Array,typename T,typename...Args>
    constexpr static bool IsSequenceContainer =
        std::is_same_v<Array<T,Args...>, std::list<T,Args...>> ||
        std::is_same_v<Array<T,Args...>,std::vector<T,Args...>>||
        std::is_same_v<Array<T,Args...>,std::deque<T,Args...>>||
        std::is_same_v<Array<T>,QList<T>>||
        std::is_same_v<Array<T>,QVector<T>>;

    ///字符串转换为容器
    template<typename T,typename...Args,template<typename...> class Array,
    typename std::enable_if<IsSequenceContainer<Array,T,Args...>,Array<T,Args...>*>::type* = nullptr>
    inline void convertStringToArg(const QString& str, Array<T,Args...>& array)
    {
        QStringList stringList = str.split(spliter);

        Array<T,Args...> tempArray;
        for(auto& item : stringList)
        {
            T value;
            convertStringToArg(item,value);
            tempArray.push_back(value);
        }
        array = std::move(tempArray);
    }

    ///字符串转换为array
    template<typename T,size_t N>
    inline void  convertStringToArg(const QString& str, std::array<T,N>& array)
    {
        QStringList stringList = str.split(spliter);

        int index = 0;
        T value;
        for(auto& item : stringList)
        {
            if(index < N)
            {
                convertStringToArg(item,value);
                array[index] = value;
                index++;
            }
        }
    }

    ///字符串转换为数组
    template<typename T,std::size_t N,typename std::enable_if<std::is_array<T>::value,T>::type* = nullptr>
    inline void convertStringToArg(const QString& str, T(&array)[N])
    {
        QStringList stringList = str.split(spliter);
        ///读取到的参数数量和当前容器的大小不匹配的时候不进行参数读取工作
        if ( stringList.size() != N )
            return;

        int index = 0;
        T value;
        for(auto& item : stringList)
        {
            convertStringToArg(item,value);
            array[index] = value;
            index++;
        }
    }
}

#endif // STRINGCONVERTORQ_HPP
