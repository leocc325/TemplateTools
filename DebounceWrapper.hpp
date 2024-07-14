#ifndef DEBOUNCEWRAPPER_HPP
#define DEBOUNCEWRAPPER_HPP

#include <type_traits>
#include <functional>

class DebounceWrapper
{
public:
    ///普通函数
    template<typename Func,typename...Args>
    void call(Func&& func,Args&&...args);

    ///成员函数
    template<typename Func,typename Obj,typename...Args>
    void call(Func func,Obj* obj,Args&&...args);

    ///可调用对象


    ///std::function
    template<typename Return,typename...Args>
    void call(const std::function<Return(Args&&...args)>& callable);

private:

};

#endif
