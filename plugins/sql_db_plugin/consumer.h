#ifndef CONSUMER_H
#define CONSUMER_H

#include <thread>
#include <atomic>
#include <vector>
#include <boost/noncopyable.hpp>
#include <fc/log/logger.hpp>

#include "fifo.h"

namespace eosio {

template<typename T>
class consumer : public boost::noncopyable
{
public:
    using consume_function = std::function<void(const std::vector<T>&)>;

    consumer(consume_function consume);
    ~consumer();

    void push(const T& b);

private:
    void run();

    fifo<T> m_fifo;
    std::shared_ptr<std::thread> m_thread;
    std::atomic<bool> m_exit;
    consume_function m_consume_function;
};

template<typename T>
consumer<T>::consumer(consume_function consume):
    m_fifo(fifo<T>::behavior::blocking),
    m_consume_function(consume)
{
    m_exit = false;
    m_thread = std::make_shared<std::thread>([&]{this->run();});
}

template<typename T>
consumer<T>::~consumer()
{
    m_fifo.set_behavior(fifo<T>::behavior::not_blocking);
    m_exit = true;
    m_thread->join();
}

template<typename T>
void consumer<T>::push(const T& block)
{
    m_fifo.push(block);
}

template<typename T>
void consumer<T>::run()
{
    dlog("Consumer thread Start");
    while (!m_exit)
    {
        auto elements = m_fifo.pop_all();
        m_consume_function(elements);
    }
    dlog("Consumer thread End");
}

} // namespace

#endif // CONSUMER_H
