#ifndef STRINGCONVERTOR_H
#define STRINGCONVERTOR_H

#define QT_ENV

#include <string>
#include <list>
#include <deque>
#include <forward_list>
#include <sstream>
#include <type_traits>

#ifdef QT_ENV
#include <QList>
#include <QVector>
#endif

class Object
{
public:
    Object()//默认构造函数
    {
        objName = "objName";
        dataLength = 10;
    }

    friend std::ostream& operator << (std::ostream& os,const Object& obj)
    {
        os << obj.objName <<" "<< obj.dataLength;
        return os;
    }


    friend std::istream& operator >> (std::istream& is,Object& obj)
    {
        std::string arrayString ;

        is >> arrayString;
        is >> obj.objName >> obj.dataLength;

        return is;
    }

private:
    std::string objName;
    unsigned dataLength;
};

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
        memcpy(arg,str.data(),str.length());
    }

    ///字符串转换为class object pointer
    template<typename T,typename std::enable_if<NonCharPointer<T*>, T>::type* = nullptr>
    void convertStringToArg(const std::string& str,T* obj)
    {
        std::stringstream in;
        in << str;
        in >> (*obj);
    }

#ifdef QT_ENV
    template<template<typename...> class Array,typename...Args>
    constexpr static bool IsSequenceContainer =
    std::is_same_v<Array<Args...>, std::list<Args...>> ||
    std::is_same_v<Array<Args...>,std::vector<Args...>>||
    std::is_same_v<Array<Args...>,std::deque<Args...>>||
    std::is_same_v<Array<Args...>,QList<Args...>>||
    std::is_same_v<Array<Args...>,QVector<Args...>>;
#else
    template<template<typename...> class Array,typename...Args>
    constexpr static bool IsSequenceContainer =
    std::is_same_v<Array<Args...>, std::list<Args...>> ||
    std::is_same_v<Array<Args...>,std::vector<Args...>>||
    std::is_same_v<Array<Args...>,std::deque<Args...>>;
#endif

    ///字符串转换为容器
    template<typename T,typename...Args,template<typename...> class Array>
    inline typename std::enable_if<IsSequenceContainer<Array,T,Args...>,void>::type
    convertStringToArg(const std::string& str, Array<T,Args...>& array)
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
        for(auto item : stringList)
        {
            if(index < N)
                convertArgToString(item,array[index]);
        }
    }

    ///字符串转换为数组
    template<typename T,std::size_t N,typename std::enable_if<std::is_array<T>::value,T>::type* = nullptr>
    inline void convertStringToArg(const std::string& str, T(&arg)[N])
    {
        std::list<std::string> stringList = split(str,spliter.data());
        ///读取到的参数数量和当前容器的大小不匹配的时候不进行参数读取工作
        if ( stringList.size() != N )
            return;

        T* ptr = arg;
        for(auto item : stringList)
        {
            convertStringToArg(item,arg++);
        }
    }

    bool Test_StringConvertor()
    {
        enum EnumType{AA,BB,CC};
        ///arg to string
        EnumType type_enum = CC;
        std::string str_001 = convertArgToString(type_enum);

        bool type_bool = false;
        std::string str_oo2 = convertArgToString(type_bool);

        char type_char = 'a';
        std::string str_003 = convertArgToString(type_char);

        int type_int = -10;
        std::string str_004 = convertArgToString(type_int);

        unsigned int type_uint = 10;
        std::string str_005 = convertArgToString(type_uint);

        float type_float = 1.23;
        std::string str_006 = convertArgToString(type_float);

        double type_double = 2.34;
        std::string str_007 = convertArgToString(type_double);

        const char* type_charp = "hellow";
        std::string str_008 = convertArgToString(type_charp);

        std::string type_string = "hellow";
        std::string str_009 = convertArgToString(type_string);

        std::list<int> type_list = {0,1,2,3,4};
        std::string str_010 = convertArgToString(type_list);

        std::vector<int> type_vector = {2,3,4,5,6};
        std::string str_011 = convertArgToString(type_vector);

        int type_array[5] = {5,6,7,8,9};
        std::string str_012 = convertArgToString(type_array);

        QObject type_obj;
        std::string str_013 = convertArgToString(&type_obj);

        ///string to arg
        EnumType to_enum;
        bool to_bool;
        int to_int;
        unsigned int to_uint;
        float to_float;
        double to_double;
        char* to_charp;
        return true;
    }

    /*
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
*/
}

#endif // STRINGCONVERTOR_H
