#ifndef FIFO_H
#define FIFO_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <deque>

namespace eosio {

template<typename T>
class fifo
{
public:
    void push(const T&element);
    T pop();

private:
    std::mutex m_mux;
    std::condition_variable m_cond;
    std::deque<T> m_deque;
};

template<typename T>
void fifo<T>::push(const T& element)
{
    std::lock_guard<std::mutex> lock(m_mux);
    m_deque.push_back(element);
    m_cond.notify_one();
}

template<typename T>
T fifo<T>::pop()
{
    std::unique_lock<std::mutex> lock(m_mux);
    while(m_deque.empty())
        m_cond.wait(lock);
    auto element = std::move(m_deque.front());
    m_deque.pop_front();
    return element;
}

} // namespace

#endif // FIFO_H
