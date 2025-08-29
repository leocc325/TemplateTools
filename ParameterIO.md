# 这是一个ParameterIO.hpp的使用说明

## 一：关于类ParameterIO的成员函数介绍
ParameterIO是一个参数保存和读取的类,用于将指定的参数保存为xml文件和读取。

### ParameterIO成员函数简介:<br />

#### 1.成员函数static ParameterIO* getInstance()
ParameterIO是一个单例类,调用这个函数获取ParameterIO对象指针。

#### 2.成员函数bool initParameterIO(const QString& filePath)
初始化,将给定参数文件路径代表的xml文件中的内容加载到内存中,这里只需要传入文件夹路径,程序会自动在这个文件夹内生成一个parameter.xml文件。

### 3.成员函数void repair()
这个函数会在初始化失败抛出异常的时候将xml内存重置为空xml文档,避免xml格式损坏无法保存参数。

### 4.成员函数template<typename...Args> void registerArguments(const QString& objName,Args&...args);
设置需要保存和读取的参数,第一个参数objName为这一组参数所组成的xml节点的名称。

### 5.成员函数int writeXmlFile()
将内存中的xml数据写入到文本中。

### 6.成员函数int readXmlFile()
将xml文件中的参数读取到内存中并将对应的数据写回到对应的变量中。

## 二：关于类ParameterIO的使用方法介绍
ParameterIO的成员函数调用存在以下限制:<br />
1.初始化函数需要在initParameterIO程序启动之后,业务初始化之前调用。<br />
2.参数的保存writeXmlFile()函数需要在程序即将结束,业务数据销毁之前调用。<br />
3.参数的读取需要在业务完全初始化(需要保存的数据所在业务的都初始化完成)之后调用。<br />
4.设置目标参数的函数registerArguments()可以在数据初始化之后,且能访问到这些数据的地方调用,例如构造函数、初始化成员函数或者对象被创建的地方。<br />

## 三：使用案例
```c++

class Object
{
public:
    Object(QString objName){
        ParameterIO::getInstance()->registerArguments(objName,age,height,name);
    };

public:
    int age = 0;
    double height = 0;
    QString name = "";
};

int main()
{
    try {
        ParameterIO::getInstance()->initParameterIO("D:/Desktop");
    } catch (const ParameterIOException& e) {
        ParameterIO::getInstance()->repair();
    }

    //创建两个Object对象
    Object obj1("Obj1");
    Object obj2("Obj2");

    //将文件中的数据读取到这两个对象中,如果文件为空或者查找不到对象对应的节点则什么都做
    ParameterIO::getInstance()->readXmlFile();

    //更改这两个对象内部的数据
    obj1.age = 10;
    obj1.height = 165;
    obj1.name = "hellow";

    obj2.age = 16;
    obj2.height = 180;
    obj2.name = "world";

    //将这两个对象的数据保存到文件中
    ParameterIO::getInstance()->writeXmlFile();
}
```

main函数执行完毕之后D:/Desktop文件夹内会存在一个名为parameter的xml文件,文件的内容为:
```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<root>
    <Obj1>
        <Typei arg="10"/>
        <Typed arg="165"/>
        <Type7QString arg="hellow"/>
    </Obj1>
    <Obj2>
        <Typei arg="16"/>
        <Typed arg="180"/>
        <Type7QString arg="world"/>
    </Obj2>
</root>
```

