#include "consumer.h"

#include <iostream>

namespace eosio {

consumer::consumer(std::shared_ptr<database> db):
    m_db(db)
{

}

void consumer::push(const chain::block_trace &bt)
{
    ilog("applied_block");
    m_block_trace_fifo.push(bt);
}

void consumer::push(const chain::signed_block &b)
{
    ilog("applied_irreversible_block");
    m_block_trace_process_fifo.push(b);
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
    std::cout << "Thread Start" << std::endl;
    while (future_obj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
    {
        std::cout << "Doing Some Work" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    }
    std::cout << "Thread End" << std::endl;
}

} // namespace
