#include "CommandConsole.hpp"
#include <QCoreApplication>
#include <QThread>

CommandConsole* CommandConsole::instance = nullptr;

CommandConsole *CommandConsole::getInstance()
{
    if(instance == nullptr)
        instance = new CommandConsole();
    return instance;
}

QString CommandConsole::handleCommand(const QString &cmd)
{
    //1:去除字符串中重复的空格
    QString cmdStr = cmd;

    cmdStr.replace(QRegExp("\\s+"), " ");

    //2:去除字符串中开头和结尾的空格
    cmdStr = cmdStr.trimmed();

    //3:按空格分割字符串
    argStringList = cmdStr.split(" ");

    //首先判断传入的指令是否至少包含了函数的key
    if(argStringList.count() < 1)
    {
        errorInfo = QString("error:no command detected");
        return "command error";
    }

    //取出第一个字符串查询对应的fucntion
    QString hashKey = argStringList.first();
    if(!funcHash.contains(hashKey.toLower()))
    {
        errorInfo = QString("error: wrong command:")+QString(hashKey);
        return "command error";
    }

    //取出后续的字符串准备做参数类型转换
    argStringList.removeFirst();

    std::function<void()> func = funcHash.value(hashKey.toLower());

    //在QT中,被注册到控制台的函数有可能是GUI文件中的函数,会直接与界面控件进行交互,如果这个函数(Console::handleCommand)在子线程中被调用，最终被注册的函数也
    CallType type = threadHash.value(hashKey.toLower());
    if(type == GuiThread)
    {
        QThread* GUI = qApp->thread();
        QMetaObject::invokeMethod(GUI,func);
    }
    else if(type == IndependentThread)
    {
        std::thread t(func);
        t.detach();
    }
    else
        func();

    return returnValue;
}
