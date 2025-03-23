# TemplateTools
## 序言(Preface)
这是一个基于C++11/C++14的模板合集,里面包含了一些在泛型编程和元编程中可能会用到的一些工具,以及一些基于模板实现的功能,各个文件的功能和使用方法如下。 <br />
This is a collection of templates based on C++11/C++14, which includes some tools that may be used in generic programming and metaprogramming, as well as some features implemented using templates. The functions and usage methods of each file are as follows. <br />

## 0.testdemo
这个文件中包含了一些测试案例用于检测其他模板文件中的函数是否能正常运行。<br />
This file contains some test cases to check if the functions in other template files are working correctly.<br />

## 1.FunctionTraits
该模板用于获取普通函数、成员函数、std::function、callable等的函数类型、参数数量和参数类型。<br />
This template is used to obtain the function type, number of parameters, and parameter types of functions.The functions include:ordinary functions, member functions, std::function, callables, and so on.<br />

## 2.TypeList
该模板用于从一个参数包中提取单个参数类型或者子参数包,具体能力包括:获取参数包第一个参数、获取参数包最后一个参数、获取参数包前N个参数、获取参数包后N个参数。 <br />
This template is used to extract individual parameter types or sub-parameter packs from a parameter pack. Its specific capabilities include: obtaining the first parameter of the parameter pack, obtaining the last parameter of the parameter pack, obtaining the first N parameters of the parameter pack, and obtaining the last N parameters of the parameter pack.<br />

## 3.StringConvertor && StringConvertorQ
该模板用于将常用的变量和容器转换为字符串,或者将字符串转换为相应变量。 <br />
This template is used to convert common variables and containers into strings, or convert strings into corresponding variables. <br />

StringConvertor用于变量和std::string之间的转换,StringConvertorQ用于变量和QString之间的转换。<br />
StringConvertor is used for conversion between variables and std::string, while StringConvertorQ is used for conversion between variables and QString.<br />

该模板文件详细功能如下: <br />
1.将数值类型变量、枚举、bool、char*、重载了>>和<<运算符的对象以及上述所有类型的容器(链表、向量等)和数组转换为字符串 。<br />
2.将字符串转换为相应的数值类型变量、枚举、bool、char*、重载了>>和<<运算符的对象以及上述所有类型的容器(链表、向量等)和数组。<br />
3.文件中包含一个测试函数Test_StringConvertor用于测试字符串和变量之间的转换是否成功。 <br />
The detailed functionality of this template file is as follows: <br />
1.Convert numeric variables, enums, bool, char*, objects with overloaded >> and << operators, and containers (lists, vectors, etc.) and arrays of all the above types into strings.<br />
2.Convert strings into the corresponding numeric variables, enums, bool, char*, objects with overloaded >> and << operators, and containers (lists, vectors, etc.) and arrays of all the above types.<br />
3.The file contains a test function Test_StringConvertor() to verify whether the conversion between strings and variables is successful. <br />

## 4.ParameterIO
类ParameterIO用于将程序中的参数保存为xml文件,或者从xml文件中将参数读取到内存中,由此实现参数的保存和读取功能。<br />
Class ParameterIO is used to save parameters from the program to an XML file or read parameters from an XML file into memory, thereby enabling the functionality of saving and loading parameters. <br />

该文件只能在 Qt 框架的环境中执行。<br />
This file can only be executed in the Qt framework environment. <br />

## 5.CommandConsole
类CommandConsole类似于命令行、游戏控制台等指令工具,可以通过输入一个字符串完成函数调用,尤其是在程序发布以后，可以在exe程序运行时输入字符串修改内存中的变量或者查询目标变量值。<br />
The class CommandConsole is similar to command-line tools or game consoles, allowing functions to be called by entering a string. This is especially useful after the program is released, as it enables modification of variables in memory or querying the values of target variables during the execution of the executable program by entering strings.<br />

## 6.History
该模板用于实现程序中的撤销和重做功能。<br />
This template is used to implement undo and redo functionality in a program. <br />

## 7.Throttler && ThrottlePrivate.hpp
用于函数调用节流和防抖。<br />
This function is used to Throttler and Debounce.<br />

## DelayTask
这是一个延时任务类

## ThreadPool
这是一个线程池

## FrameSerializer
这是一个unsigned char数组序列化工具,主要可以用于各种通信协议的下的数据转换和组包。<br />
该模板文件组要由以下几个部分组成:<br />
1:Frame:用于表示一个数据帧,这是一个unsigned char的包装器,也是各个模板函数的返回类型
2:Trans:这个模板类主要用于将数组、容器转换为对应的数据帧，或者按照给定的通信协议将函数参数按要求转换为对应的数据帧,以及将多个数据帧拼接成一个完整的数据帧。
3:FrameCheck:用于对已有的unsigned char数组做数据校验,目前可以对数据做20多种crc校验以及和校验。
