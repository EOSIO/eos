#ifndef CONSUMER_H
#define CONSUMER_H

#include <thread>
#include <atomic>
#include <vector>
#include <fc/log/logger.hpp>

#include "fifo.h"

namespace eosio {

template<typename T>
class consumer
{
public:
    consumer(std::function<const std::vector<T>&> f);
    virtual ~consumer(){}

    void push(const T& b);

    void start();
    void stop();

private:
    void run();

    fifo<T> m_fifo;
    std::shared_ptr<std::thread> m_thread;
    std::atomic<bool> m_exit;
    std::function<const std::vector<T>&> consume;
};

template<typename T>
consumer<T>::consumer(std::function<const std::vector<T>&> f):
    m_fifo(fifo<T>::behavior::blocking),
    consume(f)
{

}

template<typename T>
void consumer<T>::push(const T& block)
{
    m_fifo.push(block);
}

template<typename T>
void consumer<T>::start()
{
    m_exit = false;
    m_thread = std::make_shared<std::thread>([&]{this->run();});
}

template<typename T>
void consumer<T>::stop()
{
    m_fifo.set_behavior(fifo<T>::behavior::not_blocking);
    m_exit = true;
    m_thread->join();
}

template<typename T>
void consumer<T>::run()
{
    dlog("Consumer thread Start");
    while (!m_exit)
    {
        auto elements = m_fifo.pop_all();
        consume(elements);
    }
    dlog("Consumer thread End");
}

} // namespace

#endif // CONSUMER_H
