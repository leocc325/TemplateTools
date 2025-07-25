# 这是一个FrameSerializer.hpp的使用说明

假设:有一个print函数用于打印数据数据
```c++
void print(unsigned char* buf){
  if(buf == nullptr)
    //打印输出:empty array
  else
    //打印输出:按16进制打印buf数组
}
```

## 一：关于类 Frame 的成员函数和使用方法介绍
Frame用于表示一个数据帧,这是一个unsigned char的包装器,也是各个模板函数的返回类型。 <br />
这个类不支持拷贝,支持移动赋值和移动构造,支持[]获取数据,支持隐式转换为char*、unsigned char*、void*。<br />

### Frame使用说明:<br />
#### 1.默认构造函数Frame()
生成一个空指针,Frame长度为0。 <br />
```c++
Frame frameA;
print(frameA); //输出: empty array
```

#### 2.构造函数Frame(unsigned long long size)
生成一个长度为size的unsigned char数组,数组仅被分配了空间,没有被初始化。<br />
```c++
Frame frameB(4);
print(frameB); //输出: 随机十六进制值
```

#### 3.构造函数Frame(unsigned char* buf,unsigned long long size)
传入一个unsigned char数组指针buf进行初始化,Frame会对buf进行拷贝,长度为size个字节。<br />
```
unsigned char buf[4] = {0x0A,0x0B,0x0C,0x0D}
Frame frameC(buf,4);
print(frameC); //输出: 0x0A 0x0B 0x0C 0x0D
```

#### 4.构造函数Frame(char* buf,unsigned long long size)
传入一个char数组指针buf进行初始化,Frame会对buf进行拷贝,长度为size个字节。<br />
```
char buf[4] = {0x01,0x02,0x03,0x04}
Frame frameD(buf,4);
print(frameD); //输出: 0x01 0x02 0x03 0x04
```

#### 5.移动构造函数Frame(Frame&& other)
```c++
Frame frameE(std::move(frameD));
print(frameD); //输出: empty array
print(frameE); //输出: 0x01 0x02 0x03 0x04
```

#### 6.移动赋值运算符Frame& operator = (Frame&& other) 
```c++
Frame frameF;
frameF = std::move(frameE);
print(frameE); //输出: empty array
print(frameF); //输出: 0x01 0x02 0x03 0x04
```

#### 7.[]获取数组某个索引上的引用
```c++
unsigned char ch = frameF[1]; //ch is oxo1
frameF[1] = 0x0A;
print(frameF); //输出: 0x01 0x0A 0x03 0x04
```

#### 8.char*、unsigned char*、void*的隐式转换支持
```c++
void func1(char* buf){}
void func2(unsigned char* buf){}
void func3(void* buf){}

func1(frameF);//正确运行
func2(frameF);//正确运行
func3(frameF);//正确运行
```
#### 9.unsigned char* data() 获取Frame数据成员指针

#### 10.unsigned long long size() 获取Frame数据长度(字节) 

#### 11.template<typename...T> static Frame combine(T&&...frames) 将若干个Frame按传入顺序拼接成完整的数据帧 
```c++
Frame frameA; //假设 frameA = 0x0A 0x0B 0x0C 0x0D
Frame frameB; //假设 frameB = 0x01 0x02 0x03 0x04
Frame frameC; //假设 frameC = 0x05 0x06 0x07 0x08

Frame frameABC = Frame::combine(frameA,frameB,frameC);
print(frameACB); //输出: 0x0A 0x0B 0x0C 0x0D 0x01 0x02 0x03 0x04 0x05 0x06 0x07 0x08
```
#### 注: Frame基本上可以作为unsigned char*的平替,需要使用char*、unsigned char*、void*等参数类型的地方基本上都可以直接传入Frame对象,无需再做额外的转换或者使用data()函数获取Frame对象内部的数组指针。 <br />

## 二：关于类模板 template<unsigned...BytePerArg>struct Trans 的成员函数和使用方法介绍
Trans模板用于将给定的数组、容器、函数参数按通信协议转换为对应的数据帧(Frame),这个模板中的所有函数返回值均为Frame。<br />
类模板参数unsigned...BytePerArg用于表明每一个数据在转换为数据帧(Frame)之后每一个数据所占的字节数,由于这是一个unsigned参数包,所以可以用BytePerArg...来表示通信协议。<br />
但是通过通信协议转换函数参数为数据帧和通过数组转换为数据帧存在一些区别,下面将介绍各个转换函数使用方法和注意事项。<br />

