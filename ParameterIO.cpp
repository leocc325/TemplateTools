#include "ParameterIO.hpp"

#include <QTextStream>
#include <QDebug>
#include <QDir>

ParameterIO* ParameterIO::instance = nullptr;

const char *ParameterIOException::what() const noexcept
{
    return message.data();
}

bool ParameterIO::initParameterIO(const QString& filePath)
{
    //这个函数的目的只是为了从文件初始化QDomDocument,生成xml文件的工作是在writeXmlFile()函数中进行
    QFileInfo fileInfo(filePath);

    //判断给定的路径是否是文件夹
    if(!fileInfo.isDir())
        throw ParameterIOException("error:file path it's not a file folder");

    //生成文件名parameter.xml
    m_TargetFile.setFileName(filePath + "/parameter");

    //打开文件
    if(!m_TargetFile.open(QFile::ReadWrite | QFile::Text ))
        throw ParameterIOException("error: file open failed");

    //将xml文件读取到QDomDocument
    QString errorStr;
    int errorLine,errorCol;
    if(!m_Doc.setContent(&m_TargetFile,true,&errorStr,&errorLine,&errorCol))
    {
        QString error = "error: xml file load failed,  " + errorStr +"line: " + QString::number(errorLine) + "col: " + QString::number(errorCol);
        throw ParameterIOException(error.toStdString());
    }

    //这个函数抛出异常之后使用catch捕捉并调用repair()函数
    return true;
}

int ParameterIO::writeXmlFile()
{
    if(!m_TargetFile.exists())
        return false;

    m_Mode = IO_Write;
    QMapIterator<QString,std::function<void()>> it(m_FuncMap);
    while (it.hasNext())
    {
        it.next();
        m_CurrentObjName = it.key();//更新当前对象名称
        std::function<void()> write = it.value();

        write();
    }
    m_CurrentObjName.clear();

    if(m_TargetFile.open(QFile::WriteOnly|QFile::Truncate))
    {
        //输出到文件
        QTextStream out_stream(&m_TargetFile);
        m_Doc.save(out_stream,4); //缩进4格
        m_TargetFile.close();
        return true;
    }

    return false;
}

int ParameterIO::readXmlFile()
{
    if(!m_TargetFile.exists())
        return false;

    m_Mode = IO_Read;

    QMapIterator<QString,std::function<void()>> it(m_FuncMap);
    while (it.hasNext())
    {
        it.next();
        m_CurrentObjName = it.key();//更新当前对象名称
        std::function<void()> read = it.value();

        read();
    }
    m_CurrentObjName.clear();

    return true;
}

ParameterIO::ParameterIO()
{

}

void ParameterIO::repair()
{
    //生成一个包含文件头和根节点的空xml对象
    QDomProcessingInstruction xmlInstruction = m_Doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
    m_Doc.appendChild(xmlInstruction);

    QDomElement root = m_Doc.createElement("root");
    m_Doc.appendChild(root);
}
