#include "consumer.h"

namespace eosio {

consumer::consumer(std::shared_ptr<database> db):
    m_db(db)
{

}

void consumer::push(const chain::block_trace &t)
{
    m_block_trace_fifo.push(t);
}

void consumer::push(const chain::signed_block &b)
{
    m_block_trace_process_fifo.push(b);
}

void consumer::consume()
{
    dlog("consumer::consume");
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
    dlog("Thread Start");
    while (future_obj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
    {
        consume();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    }
    dlog("Thread End");
}

} // namespace