### Trans类的成员函数简介
#### 1.按通信协议转换函数:template<ByteMode Mode = Big, typename...Args> static Frame byProtocol(Args...args)
这个函数支持将函数参数列表中的数据按照类模板参数指定的通信协议转换为对应的二进制数据。<br />
例如存在以下通信协议A:<br />

帧头  | 指令号 | 数据  | 校验和 | 帧尾
----  | ----- | ----- | ----- | -----
1字节 | 2字节 |  4字节 | 1字节 | 1字节

那么我们可以通过以下的方式来创建一个对应的数据帧:
```c++
Frame frameG = Trans<1,2,4,1,1>::byProtocol(0x5A,0x02,0x11,0x13,0xA5);//创建一个大端保存的数据帧
```
此时便创建了一个以通信协议A为基准的二进制数据帧frameG。 <br />
frameG所代表的数据为<ins>0x5A</ins> <ins>0x00 0x02</ins> <ins>0x00 0x00 0x00 0x11</ins> <ins>0x13</ins> <ins>0xA5</ins> 。 <br />
是的,细心的朋友可能已经发现了上面的数据帧是以***大端***保存的数据帧,那如果需要数据按照小端保存又该怎么做呢? 很简单,只需要在创建帧的时候在函数模板参数中指定帧的大小端类型即可:
```c++
Frame frameH = Trans<1,2,4,1,1>::byProtocol<Little>(0x5A,0x02,0x11,0x13,0xA5);//创建一个小端保存的数据
```
frameH即为按照通信协议A和给定数据组成的以***小端***保存的数据帧。 <br />
frameH所代表的数据为<ins>0x5A</ins> <ins>0x02 0x00</ins> <ins>0x11 0x00 0x00 0x00</ins> <ins>0x13</ins> <ins>0xA5</ins> 。 <br />

#### 注: 类模板Trans中的全部函数默认指定为大端保存,如果需要用小端保存数据请像frameH一样在调用函数模板时在函数模板参数中指定大小端类型
这两种创建方式总结为如下:
```c++
//创建一个大端保存的数据帧
Frame frameG = Trans<1,2,4,1,1>::byProtocol<Big>(0x5A,0x02,0x11,0x13,0xA5);
//上方调用等价于Frame frameG = Trans<1,2,4,1,1>::byProtocol(0x5A,0x02,0x11,0x13,0xA5)

//创建一个小端保存的数据
Frame frameH = Trans<1,2,4,1,1>::byProtocol<Little>(0x5A,0x02,0x11,0x13,0xA5);
```
上述方式就是根据通信协议生成数据帧的基本方法,但是当通信协议为一些特殊情况时,我们有更简单的方式生成数据帧。 <br />
假设存在以下通信协议B: <br />

帧头  | 指令号 | 数据  | 校验和 | 帧尾
----  | ----- | ----- | ----- | -----
1字节 | 1字节 |  1字节 | 1字节 | 1字节

***当通信协议需要的每一个数据都只需要占用一个字节的时候***,我们不需要像frameG或frameH那样在模板参数中表明每一个字节的长度,此时我们可以直接在通信协议参数中填入帧的总长度即可: <br />
```c++
//因为通信协议B的总长度为5,所以我们直接写入Trans<5>即可,而不需要写成frameJ那样
Frame frameI = Trans<5>::byProtocol(0x5A,0x02,0x11,0x13,0xA5);
Frame frameJ = Trans<1,1,1,1,1>::byProtocol(0x5A,0x02,0x11,0x13,0xA5);
print(frameI); //输出 0x5A 0x02 0x11 0x12 0xA5
print(frameJ); //输出 0x5A 0x02 0x11 0x12 0xA5
```
事实上frameI和frameJ的内容是完全一致的,但是依然推荐选择frameI的方式,这种方式更简洁、效率高效(因为不需要做frameJ那么多的模板展开)。<br />
此外,当通信协议规定每一个数据只占用一个字节时,指定大小端也是无效的,即以下三种调用方式其实也是等价的
```c++
Frame frameI = Trans<5>::byProtocol(0x5A,0x02,0x11,0x13,0xA5);
Frame frameIbig = Trans<5>::byProtocol<Big>(0x5A,0x02,0x11,0x13,0xA5);
Frame frameIlittlr = Trans<5>::byProtocol<Little>(0x5A,0x02,0x11,0x13,0xA5);
```
***注:为了避开模板编译时的二义性,出现类似于frameJ的调用时编译会报错,此时将代码更改为FrameI样式的调用即可***<br/>

