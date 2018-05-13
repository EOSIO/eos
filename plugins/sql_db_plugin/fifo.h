/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <vector>
#include <boost/noncopyable.hpp>

namespace eosio {

template<typename T>
class fifo : public boost::noncopyable
{
public:
    enum class behavior {blocking, not_blocking};

    fifo(behavior value);

    void push(const T& element);
    std::vector<T> pop_all();
    void set_behavior(behavior value);

private:
    std::mutex m_mux;
    std::condition_variable m_cond;
    std::atomic<behavior> m_behavior;
    std::deque<T> m_deque;
};

template<typename T>
fifo<T>::fifo(behavior value)
{
    m_behavior = value;
}

template<typename T>
void fifo<T>::push(const T& element)
{
    std::lock_guard<std::mutex> lock(m_mux);
    m_deque.push_back(element);
    m_cond.notify_one();
}

template<typename T>
std::vector<T> fifo<T>::pop_all()
{
    std::unique_lock<std::mutex> lock(m_mux);
    m_cond.wait(lock, [&]{return m_behavior == behavior::not_blocking || !m_deque.empty();});

    std::vector<T> result;
    while(!m_deque.empty())
    {
        result.push_back(std::move(m_deque.front()));
        m_deque.pop_front();
    }
    return result;
}

template<typename T>
void fifo<T>::set_behavior(behavior value)
{
    m_behavior = value;
    m_cond.notify_all();
}

} // namespace


