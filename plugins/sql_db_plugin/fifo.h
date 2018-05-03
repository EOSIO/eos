#ifndef FIFO_H
#define FIFO_H

#include <mutex>
#include <deque>

namespace eosio {

template<typename T>
class fifo
{
public:
    void push(const T&);
    T pop();

private:
    std::mutex m_mux;
    std::deque<T> m_deque;
};

template<typename T>
void fifo<T>::push(const T& d)
{
    std::lock_guard<std::mutex> lock(m_mux);
    m_deque.push_back(d);
}

template<typename T>
T fifo<T>::pop()
{
    std::lock_guard<std::mutex> lock(m_mux);
    auto e = m_deque.front();
    m_deque.pop_front();
    return e;
}

} // namespace

#endif // FIFO_H