***如果仅仅是根据通信协议将传入函数的参数转换为数据帧,基本上只用到上面这一个函数即可。<br />
但是在某些情况下我们需要通过数据帧来传输数据,而帧的数据段长度可能是变化的,此时无法再通过指定通信协议然后调用byProtocol生成数据帧,也是不现实的。<br />
 <ins>通信协议太长会导致模板在展开时代码膨胀,这会严重影响代码的编译效率,同时使编译后的文件体积急剧增加,所以在协议太长的情况下确定数据帧的时候请根据系统性能和需求确定是否需要通过byProtocol生成一个数据帧</ins>。 <br />
 为了应对这种情况,我们可以将这种长度不定的帧拆分为三个部分: <ins>帧头段、数据段、帧尾段</ins>。<br />
 <ins>帧头段、帧尾段</ins>一定是可以根据通信协议确定的固定长度帧,这两部分数据可以通过byProtocol生成。 <br />
 <ins>数据段</ins>则通过fromArray函数生成,由于数据段中每一个数据所占长度通常都是固定的,所以在调用fromArray时只需要在类模板的模板参数中传入一个unsigned参数即可，具体使用方法参见每一个fromArray函数说明*** <br />

#### 2.容器转换函数:static Frame fromArray(const Array<T,Args...>& array) 将给定的容器中的数据按顺序转换为<ins>固定字节长度</ins>的unsigned char数组。
这个函数支持将多种容器作为参数,传入的容器内部需要<ins>存在siez()函数</ins>用于获取容器数据长度,建议传入的容器为顺序容器。使用方法如下:
```c++
std::vector<int> vec = {10,11,12,13};
Frame frameJ = Trans<4>::fromArray(vec);//将vec中的每一个数据都转换为每个数据占用4字节长度unsigned char数组
Frame frameK = Trans<3>::fromArray(vec);//将vec中的每一个数据都转换为每个数据占用3字节长度unsigned char数组
```
frameJ中的内容为:<ins>0x00 0x00 0x00 0x0A</ins>  <ins>0x00 0x00 0x00 0x0B</ins>   <ins>0x00 0x00 0x00 0x0C</ins>   <ins>0x00 0x00 0x00 0x0D</ins> <br />
frameK中的内容为:<ins>0x00 0x00 0x0A</ins>  <ins>0x00 0x00 0x0B</ins>   <ins>0x00 0x00 0x0C</ins>   <ins>0x00 0x00 0x0D</ins> <br />
同样的,***fromArray生成的数据帧默认是大端保存的***,如果需要生成小端保存的帧,也只需要在调用fromArray的时候指定大小端类型即可: <br />
```c++
std::vector<int> vec = {10,11,12,13};
Frame frameK = Trans<3>::fromArray(vec);//将vec中的每一个数据都转换为每个数据占用3字节长度unsigned char数组,大端保存
Frame frameK_L = Trans<3>::fromArray<Little>(vec);//将vec中的每一个数据都转换为每个数据占用3字节长度unsigned char数组,小段保存
```
frameK_L中的内容为:<ins>0x0A 0x00 0x00</ins>  <ins>0x0B 0x00 0x00</ins>   <ins>0x0C 0x00 0x00</ins>   <ins>0x0D 0x00 0x00</ins> <br />

