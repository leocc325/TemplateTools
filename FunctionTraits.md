# 这是一个FunctionTraits.hpp的使用说明

假设:有一个print函数用于打印数据数据
```c++
void print(unsigned char* buf){
  if(buf == nullptr)
    //打印输出:empty array
  else
    //打印输出:按16进制打印buf数组
}
```

## class Frame
Frame用于表示一个数据帧,这是一个unsigned char的包装器,也是各个模板函数的返回类型。 <br />
这个类不支持拷贝,支持移动赋值和移动构造,支持[]获取数据,支持隐式转换为char*、unsigned char*、void*。<br />
### Frame使用说明:<br />
1.默认构造函数Frame() <br />
生成一个空指针,Frame长度为0。 <br />
```c++
Frame frameA;
print(frameA); //输出: empty array
```

2.构造函数Frame(unsigned long long size) <br /> 
生成一个长度为size的unsigned char数组,数组仅被分配了空间,没有被初始化。<br />
```c++
Frame frameB(4);
print(frameB); //输出: 随机十六进制值
```

3.构造函数Frame(unsigned char* buf,unsigned long long size) <br />
传入一个unsigned char数组指针buf进行初始化,Frame会对buf进行拷贝,长度为size个字节。<br />
```
unsigned char buf[4] = {0x0A,0x0B,0x0C,0x0D}
Frame frameC(buf,4);
print(frameC); //输出: 0x0A 0x0B 0x0C 0x0D
```

4.构造函数Frame(char* buf,unsigned long long size) <br />
传入一个char数组指针buf进行初始化,Frame会对buf进行拷贝,长度为size个字节。<br />
```
char buf[4] = {0x01,0x02,0x03,0x04}
Frame frameD(buf,4);
print(frameD); //输出: 0x01 0x02 0x03 0x04
```

5.移动构造函数Frame(Frame&& other) <br />
```c++
Frame frameE(std::move(frameD));
print(frameD); //输出: empty array
print(frameE); //输出: 0x01 0x02 0x03 0x04
```

6.移动赋值运算符Frame& operator = (Frame&& other) <br />
```c++
Frame frameF;
frameF = std::move(frameE);
print(frameE); //输出: empty array
print(frameF); //输出: 0x01 0x02 0x03 0x04
```

7.[]获取数组某个索引上的引用 <br />
```c++
unsigned char ch_1 = frameF[1]; //ch_1 is oxo1
frameF[1] = 0x0A;
print(frameF); //输出: 0x01 0x0A 0x03 0x04
```

8.char*、unsigned char*、void*的隐式转换支持 <br />
```
void func1(char* buf){}
void func2(unsigned char* buf){}
void func3(void* buf){}

func1(frameF);//正确运行
func2(frameF);//正确运行
func3(frameF);//正确运行
```
9.unsigned char* data() 获取Frame数据成员指针 <br />

10.unsigned long long size() 获取Frame数据长度(字节) <br />

11.template<typename...T> static Frame combine(T&&...frames) 将若干个Frame按传入顺序拼接成完整的数据帧 <br />
```
Frame frameX; //假设 frameX = 0x0A 0x0B  0x0C 0x0D
Frame frameY; //假设 frameY = 0x01 0x02  0x03 0x04
Frame frameZ; //假设 frameZ = 0x05 0x06  0x07 0x08

Frame frameXYZ = Frame::combine(frameX,frameY,frameZ);
print(frameXYZ); //输出: 0x0A 0x0B  0x0C 0x0D 0x01 0x02  0x03 0x04 0x05 0x06  0x07 0x08
```

## class template<unsigned...BytePerArg>struct Trans

## FrameCheck

