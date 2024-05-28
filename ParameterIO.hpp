#ifndef PARAMETERIO_HPP
#define PARAMETERIO_HPP

#include "StringConvertorQ.hpp"

#include <functional>

#include <QMap>
#include <QFile>
#include <QCoreApplication>
#include <QDomDocument>
#include <QXmlStreamWriter>

static const QString NodeType  = "Type";
static const QString AttName = "arg";

using namespace MetaUtility;

class ParameterIOException:public std::exception
{
public:
    ParameterIOException(const std::string& msg):message(msg){}
    virtual const char* what() const noexcept;
private:
    std::string message;
};

class ParameterIO
{
public:
    static ParameterIO* getInstance()
    {
        if(instance == nullptr)
            instance = new ParameterIO();
        return instance;
    }

    bool initParameterIO(const QString& filePath);//传入一个文件夹路径初始化

    void repair();//initParameterIO抛出异常之后调用这个函数

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
    }

    template<typename firstArg,typename...Args> void readArgs(QDomElement & node, firstArg& first, Args&...args);

    inline void readArgs(QDomElement & node)
    {
        Q_UNUSED(node);
        m_RecursionIndex = 0;
    }

private:
    static ParameterIO* instance;
    QFile m_TargetFile;
    QDomDocument m_Doc;
    IOmode m_Mode = {};
    int m_RecursionIndex = {};//递归索引，用于记录递归次数，并从xml节点中取出对应索引的值
    QString m_CurrentObjName = {};//对象名,用于表示当前在操作哪一个对象的数据
    QMap<QString,std::function<void()>> m_FuncMap;
};

template<typename firstArg, typename...Args>
void ParameterIO::readArgs(QDomElement &node, firstArg& first, Args&...args)
{
    //因为开始递归之前已经对参数包的数量和节点索引数量进行了判断,所以这里的索引一定不会超出节点属性的数量
    //但是这里还需要判断参数类型和节点保存下来的参数类型是否一致,因为变量的顺序可能被更改,类型一致的时候才读取数据
    QString name = node.childNodes().at(m_RecursionIndex).nodeName().remove(0,NodeType.length());
    if(typeid (first).name() == name)
    {
        QString valueStr = node.childNodes().at(m_RecursionIndex).attributes().item(0).nodeValue();
        convertStringToArg(valueStr,first);
    }

    //索引增加，进行下一次递归
    m_RecursionIndex++;
    readArgs(node,args...);
}

template<typename firstArg, typename...Args>
void ParameterIO::writeArgs(QDomElement &node, const firstArg &first, const Args&...args)
{
    //创建xml节点,节点名称不能以数字开头,否则无法正确读取,所以在设置节点名称的时候在前面加上Type
    QDomElement childNode = m_Doc.createElement(NodeType+typeid (first).name());
    QString argString = convertArgToString(first);//解析当前参数
    childNode.setAttribute(AttName,argString);//节点属性,在属性名称功能被添加之前都用AttName替代,因为一个节点只会有一个属性
    node.appendChild(childNode);//将节点节点插入到父节点
    writeArgs(node,args...);//解析下一个参数
}

template<typename...Args>
void ParameterIO::readXmlNode(const QString &objName, Args&...args)
{
    //获取根节点
    QDomElement root=m_Doc.documentElement();

    //从xml读取objName对应的节点名称
    QDomElement node = root.elementsByTagName(objName).item(0).toElement();
    if(node.isNull())
        return;

    //判断参数数量和节点属性数量是否一致,如果不一致则说明程序或者文件内容有改动,因此跳过对这个参数包的赋值
    if(sizeof... (args) == node.childNodes().count())
        readArgs(node,args...);//展开参数包,从xml节点查找对应的属性并写入到变量中
}

template<typename...Args>
void ParameterIO::writeXmlNode(const QString &objName, const Args&...args)
{
    QDomElement root=m_Doc.documentElement();//获取根节点

    //创建一个xml节点
    QDomElement  node = m_Doc.createElement(objName);
    writeArgs(node,args...);

    //查找是否存在同名节点
    QDomNodeList nodeList=root.elementsByTagName(objName);
    QDomElement oldNode = nodeList.at(0).toElement();//旧节点

    //判断是否已经存在同名节点，如果存在则替换，否则添加
    if(oldNode.isNull())
        root.appendChild(node);
    else
        root.replaceChild(node,oldNode);
}

template<typename...Args>
void ParameterIO::options(QString objName, Args&...args)
{
    switch (m_Mode)
    {
    case IO_Write:writeXmlNode(objName,args...);break;
    case IO_Read:readXmlNode(objName,args...);break;
    }
}

template<typename...Args>
void ParameterIO::registerArguments(const QString &objName, Args&...args)
{
    std::function<void()> func = [&,objName](){
        this->options(objName,args...);
    };
    m_FuncMap.insert(objName,func);
}

#endif // PARAMETERIO_HPP