***需要注意的是,在指定模板Trans<N>字节长度时,最好保证字节长度大于等于容器中的元素类型的字节大小,否则会导致数据被截断,保存的值无法再转换为原来的值。<br />***
例如将std::vector<int>中的数据转换为2字节大小的unsigned char数组,会出现以下情况:<br />
```c++
std::vector<int> vec = {10,11,0x11223344,0x55667788};//容器中元素的类型为int,占用4个字节
Frame frameM = Trans<2>::fromArray<Big>(vec);//将容器中的数据转换为每个数据占2个字节宽的数据帧,且大端保存
Frame frameN = Trans<2>::fromArray<Little>(vec);//将容器中的数据转换为每个数据占2个字节宽的数据帧,且小端保存
```
frameM中的内容为:<ins>0x00 0x0A</ins>  <ins>0x00 0x0B</ins>  <ins>0x33 0x44</ins>  <ins>0x77 0x88</ins> <br />
frameN中的内容为:<ins>0x0A 0x00</ins>  <ins>0x0B 0x00</ins>  <ins>0x44 0x33</ins>  <ins>0x88 0x77</ins> <br />
显然,由于在转换的时候指定用2字节长度去保存转换后的数据,2字节足够表示10和11但是不足够保存0x11223344和0x55667788, <br />
所以对10和11两个数据没有影响,但是比较大的两个数的高位被截断了,***所以一定要注意转换后的字节长度能否容纳原数据*** <br />

#### 3.静态数组转换函数：static Frame fromArray(const T(&array)[N]) 将静态数组按顺序转换为<ins>固定字节长度</ins>的unsigned char数组。
注意事项同上,使用方法如下:
```c++
int arr[4] = {0xAA,0xBB,0xCC,0xDD};
Frame frameO = Trans<3>::fromArray<Big>(arr);//将静态数组中的数据转换为每个数据占3个字节宽的数据帧,且大端保存
Frame frameP = Trans<3>::fromArray<Little>(arr);//将静态数组中的数据转换为每个数据占3个字节宽的数据帧,且小端保存
```
frameO中的内容为:<ins>0x00 0x00 0xAA</ins>  <ins>0x00 0x00 0xBB</ins>  <ins>0x00 0x00 0xCC</ins>  <ins>0x00 0x00 0xDD</ins> <br />
frameP中的内容为:<ins>0xAA 0x00 0x00</ins>  <ins>0xBB 0x00 0x00</ins>  <ins>0xCC 0x00 0x00</ins>  <ins>0xDD 0x00 0x00</ins> <br />


#### 4.动态数组转换函数: static  Frame fromArray(const T* array,std::size_t length) 将静态数组按顺序转换为<ins>固定字节长度</ins>的unsigned char数组。
注意事项同上,由于这个版本的fromArray接受的参数为一个动态数组指针,这里无法直接获取数组长度,所以需要在调用时将数组长度一并传入,使用方法如下:
```c++
int* arr = new int[4];
//将arr赋值为:{0x11,0x22,0x33,0x44};
Frame frameQ = Trans<3>::fromArray<Big>(arr,4);//将静态数组中的数据转换为每个数据占3个字节宽的数据帧,且大端保存
Frame frameR = Trans<3>::fromArray<Little>(arr,4);//将静态数组中的数据转换为每个数据占3个字节宽的数据帧,且小端保存
```
frameQ中的内容为:<ins>0x00 0x00 0x11</ins>  <ins>0x00 0x00 0x22</ins>  <ins>0x00 0x00 0x33</ins>  <ins>0x00 0x00 0x44</ins> <br />
frameR中的内容为:<ins>0x11 0x00 0x00</ins>  <ins>0x22 0x00 0x00</ins>  <ins>0x33 0x00 0x00</ins>  <ins>0x44 0x00 0x00</ins> <br />

**注:Trans类存在对数据类型的限制,要求传入的数组元素类型、容器元素类型、参数类型需要能被隐式转换为整型数据,例如浮点数(可能会转换后的导致数据错误)、枚举值等,如果是自定义类型则需要在类的内部实现整形数据的隐式转换函数。*** <br />
***注:在2025.7.21的更新中新增了对浮点值类型的支持,现在通过byProtocol生成数据帧或者通过fromArray生成数据帧均可以传入浮点类型的值,但是依然需要注意窄化转换的问题,即:将变量转化char数组时指定的字节长度最好不要小于变量类型所占字节数,否则高位数据会丢失*** <br />

