#include "consumer.h"

#include <fc/log/logger.hpp>

namespace eosio {

consumer::consumer(std::shared_ptr<database> db):
    m_db(db)
{

}

consumer::~consumer()
{

}

void consumer::start()
{
    m_thread = std::make_shared<std::thread>(
                [&]{this->run(m_exit_signal.get_future());}
    );
}

void consumer::stop()
{
    m_exit_signal.set_value();
    m_thread->join();
}

void consumer::run(std::future<void> future_obj)
{
    dlog("Consumer thread Start");
    while (future_obj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
        consume();
    dlog("Consumer thread End");
}

} // namespace
