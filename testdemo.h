#ifndef TESTDEMO_H
#define TESTDEMO_H

#define SC_SWITCH 1

#include "TypeList.hpp"
#if SC_SWITCH
#include "StringConvertor.hpp"
#else
#include "StringConvertorQ.hpp"
#endif

using namespace MetaUtility;

class TestObject
{
public:
    TestObject()
    {
        objName = "objName";
        dataLength = 10;
    }

    friend std::ostream& operator << (std::ostream& os,const TestObject& obj)
    {
        os << obj.objName <<" "<< obj.dataLength;
        return os;
    }

    friend std::istream& operator >> (std::istream& is,TestObject& obj)
    {
        is >> obj.objName >> obj.dataLength;
        return is;
    }

public:
    std::string objName;
    unsigned dataLength;
};

bool Test_TypeList()
{
    auto tpA_0 = GetNthType<2,int,bool,double,float>::type();
    auto tpA_1 = AppendType<TypeList<>,int>::type();
    auto tpA_2 = AppendType<TypeList<int>,double>::type();
    auto tpA_3 = PrependType<TypeList<int>,double>::type();
    auto tpA_4 = RemoveFirstType<TypeList<int,double,float>>::type();
    auto tpA_5 = RemoveFirstType<TypeList<int>>::type();

    auto tpB_1 = FrontArgsHelper<0,TypeList<>,int,double,float,bool,char>::type();
    auto tpB_2 = FrontArgsHelper<1,TypeList<>,int,double,float,bool,char>::type();
    auto tpB_3 = FrontArgsHelper<3,TypeList<>,int,double,float,bool,char>::type();
    auto tpB_4 = FrontArgsHelper<4,TypeList<>,int,double,float,bool,char>::type();

    auto tpC_1 = BehindArgsHelper<0,TypeList<int,double,float,bool,char>>::type();
    auto tpC_2 = BehindArgsHelper<1,TypeList<int,double,float,bool,char>>::type();
    auto tpC_3 = BehindArgsHelper<3,TypeList<int,double,float,bool,char>>::type();
    auto tpC_4 = BehindArgsHelper<4,TypeList<int,double,float,bool,char>>::type();

    auto tpD_1 = FrontArgs<0,int,double,bool,float>::type();
    auto tpD_2 = FrontArgs<4,int,double,bool,float>::type();
    auto tpD_3 = FrontArgs<1,int,double,bool,float>::type();
    auto tpD_4 = FrontArgs<3,int,double,bool,float>::type();

    auto tpE_1 = BehindArgs<0,int,double,bool,float>::type();
    auto tpE_2 = BehindArgs<1,int,double,bool,float>::type();
    auto tpE_3 = BehindArgs<2,int,double,bool,float>::type();
    auto tpE_4 = BehindArgs<4,int,double,bool,float>::type();

    return true;
}

#if SC_SWITCH
bool Test_StringConvertor()
{
    enum EnumType{AA,BB,CC};
    ///arg to string
    EnumType type_enum = CC;
    std::string str_enum = convertArgToString(type_enum);

    bool type_bool = false;
    std::string str_bool = convertArgToString(type_bool);

    char type_char = 'a';
    std::string str_char = convertArgToString(type_char);

    int type_int = -10;
    std::string str_int = convertArgToString(type_int);

    unsigned int type_uint = 10;
    std::string str_uint = convertArgToString(type_uint);

    float type_float = 1.23;
    std::string str_float = convertArgToString(type_float);

    double type_double = 2.34;
    std::string str_double = convertArgToString(type_double);

    const char* type_charp = "hellow";
    std::string str_charp = convertArgToString(type_charp);

    std::string type_string = "hellow";
    std::string str_string = convertArgToString(type_string);

    std::list<int> type_stdlist = {0,1,2,3,4};
    std::string str_stdlist = convertArgToString(type_stdlist);

    std::vector<int> type_stdvector = {2,3,4,5,6};
    std::string str_stdvector = convertArgToString(type_stdvector);

    int type_array[5] = {5,6,7,8,9};
    std::string str_array = convertArgToString(type_array);

    TestObject type_obj;
    type_obj.objName = "newName";
    type_obj.dataLength = 20;
    std::string str_obj = convertArgToString(&type_obj);

    ///string to arg
    EnumType to_enum;
    convertStringToArg(str_enum,to_enum);

    bool to_bool;
    convertStringToArg(str_bool,to_bool);

    int to_int;
    convertStringToArg(str_int,to_int);

    unsigned int to_uint;
    convertStringToArg(str_uint,to_uint);

    float to_float;
    convertStringToArg(str_float,to_float);

    double to_double;
    convertStringToArg(str_double,to_double);

    char* to_charp = nullptr;
    convertStringToArg(str_charp,to_charp);

    std::list<int,std::allocator<int>> to_stdlist;
    convertStringToArg(str_stdlist,to_stdlist);

    std::vector<int> to_stdvector;
    convertStringToArg(str_stdvector,to_stdvector);

    std::array<int,5> to_stdarray;
    convertStringToArg(str_array,to_stdarray);

    std::deque<int> to_stddeque;
    convertStringToArg(str_stdlist,to_stddeque);

    int to_array[5];
    convertStringToArg(str_array,to_array);

    TestObject to_obj;
    convertStringToArg(str_obj,&to_obj);
    return true;
}
#else
void Test_StringConvertorQ()
{

}
#endif

#endif // TESTDEMO_H
