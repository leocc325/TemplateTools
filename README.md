# TemplateTools
## 序言(Preface)
这是一个基于C++11/C++14的模板合集,里面包含了一些在泛型编程和元编程中可能会用到的一些工具,以及一些基于模板实现的功能,各个文件的功能和使用方法如下。 <br />
This is a collection of templates based on C++11/C++14, which includes some tools that may be used in generic programming and metaprogramming, as well as some features implemented using templates. The functions and usage methods of each file are as follows. <br />

## 1.FunctionTraits
该模板用于获取普通函数、成员函数、std::function、callable等的函数类型、参数数量和参数类型。<br />
This template is used to obtain the function type, number of parameters, and parameter types of functions.The functions include:ordinary functions, member functions, std::function, callables, and so on.<br />

## 2.TypeList
该模板用于从一个参数包中提取单个参数类型或者子参数包,具体能力包括:获取参数包第一个参数、获取参数包最后一个参数、获取参数包前N个参数、获取参数包后N个参数。 <br />
This template is used to extract individual parameter types or sub-parameter packs from a parameter pack. Its specific capabilities include: obtaining the first parameter of the parameter pack, obtaining the last parameter of the parameter pack, obtaining the first N parameters of the parameter pack, and obtaining the last N parameters of the parameter pack.<br />

## 3.StringConvertor
该模板用于将常用的变量和容器转换为字符串,或者将字符串转换为相应变量。 <br />
This template is used to convert common variables and containers into strings, or convert strings into corresponding variables. <br />
