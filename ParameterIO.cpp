#include "ParameterIO.hpp"

ParameterIO* ParameterIO::instance = nullptr;

void ParameterIO::initParameterIOTask()
{

}

int ParameterIO::writeXmlFile()
{
    m_Mode = IO_Write;
}

int ParameterIO::readXmlFile()
{
    m_Mode = IO_Read;
}

ParameterIO::ParameterIO()
{

}
