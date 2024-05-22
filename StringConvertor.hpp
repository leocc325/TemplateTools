#ifndef STRINGCONVERTOR_H
#define STRINGCONVERTOR_H

#include <string>
#include <sstream>
#include <type_traits>

#include <QStringList>
class Object
{
public:
    Object()//默认构造函数
    {
        objName = "objName";
        dataLength = 10;
        data  = new int[dataLength];

        for(unsigned i = 0; i < dataLength ; i++)
        {
            data[i] = i + 10;
        }
    }

    friend std::ostream& operator << (std::ostream& os,const Object& obj)
    {
        std::string arrayString ;
        for(unsigned i = 0; i < obj.dataLength; i++)
        {
            arrayString += std::to_string(obj.data[i]);
            if(i < (obj.dataLength - 1) )
            {
                arrayString += "[SP]";
            }
        }
        os << arrayString<<" "<< obj.objName <<" "<< obj.dataLength;
        return os;
    }


    friend std::istream& operator >> (std::istream& is,Object& obj)
    {
        std::string arrayString ;

        is >> arrayString;
        QStringList list =  QString::fromStdString(arrayString).split("[SP]");
        for(int i = 0; i < list.count(); i++)
        {
            int value  =  list.at(i).toInt();
            obj.data[i] = value;
        }

        is >> obj.objName >> obj.dataLength;

        return is;
    }

private:
    int* data;
    std::string objName;
    unsigned dataLength;
};

namespace MetaUtility {

    ///数组分隔符
    static std::string spliter("[SP]");

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
    template<typename T,template<typename Element> class Array>
    std::string convertArgToString(const Array<T>& array)
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

    bool Test_StringConvertor()
    {
        enum Arity{AA = 10};
        std::string s_1 = convertArgToString(true);
        std::string s_2 = convertArgToString(char('a'));
        std::string s_3 = convertArgToString(10);
        std::string s_4 = convertArgToString(1.2);
        const std::string s_5 = convertArgToString(AA);
        std::string s_6 = convertArgToString(s_5);
        std::string s_7 = convertArgToString("aaaaaaaaa");
        Object* obj = new Object();
        std::string s_8 = convertArgToString(obj);
        return true;
    }
}

#endif // STRINGCONVERTOR_H
