#include "History.hpp"

using namespace History;

void History::StateStack::undo()
{
    if(m_UndoStack.empty())
        return;

    m_StackMutex.lock();

    std::function<void()> func = m_UndoStack.front();
    m_UndoStack.pop_front();

    func();

    m_StackMutex.unlock();
}

void StateStack::redo()
{
    if(m_UndoStack.empty())
        return;
}

void StateStack::update(std::function<void()>&& undoFunc,std::function<void()>&& redoFunc)
{
    if(m_StackMutex.try_lock())
        return;

    if(m_UndoStack.size() >= m_MaxSize)
        m_UndoStack.pop_back();//当栈已经满了的情况下,删除最末端的元素

    m_UndoStack.push_front(std::move(undoFunc));
    m_RedoFunc = std::move(redoFunc);
    m_StackMutex.unlock();
}

        /** note 1
         *栈在压入和弹出的时候加锁,除此以外这个锁还有一个很重要的重用,就是防止撤销操作进入死循环,不是所有人都能在刚开始接触和使用History接口的时候就能立刻理解到在什么时候、什么地方注册撤销动作才是正确合理的。
         * 考虑以下场景,存在一个add和remove方法,它们互为对方的撤销动作,所以在注册撤销动作的时候可能出现以下情况:
         *
         * add方法的功能实现如下:
         * void add(Item *item)
         * {
         *      History::store(&remove,...);//在添加Item的同时注册一个remove动作,用于撤销
         *      ......;//执行添加操作
         * }
         *
         * remove方法的功能的实现如下:
         * void remove(Item* item)
         * {
         *      History::store(&add,...);//在删除Item的同时注册一个add动作,用于撤销
         *      ......;执行删除操作
         * }
         *
         * 当add函数被执行的时候,一个remove函数就被添加到栈中,随后用户执行了一次撤销动作,栈中的remove函数被执行,一个add函数又被添加到栈中
         * 就正常的使用场景来说,在初始情况下,执行一个函数,再执行撤销之后栈里面的元素数量应该是0,但是上述情况会导致撤销之后栈中的元素数量始终为1
         * 这个栈就会像被死锁了一样,尽管此时还可以往栈里面添加元素,但是一旦执行到add或者remove函数的时候,元素的弹出就卡在了这个位置
         * 栈后面的函数再也不会被弹出,因为栈此时陷入了add->remove->add->remove->add->remove的死循环中。
         *
         * 如果能在正确的地方调用History::store函数,这种情况是不会出现的,比较推荐的做法是程序响应用户操作的时候进行store操作,比如:用户点下一个按钮,这个按钮对应的动作就是添加
         * void onPushButtonClicked()
         * {
         *      History::store(&remove,...);
         *      add(Item* item);
         * }
         * 删除Item的时候同理,这样使用History::store函数就不会导致撤销动作陷入死循环
         * 但是正如前面所说,不是所有人都能在刚开始使用History::store的时候就理解到在哪里调用才是正确的(尽管大部分人应该都能正确使用,但是依然不能排除意外错误使用的可能性)
         * 所以对这种情况就行规避是必要的,很巧的是,锁刚好可以避免程序出现这种情况
         * 在添加撤销动作和执行撤销动作的时候都对栈进行加锁,一方面可以避免数据竞争(尽管栈的压入和弹出都应该是在主线程中执行,但是谁能保证History::store不会被使用者在子线程中调用呢)
         * 另一方面就是避免出现上述的死循环情况,其原理如下
         * 在StateStack::add和StateStack::undo中都加上锁,使代码结构如下
         * void StateStack::add(Func func)
         * {
         *      if(mutex.try_lock())
         *          return;
         *      ......;//加锁成功执行后续操作
         * }
         *
         * void StateStack::undo()
         * {
         *      mutex.lock();
         *      ......;//加锁成功之后从栈中取出撤销函数并执行
         * }
         *
         * 此时,在执行撤销的时候,函数调用展开如下(已remove的撤销动作为例):
         * void StateStack::undo()
         * {
         *      mutex.lock();
         *      ......;//从栈中取出add(Item*)函数并执行
         *
         *      add(Item *item) //#1
         *      {
         *          History::store(&remove,...)#2
         *          {
         *              if(mutex.try_lock())  //#3
         *                  return;
         *
         *              ......;//将remove函数添加到栈中
         *          }
         *          ......;// #4
         *      }
         *      mutex.unlock();
         * }
         * 当函数执行到#1的时候,此时撤销动作(add函数被调用),根据前面的函数定义,add(Item*)函数会首先注册一个撤销动作,所以程序会继续执行到#2处,在#3处,由于尝试加锁失败
         * 所以History::store会被return,程序跳转回add(Item*)函数#4处继续执行,即完成添加Item*的操作
         * 这样一来,就不会有remove(Item* item)在撤销流程中被添加到栈中,也完成的remove对应的add操作,避免了撤销死循环
        **/
