#include "consumer.h"

#include <fc/log/logger.hpp>

namespace eosio {

consumer::~consumer()
{

}

void consumer::start()
{
    m_exit = false;
    m_thread = std::make_shared<std::thread>([&]{this->run();});
}

void consumer::stop()
{
    m_exit = true;
    m_thread->join();
}

void consumer::run()
{
    dlog("Consumer thread Start");
    while (!m_exit)
        consume();
    dlog("Consumer thread End");
}

} // namespace
