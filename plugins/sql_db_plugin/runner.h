#ifndef CONSUMER_H
#define CONSUMER_H

#include <thread>
#include <atomic>
#include <vector>
#include <boost/noncopyable.hpp>
#include <fc/log/logger.hpp>

#include "consumer.h"
#include "fifo.h"

namespace eosio {

template<typename T>
class runner : public boost::noncopyable
{
public:
    using vector = std::vector<T>;

    runner(std::unique_ptr<consumer<T>> c);
    ~runner();

    void push(const T& element);

private:
    void run();

    fifo<T> m_fifo;
    std::unique_ptr<std::thread> m_thread;
    std::atomic<bool> m_exit;
    std::unique_ptr<consumer<T>> m_consumer;
};

template<typename T>
runner<T>::runner(std::unique_ptr<consumer<T> > c):
    m_fifo(fifo<T>::behavior::blocking),
    m_consumer(std::move(c))
{
    m_exit = false;
    m_thread = std::make_unique<std::thread>([&]{this->run();});
}

template<typename T>
runner<T>::~runner()
{
    m_fifo.set_behavior(fifo<T>::behavior::not_blocking);
    m_exit = true;
    m_thread->join();
}

template<typename T>
void runner<T>::push(const T& element)
{
    m_fifo.push(element);
}

template<typename T>
void runner<T>::run()
{
    dlog("Consumer thread Start");
    while (!m_exit)
    {
        auto elements = m_fifo.pop_all();
        m_consumer->consume(elements);
    }
    dlog("Consumer thread End");
}

} // namespace

#endif // CONSUMER_H
