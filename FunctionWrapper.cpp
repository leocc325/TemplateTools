#include "FunctionWrapper.hpp"

FunctionWrapper::FunctionPrivate::FunctionPrivate(std::size_t msg)
{
    using Ret = void;
    using ArgsTuple = std::tuple<>;

    //默认初始化参数类型和返回值类型为空的tuple和void
    msgId = msg;
    argTupleInfo = &typeid(ArgsTuple);
    resultInfo = &typeid(Ret);
}

FunctionWrapper::FunctionPrivate::~FunctionPrivate()
{
    if(deleteHelper)
        deleteHelper(argsTuple,result);
}

FunctionWrapper::FunctionPrivate::FunctionPrivate(const FunctionWrapper::FunctionPrivate &other)
{
    this->msgId = other.msgId;
    this->functor = other.functor;
    this->msgArgsHelper = other.msgArgsHelper;
    this->scpiArgsHelper = other.scpiArgsHelper;
    this->deleteHelper = other.deleteHelper;
    this->resultHelper = other.resultHelper;
    this->resultString = other.resultString;

    //指针拷贝完成之后对参数指针初始化
    this->resultInfo = other.resultInfo;
    this->argTupleInfo = other.argTupleInfo;
    this->msgArgsHelper(other.argsTuple,this->argsTuple);
    this->resultHelper(other.result,this->result);
}

FunctionWrapper::FunctionPrivate::FunctionPrivate(FunctionWrapper::FunctionPrivate &&other)
{
    this->msgId = other.msgId;
    //这里需要将other.msgId重置为默认值吗？

    this->functor = std::move(other.functor);

    this->msgArgsHelper = other.msgArgsHelper;
    other.msgArgsHelper = nullptr;

    this->scpiArgsHelper = other.scpiArgsHelper;
    other.scpiArgsHelper = nullptr;

    this->deleteHelper = other.deleteHelper;
    other.deleteHelper = nullptr;

    this->resultHelper = other.resultHelper;
    other.resultHelper = nullptr;

    this->argsTuple = other.argsTuple;
    other.argsTuple = nullptr;

    this->result = other.result;
    other.result = nullptr;

    this->argTupleInfo = other.argTupleInfo;
    other.argTupleInfo = nullptr;

    this->resultInfo = other.resultInfo;
    other.resultInfo = nullptr;

    this->resultString = std::move(other.resultString);
}

FunctionWrapper::FunctionWrapper(std::size_t msg)
{
    d = new FunctionPrivate(msg);
}

FunctionWrapper::FunctionWrapper(const FunctionWrapper &other)
{
    d = new FunctionPrivate(*other.d);
}

FunctionWrapper::FunctionWrapper(FunctionWrapper &&other) noexcept
{
    d = other.d;
    other.d = nullptr;
}
