#ifndef CONSUMER_H
#define CONSUMER_H

#include <thread>
#include <future>

#include "fifo.h"
#include "database.h"

namespace eosio {

class consumer
{
public:
    consumer(std::shared_ptr<database> db);

    void push(const chain::block_trace& t);
    void push(const chain::signed_block& b);

    void start();
    void stop();

private:
    void run(std::future<void> future_obj);

    std::shared_ptr<database> m_db;
    fifo<chain::block_trace> m_block_trace_fifo;
    fifo<chain::signed_block> m_block_trace_process_fifo;
    std::shared_ptr<std::thread> m_thread;
    std::promise<void> m_exit_signal;
};

} // namespace

#endif // CONSUMER_H
