#ifndef METAENUM_HPP
#define METAENUM_HPP

#include <QString>
#include <vector>
#include <QPair>
#include <set>

template<typename T>
struct EnumMap
{
    constexpr static bool registed = false;
    const std::vector<std::pair<T,QString>> map= {};
    //***如果没有在map中找到需要打印提示信息,有可能是字符串输入错误，也有可能是map中录入的键值对缺失或错误
};

template <typename Key,typename Value = QString,typename...Args>
static std::vector<std::pair<Key,Value>> generateEnumMap(Key key,Value value,Args...args)
{
    static_assert(sizeof...(Args) % 2 == 0, "Arguments must come in pairs: enum value followed by QString");
    return std::vector<std::pair<Key,Value>>{};
}

// 注册枚举的宏
#define RegMetaEnum(EnumType, ...)    \
template <>     \
struct EnumMap<EnumType> \
{    \
constexpr static bool registed = true;  \
const std::vector<std::pair<EnumType,QString>> map= generateEnumMap(__VA_ARGS__) ;\
};

enum Color{Red,Blue = 5,Green};
RegMetaEnum(Color,Red,QString("Red"),Blue,QString("Blue"),Green,QString("Green"))

//template<>
//struct EnumMap<Color>{
//    constexpr static bool registed = true;
//    const std::vector<std::pair<Color,QString>> map= {
//        std::pair<Color,QString>{Red,"Red"},
//        std::pair<Color,QString>{Blue,"Blue"},
//        std::pair<Color,QString>{Green,"Green"}
//    };
//};

enum Fruit{Apple,Banana,Orange};
RegMetaEnum(Fruit,Apple,QString("Apple"),Banana,QString("Banana"),Orange,QString("Orange"))

//template<>
//struct EnumMap<Fruit>{
//    constexpr static bool registed = true;
//    const std::vector<std::pair<Fruit,QString>> map= {
//        std::pair<Fruit,QString>{Apple,"Apple"},
//        std::pair<Fruit,QString>{Banana,"Banana"},
//        std::pair<Fruit,QString>{Orange,"Orange"}
//    };
//};

enum Math{Add,Divide,FFT};

template<typename T,typename std::enable_if<EnumMap<T>::registed>::type>
QString enumToString(T value)
{
    /**这里使用迭代器区查找就不要计算键值对的数量了**/
    EnumMap<T> emap;
    auto it = emap.map.cbegin();
    while (it != emap.map.cend())
    {
        if(it->first == value)
        {
            return it->second;
        }
        ++it;
    }

    return QString();
}

template<typename T,typename std::enable_if<!EnumMap<T>::registed>::type>
QString enumToString(T value)
{
    return QString::number(value);
}

template<typename T>
typename std::enable_if<EnumMap<T>::registed,T>::type
stringToEnum(const QString& str,T& arg)
{
    EnumMap<T> emap;
    auto it = emap.map.cbegin();
    while (it != emap.map.cend())
    {
        if(it->second == str)
        {
            arg = T(it->first);
        }
        ++it;
    }
}

template<typename T>
typename std::enable_if<!EnumMap<T>::registed,T>::type
stringToEnum(const QString& str,T& arg)
{
    arg = T(str.toInt());
}


#endif // METAENUM_HPP
