#ifndef PARAMETERIO_HPP
#define PARAMETERIO_HPP

#include <functional>

#include <QMap>
#include <QFile>
#include <QDomDocument>
#include <QXmlStreamWriter>

class ParameterIO
{
public:
    static ParameterIO* getInstance()
    {
        if(instance == nullptr)
            instance = new ParameterIO();
        return instance;
    }

    void initParameterIOTask();//初始化

    template<typename...Args> void registerArguments(const QString& objName,Args&...args);//参数注册

    int writeXmlFile();//写xml文件

    int readXmlFile();//读xml文件

private:
    enum IOmode {IO_Write,IO_Read};

    ParameterIO();

    template<typename...Args> void options(QString objName, Args&...args);

    template<typename...Args> void writeXmlNode(const QString& objName,const Args&...args);

    template<typename...Args> void readXmlNode(const QString& objName,Args&...args);

    template<typename firstArg,typename...Args> void writeArgs(QDomElement & node,const firstArg& first, const Args&...args);

    inline void writeArgs(QDomElement & node)
    {
        Q_UNUSED(node);
        m_RecursionIndex = 0;
    }

    template<typename firstArg,typename...Args> void readArgs(QDomElement & node, firstArg& first, Args&...args);

    inline void readArgs(QDomElement & node)
    {
        Q_UNUSED(node);
        m_RecursionIndex = 0;
    }

private:
    static ParameterIO* instance;
    IOmode m_Mode = IO_Write;
    int m_RecursionIndex = 0;//递归索引，用于记录递归次数，并从xml节点中取出对应索引的值
    QFile m_TargetFile;
    QDomDocument m_Doc;
    QMap<QString,std::function<void()>> m_FuncMap;
};

template<typename...Args>
void ParameterIO::options(QString objName, Args&...args)
{
    switch (m_Mode)
    {
    case IO_Write:;break;
    case IO_Read:;break;
    }
}

template<typename...Args>
void ParameterIO::registerArguments(const QString &objName, Args&...args)
{
    std::function<void()> func = std::bind(&ParameterIO::options,this,objName,std::forward<Args>(args)...);
    m_FuncMap.insert(objName,func);
}

#endif // PARAMETERIO_HPP
