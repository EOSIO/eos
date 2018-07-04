/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <boost/noncopyable.hpp>
#include <fc/log/logger.hpp>

#include "consumer_core.h"
#include "fifo.h"

namespace eosio {

template<typename T>
class consumer final : public boost::noncopyable
{
public:
    consumer(std::unique_ptr<consumer_core<T>> core);
    ~consumer();

    void push(const T& element);

private:
    void run();

    fifo<T> m_fifo;
    std::unique_ptr<consumer_core<T>> m_core;
    std::atomic<bool> m_exit;
    std::unique_ptr<std::thread> m_thread;
};

template<typename T>
consumer<T>::consumer(std::unique_ptr<consumer_core<T> > core):
    m_fifo(fifo<T>::behavior::blocking),
    m_core(std::move(core)),
    m_exit(false),
    m_thread(std::make_unique<std::thread>([&]{this->run();}))
{

}

template<typename T>
consumer<T>::~consumer()
{
    m_fifo.set_behavior(fifo<T>::behavior::not_blocking);
    m_exit = true;
    m_thread->join();
}

template<typename T>
void consumer<T>::push(const T& element)
{
    m_fifo.push(element);
}

template<typename T>
void consumer<T>::run()
{
    dlog("Consumer thread Start");
    while (!m_exit)
    {
        auto elements = m_fifo.pop_all();
        m_core->consume(elements);
    }
    dlog("Consumer thread End");
}

} // namespace

