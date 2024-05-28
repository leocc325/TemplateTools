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

namespace MetaUtility {

    ///数组分隔符
    static std::string spliter("[SP]");

    std::list<std::string> split(const std::string& input,const std::string& spliter)
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

    ///数字量转换为string
    template<typename T,typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
    std::string convertArgToString(const T arg)
    {
        return std::to_string(arg);
    }

    ///枚举转换为string
    template<typename T,typename std::enable_if<std::is_enum<T>::value,T>::type* = nullptr>
    std::string convertArgToString(const T arg)
    {
        return std::to_string(arg);
    }

    ///string转换为string
    template<typename T,typename std::enable_if<std::is_same<std::string,T>::value,T>::type* = nullptr>
    std::string convertArgToString(const T arg)
    {
        return arg;
    }

    ///判断T是否是char*,否则会和转换对象指针的模板产生二义性
    template<typename T>
    constexpr static bool IsCharPointer = std::is_same<const char*,T>::value || std::is_same<char*,T>::value;

    ///char*转换为string
    template<typename T,typename std::enable_if<IsCharPointer<T>,T>::type* = nullptr>
    std::string convertArgToString(const T arg)
    {
        return std::string(arg);
    }

    ///判断T是否是指针,同时不是char*
    template<typename T>
    constexpr static bool NonCharPointer = std::is_pointer<T>::value && !IsCharPointer<T>;

    ///class object pointer转换为string
    template<typename T,typename std::enable_if<NonCharPointer<T>, T>::type* = nullptr>
    std::string convertArgToString(const T obj)
    {
        std::stringstream ss;
        ss << (*obj);
        return ss.str();
    }

    ///容器转换为字符串
    template<typename T,template<typename...Element> class Array,typename...Args>
    std::string convertArgToString(const Array<T,Args...>& array)
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
    std::string convertArgToString(const T(&array)[N])
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

    ///字符串转换为整数
    template <typename T,typename std::enable_if<IsIntegral<T>,int>::type* = nullptr>
    void convertStringToArg(const std::string& str,T& arg)
    {
        arg = static_cast<T>(std::stoll(str));
    }

    ///字符串转换为浮点型
    template<typename T,typename std::enable_if<std::is_floating_point<T>::value,T>::type* = nullptr>
    void convertStringToArg(const std::string& str,T& arg)
    {
        arg = static_cast<T>(std::stold(str));
    }

    ///字符串转换为char*
    void convertStringToArg(const std::string& str,char* arg)
    {
        char* data = new char[str.length()];
        memcpy(data,str.data(),str.length());

        arg  = data;
    }

    ///字符串转换为class object pointer
    template<typename T,typename std::enable_if<NonCharPointer<T*>, T>::type* = nullptr>
    void convertStringToArg(const std::string& str,T* obj)
    {
        std::stringstream in;
        in << str;
        in >> (*obj);
    }

    template<template<typename...> class Array,typename T,typename...Args>
    constexpr static bool IsSequenceContainer =
        std::is_same_v<Array<T,Args...>, std::list<T,Args...>> ||
        std::is_same_v<Array<T,Args...>,std::vector<T,Args...>>||
        std::is_same_v<Array<T,Args...>,std::deque<T,Args...>>;

    ///字符串转换为容器
    template<typename T,typename...Args,template<typename...> class Array,
    typename std::enable_if<IsSequenceContainer<Array,T,Args...>,Array<T,Args...>*>::type* = nullptr>
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
    template<typename T,std::size_t N,typename std::enable_if<std::is_array<T>::value,T>::type* = nullptr>
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

    class TestObject
    {
    public:
        TestObject()//默认构造函数
        {
            objName = "objName";
            dataLength = 10;
        }

        friend std::ostream& operator << (std::ostream& os,const TestObject& obj)
        {
            os << obj.objName <<" "<< obj.dataLength;
            return os;
        }


        friend std::istream& operator >> (std::istream& is,TestObject& obj)
        {
            is >> obj.objName >> obj.dataLength;
            return is;
        }

    public:
        std::string objName;
        unsigned dataLength;
    };

    bool Test_StringConvertor()
    {
        enum EnumType{AA,BB,CC};
        ///arg to string
        EnumType type_enum = CC;
        std::string str_enum = convertArgToString(type_enum);

        bool type_bool = false;
        std::string str_bool = convertArgToString(type_bool);

        char type_char = 'a';
        std::string str_char = convertArgToString(type_char);

        int type_int = -10;
        std::string str_int = convertArgToString(type_int);

        unsigned int type_uint = 10;
        std::string str_uint = convertArgToString(type_uint);

        float type_float = 1.23;
        std::string str_float = convertArgToString(type_float);

        double type_double = 2.34;
        std::string str_double = convertArgToString(type_double);

        const char* type_charp = "hellow";
        std::string str_charp = convertArgToString(type_charp);

        std::string type_string = "hellow";
        std::string str_string = convertArgToString(type_string);

        std::list<int> type_stdlist = {0,1,2,3,4};
        std::string str_stdlist = convertArgToString(type_stdlist);

        std::vector<int> type_stdvector = {2,3,4,5,6};
        std::string str_stdvector = convertArgToString(type_stdvector);

        int type_array[5] = {5,6,7,8,9};
        std::string str_array = convertArgToString(type_array);

        TestObject type_obj;
        type_obj.objName = "newName";
        type_obj.dataLength = 20;
        std::string str_obj = convertArgToString(&type_obj);

        ///string to arg
        EnumType to_enum;
        convertStringToArg(str_enum,to_enum);

        bool to_bool;
        convertStringToArg(str_bool,to_bool);

        int to_int;
        convertStringToArg(str_int,to_int);

        unsigned int to_uint;
        convertStringToArg(str_uint,to_uint);

        float to_float;
        convertStringToArg(str_float,to_float);

        double to_double;
        convertStringToArg(str_double,to_double);

        char* to_charp = nullptr;
        convertStringToArg(str_charp,to_charp);

        std::list<int,std::allocator<int>> to_stdlist;
        convertStringToArg(str_stdlist,to_stdlist);

        std::vector<int> to_stdvector;
        convertStringToArg(str_stdvector,to_stdvector);

        std::array<int,5> to_stdarray;
        convertStringToArg(str_array,to_stdarray);

        std::deque<int> to_stddeque;
        convertStringToArg(str_stdlist,to_stddeque);

        int to_array[5];
        convertStringToArg(str_array,to_array);

        Object to_obj;
        convertStringToArg(str_obj,&to_obj);
        return true;
    }
}

#endif // STRINGCONVERTOR_H
