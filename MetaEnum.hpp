#ifndef METAENUM_HPP
#define METAENUM_HPP

#include <QString>
#include <vector>
#include <QPair>
#include <set>
#include <QDebug>

/**
 * @EnumMapBase:建议在EnumMapBase及其子类的任何对象时都使用static创建,避免重复调用其构造函数
 * 如果有特别的效率要求可以将映射容器从std::vector更改为QHash
*/
template<typename T>
struct EnumMapBase
{
    EnumMapBase()
    {
        map = new std::vector<std::pair<T,QString>>();
    }

    EnumMapBase(const std::vector<std::pair<T,QString>>& vec)
    {
        map = new std::vector<std::pair<T,QString>>(vec);
    }

    ~EnumMapBase()
    {
        delete map;
    }

    T toEnum(const QString& str)
    {
        auto it = map->cbegin();
        while (it != map->cend())
        {
            if(it->second == str)
            {
                return T(it->first);
            }
            ++it;
        }
        qWarning()<<"error:The corresponding enum value for the string was not found. "
                    "Please check the input string or the enum mapping table.";
        return T{};
    }

    QString toString(T value)
    {
        auto it = map->cbegin();
        while (it != map->cend())
        {
            if(it->first == value)
            {
                return it->second;
            }
            ++it;
        }
        qWarning()<<"The corresponding string for the enum value was not found. "
                    "Please check the input enum or the enum mapping table.";
        return QString();
    }

private:
    std::vector<std::pair<T,QString>>* map = nullptr;
};

template<typename T>
struct EnumMap : EnumMapBase<T>
{
    //用于判断时普通枚举还是注册了可以和自定义字符串转换的枚举
    constexpr static bool registed = false;
};

template <typename Key,typename Value = QString,typename...Args>
static std::vector<std::pair<Key,Value>>
generateEnumMap(const Key& key,const Value& value,Args...args)
{
    static_assert(sizeof...(Args) % 2 == 0, "Arguments must come in pairs: enum value followed by QString");

    const unsigned vecSize = (sizeof...(Args) / 2) + 1;
    std::vector<std::pair<Key,Value>> vec;
    vec.reserve(vecSize);
    generateImpl(vec,key,value,std::forward<Args>(args)...);
    return vec;
}

template <typename Key,typename Value = QString,typename...Args>
static std::vector<std::pair<Key,Value>>
generateImpl(std::vector<std::pair<Key,Value>>& vec,const Key& key,const Value& value,Args...args)
{
    vec.emplace_back(key,value);
    generateImpl(vec,std::forward<Args>(args)...);
    return vec;
}

template <typename Key,typename Value = QString>
static std::vector<std::pair<Key,Value>>
generateImpl(std::vector<std::pair<Key,Value>>& vec,const Key& key,const Value& value)
{
    vec.emplace_back(key,value);
    return vec;
}

/**
 * @RegMetaEnum:注册枚举的宏
 * 参数分别为:EnumType 枚举类名称
 *          ... 枚举变量、字符串、枚举变量、字符串、枚举变量、字符串...交替输入作为参数
 * 示例:
 * enum Color{Red,Blue = 5,Green};
 * RegMetaEnum(Color,Red,QString("Red"),Blue,QString("Blue"),Green,QString("Green"))
 */

#if 0 /**第一版宏**/
#define RegMetaEnum(EnumType, ...)    \
template <>     \
struct EnumMap<EnumType> \
{    \
constexpr static bool registed = true;  \
const std::vector<std::pair<EnumType,QString>> map = generateEnumMap(__VA_ARGS__) ;\
};
#else /**第二版宏**/
#define RegMetaEnum(EnumType, ...)    \
template<>  \
struct EnumMap<EnumType>:EnumMapBase<EnumType>{ \
EnumMap(): EnumMapBase<EnumType>(generateEnumMap(__VA_ARGS__)){}    \
constexpr static bool registed = true;  \
};
#endif

#endif // METAENUM_HPP