## 三：关于类 FrameCheck 的成员函数和使用方法介绍
这个类主要用于对数据帧的数据做校验,主要包含两种类型的校验和校验、crc校验,其中crc校验类型又包含了20多种,基本上可以覆盖数据传输中常用的校验方式,具体的函数使用方法不一一介绍,因为每一种校验的参数基本上都是一致的,这里只拿crc16_modbus举例说明使用方法。 <br />
校验函数的声明如下:
```c++
template <unsigned ResultByte = twoByte,ByteMode mode = Big>
static FunctionReturn<ResultByte,twoByte> crc16_modbus(unsigned char* data,unsigned start,unsigned end,int pos)
```
函数接受一个unsigned char指针(可以直接传入Frame对象),计算从start索引开始(包含start)到end索引处(包含end)的全部数据的校验值,pos表示将校验结果从pos处开始写入,一共写入ResultByte个字节长度,写入校验结果时默认按照mode类型(大端)写入。
当pos<0时不写入校验结果到data数组,仅仅返回校验结果,当pos>=0返回校验结果,同时写入校验结果到pos处，***从pos处写入ResultByte个字节的数据时不能超出数组的总长度,否则会因为数组越界导致未定义行为(针对这个问题的保护机制会在后续加上)***。

默认情况下不需要指定模板尖括号中的参数,和校验通常占用1字节,crc校验结果所占字节数与crc校验方式相关,例如crc8的校验结果占用1字节、crc16_modbus校验结果占用2字节、crc32校验结果占用4字节
用户可以根据实际情况指定校验结果长度,这个长度可以大于默认长度(在不超过8字节长度限制的情况下,现在暂时没有开发超过8字节校验结果的校验方法)但是不能小于默认长度,当指定的长度小于默认长度时,模板FunctionReturn<ResultByte>会导致编译器报错,编译无法通过。
代码示例如下:
```c++
Frame frameS = {0x11,0x22,0x33,ox44,ox55,ox66,ox77,ox88,ox99,0xAA,0xBB,x0CC};
Check::crc
```

其他的校验方法使用方法也是如此,不再一一赘述(根据函数名应该就能知道这个校验是什么类型的)。

## 四：一个完整的示例。
假设存在以下场景:有一个设备一直在采集数据,现在需要将这些数据按照通信协议C发送到客户端,通信协议C如下:

帧头          | 指令号         | 数据段长度    | 数据段         | crc校验和         | 帧尾
------------  | ------------- | ------------- |------------- | ------------- | -------------
1字节         | 1字节          | 4字节         |N字节(每个数据占用2字节长度) | 4字节 | 1字节
0xAB | 0x01 | ....| .... | .... | 0xBA

