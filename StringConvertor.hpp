#ifndef STRINGCONVERTOR_H
#define STRINGCONVERTOR_H

#include <string>
#include <array>
#include <vector>
#include <list>
#include <deque>
#include <forward_list>
#include <sstream>
#include <type_traits>
#include <iostream>
#include <string.h>

namespace MetaUtility {

    ///数组分隔符
    static std::string spliter("[SP]");

    inline std::list<std::string> split(const std::string& input,const std::string& spliter)
    {
        std::list<std::string> tokens;
        size_t startPos = 0;
        size_t endPos = input.find(spliter,startPos);

        while (endPos != std::string::npos) {
            tokens.push_back(input.substr(startPos, endPos - startPos));
            startPos = endPos + spliter.size();
            endPos = input.find(spliter, startPos);
        }

        // 添加最后一个分隔符后的子串（或最后一个子串）
        tokens.push_back(input.substr(startPos, endPos));

        //如果最后一个字符串为空,则删除最后一个
        if(tokens.back().empty())
            tokens.pop_back();

        return tokens;
    }

    ///基础模板
    template<typename T,typename R = T>
    inline std::string convertArgToString(const T arg)
    {
        std::cout<<"unregistered type for convertArgToString";
        return std::string();
    }

    ///数字量转换为string
    template<typename T,typename std::enable_if<std::is_arithmetic<T>::value,T>::type>
    inline std::string convertArgToString(const T arg)
    {
        return std::to_string(arg);
    }

    ///枚举转换为string
    template<typename T,typename std::enable_if<std::is_enum<T>::value,T>::typ>
    inline std::string convertArgToString(const T arg)
    {
        return std::to_string(arg);
    }

    ///string转换为string
    template<typename T,typename std::enable_if<std::is_same<std::string,T>::value,T>::type>
    inline std::string convertArgToString(const T arg)
    {
        return arg;
    }

    ///判断T是否是char*,否则会和转换对象指针的模板产生二义性
    template<typename T>
    constexpr static bool IsCharPointer = std::is_same<const char*,T>::value || std::is_same<char*,T>::value;

    ///char*转换为string
    template<typename T,typename std::enable_if<IsCharPointer<T>,T>::type>
    inline std::string convertArgToString(const T arg)
    {
        return std::string(arg);
    }

    ///判断T是否是指针,同时不是char*
    template<typename T>
    constexpr static bool NonCharPointer = std::is_pointer<T>::value && !IsCharPointer<T>;

    ///class object pointer转换为string
    template<typename T,typename std::enable_if<NonCharPointer<T>, T>::type>
    inline std::string convertArgToString(const T obj)
    {
        std::stringstream ss;
        ss << (*obj);
        return ss.str();
    }

    ///容器转换为字符串
    template<typename T,template<typename...Element> class Array,typename...Args>
    inline std::string convertArgToString(const Array<T,Args...>& array)
    {
        std::string str;
        for(T item : array)
        {
            str.append( convertArgToString(item) );
            str.append(spliter);
        }
        return str;
    }

    ///数组转换为字符串
    template<size_t N,typename T>
    inline std::string convertArgToString(const T(&array)[N])
    {
        std::string str;
        for(T item : array)
        {
            str.append( convertArgToString(item) );
            str.append(spliter);
        }
        return str;
    }

    template<typename T>
    constexpr static bool IsIntegral = std::is_enum<T>::value || std::is_integral<T>::value;

    ///基础模板
    template <typename T,typename R = T>
    inline void convertStringToArg(const std::string& str,T& arg)
    {
        std::cout<<"unregistered type for convertStringToArg";
    }

    ///字符串转换为整数
    template <typename T,typename std::enable_if<IsIntegral<T>,T>::type>
    inline void convertStringToArg(const std::string& str,T& arg)
    {
        arg = static_cast<T>(std::stoll(str));
    }

    ///字符串转换为浮点型
    template<typename T,typename std::enable_if<std::is_floating_point<T>::value,T>::type>
    inline void convertStringToArg(const std::string& str,T& arg)
    {
        arg = static_cast<T>(std::stold(str));
    }

    ///字符串转换为char*
    inline void convertStringToArg(const std::string& str,char* arg)
    {
        char* data = new char[str.length()];
        memcpy(data,str.data(),str.length());

        arg  = data;
    }

    ///字符串转换为class object pointer
    template<typename T,typename std::enable_if<NonCharPointer<T*>, T>::type>
    inline void convertStringToArg(const std::string& str,T* obj)
    {
        std::stringstream in;
        in << str;
        in >> (*obj);
    }

    template<template<typename...> class Array,typename T,typename...Args>
    constexpr static bool IsSequenceContainer =
        std::is_same<Array<T,Args...>, std::list<T,Args...>>::value ||
        std::is_same<Array<T,Args...>,std::vector<T,Args...>>::value ||
        std::is_same<Array<T,Args...>,std::deque<T,Args...>>::value;

    ///字符串转换为容器
    template<typename T,typename...Args,template<typename...> class Array,
    typename std::enable_if<IsSequenceContainer<Array,T,Args...>,Array<T,Args...>>::type>
    inline void convertStringToArg(const std::string& str, Array<T,Args...>& array)
    {
        std::list<std::string> stringList = split(str,spliter.data());

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
    inline void  convertStringToArg(const std::string& str, std::array<T,N>& array)
    {
        std::list<std::string> stringList = split(str,spliter.data());

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
    template<typename T,std::size_t N,typename std::enable_if<std::is_array<T>::value,T>::type>
    inline void convertStringToArg(const std::string& str, T(&array)[N])
    {
        std::list<std::string> stringList = split(str,spliter.data());
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

#endif // STRINGCONVERTOR_H
