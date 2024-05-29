#include "History.hpp"

using namespace History;

void History::StateStack::undo()
{
    if(m_Stack.empty())
        return;

    m_StackMutex.lock();

    std::function<void()> func = m_Stack.front();
    m_Stack.pop_front();

    func();

    m_StackMutex.unlock();
}

void StateStack::redo()
{
    if(m_Stack.empty())
        return;
}