已知数据以数组的形式给出(或者容器),校验方式为crc32校验,计算从指令号开始到数据段结束这一部分数据,采用小端保存。<br />
```c++
//假设这一次采集的数据长度为40
const unsigned dataNum = 40;

//假设采集到的数据如下
std::vector<unsigned> data(dataNum);
for(unsigned i = 0 ; i < dataNum; i++){
  data.at(i) = i * 2;
}

//由于中间包含了数据段这一部分长度不确定的数据,所以我们可以将通信协议分为三部分:
//第一部分:帧头+指令号+数据段长度
//第二部分:数据段
//第三部分:crc校验和+帧尾
//这样我们就将原本的通信协议分成了两个固定长度的帧+一个长度变化的帧

//生成第一部分数据帧对应数据,因为数据段每一个数据占2字节,一共40个数据,所以第三个参数为dataNum*2
Frame frameHead = Trans<1,1,4>::byProtocol<Little>(0xAB,0x01,dataNum*2);

//生成第二部分数据帧,直接从std::vector<unsigned>转换
Frame frameData = Trans<2>::fromArray<Little>(data);

//生成第三部分数据帧,这里还做不了crc校验,所以函数第一个参数暂时写为0
Frame frameTail = Trans<4,1>::byProtocol<Little>(0,0xBA);

//在这里将三个部分组合成一个完整的数据帧
Frame frame = Frame::combine(frameHead,frameData,frameTail);

//计算crc校验起始和结束索引位置
unsigned start = 1;
unsigned end = 1 + 4 + dataNum*2;

//crc校验,并将校验结果从end+1处开始写入,以小端顺序写入
FrameCheck::crc32<Little>(frame,start,end,end+1);

//此时Frame的数据为:
//AB 01 50 00 00 00 00 00 02 00 04 00 06 00 08 00 0A 00 0C 00 0E 00 10 00
//12 00 14 00 16 00 18 00 1A 00 1C 00 1E 00 20 00 22 00 24 00 26 00 28 00 
//2A 00 2C 00 2E 00 30 00 32 00 34 00 36 00 38 00 3A 00 3C 00 3E 00 40 00
//42 00 44 00 46 00 48 00 4A 00 4C 00 4E 00 6F BF CC FD BA

//以及下面这一段代码
std::vector<int> vec = {111,222,333,444};
int arr[4] = {111,222,333,444};
int* arr_p = new int[4];
arr_p[0] = 111;
arr_p[1] = 222;
arr_p[2] = 333;
arr_p[3] = 444;

Frame f1 = Trans<4>::fromArray<Little>(vec);
Frame f2 = Trans<4>::fromArray<Little>(arr);
Frame f3 = Trans<4>::fromArray<Little>(arr_p,4);

qDebug()<<QByteArray(f1,f1.size()).toHex(' ').toUpper();
qDebug()<<QByteArray(f2,f2.size()).toHex(' ').toUpper();
qDebug()<<QByteArray(f3,f3.size()).toHex(' ').toUpper();

std::vector<float> vecf = {11.5,22.5,33.5,44.5};
Frame f4 = Trans<4>::fromArray(vecf);
qDebug()<<QByteArray(f4,f4.size()).toHex(' ').toUpper();

Frame f5 = Trans<4>::fromArray<Little>(vecf);
qDebug()<<QByteArray(f5,f5.size()).toHex(' ').toUpper();

const unsigned* d1 = reinterpret_cast<const unsigned*>(&vecf[0]);
const unsigned* d2 = reinterpret_cast<const unsigned*>(&vecf[1]);
const unsigned* d3 = reinterpret_cast<const unsigned*>(&vecf[2]);
const unsigned* d4 = reinterpret_cast<const unsigned*>(&vecf[3]);
Frame f6 = Trans<4,4,4,4>::byProtocol(*d1,*d2,*d3,*d4);
qDebug()<<QByteArray(f6,f6.size()).toHex(' ').toUpper();

Frame f7 = Trans<1,4,4,4,4,1>::byProtocol(0x5a,0x11,0x22,0x33,0x44,0xa5);
qDebug()<<QByteArray(f7,f7.size()).toHex(' ').toUpper();

Frame f8 = Trans<1,4,4,4,4,1>::byProtocol<Little>(0x5a,0x11,0x22,0x33,0x44,0xa5);
qDebug()<<QByteArray(f8,f8.size()).toHex(' ').toUpper();

Frame f9 = Trans<1,4,4,4,4,1>::byProtocol(0x5a,vecf[0],vecf[1],vecf[2],vecf[3],0xa5);
qDebug()<<QByteArray(f9,f9.size()).toHex(' ').toUpper();

Frame f10 = Trans<1,4,4,4,4,1>::byProtocol(0x5a,0x11223344,0x22334455,0x33445566,0x44556677,0xa5);
qDebug()<<QByteArray(f10,f10.size()).toHex(' ').toUpper();

Frame f11 = Trans<1,4,4,4,4,1>::byProtocol<Little>(0x5a,0x11223344,0x22334455,0x33445566,0x44556677,0xa5);
qDebug()<<QByteArray(f11,f11.size()).toHex(' ').toUpper();

//输出如下：
//"6F 00 00 00 DE 00 00 00 4D 01 00 00 BC 01 00 00"
//"6F 00 00 00 DE 00 00 00 4D 01 00 00 BC 01 00 00"
//"6F 00 00 00 DE 00 00 00 4D 01 00 00 BC 01 00 00"
//"41 38 00 00 41 B4 00 00 42 06 00 00 42 32 00 00"
//"00 00 38 41 00 00 B4 41 00 00 06 42 00 00 32 42"
//"41 38 00 00 41 B4 00 00 42 06 00 00 42 32 00 00"
//"5A 00 00 00 11 00 00 00 22 00 00 00 33 00 00 00 44 A5"
//"5A 11 00 00 00 22 00 00 00 33 00 00 00 44 00 00 00 A5"
//"5A 41 38 00 00 41 B4 00 00 42 06 00 00 42 32 00 00 A5"
//"5A 11 22 33 44 22 33 44 55 33 44 55 66 44 55 66 77 A5"
//"5A 44 33 22 11 55 44 33 22 66 55 44 33 77 66 55 44 A5"
```
