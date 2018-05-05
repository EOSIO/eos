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
    fifo();

    void push(const T&element);
    T pop(std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    void release_all();

private:
    std::mutex m_mux;
    std::condition_variable m_cond;
    std::atomic<bool> m_release_all;
    std::deque<T> m_deque;
};

template<typename T>
fifo<T>::fifo()
{
    m_release_all = false;
}

template<typename T>
void fifo<T>::push(const T& element)
{
    std::lock_guard<std::mutex> lock(m_mux);
    m_deque.push_back(element);
    m_cond.notify_one();
}

template<typename T>
T fifo<T>::pop(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(m_mux);
    m_cond.wait(lock, [&]{return m_release_all || !m_deque.empty();});
    if (m_release_all)
        throw std::domain_error("nothing to pop");

    auto element = std::move(m_deque.front());
    m_deque.pop_front();
    return element;
}

template<typename T>
void fifo<T>::release_all()
{
    m_release_all = true;
    m_cond.notify_all();
}

} // namespace

#endif // FIFO_H
