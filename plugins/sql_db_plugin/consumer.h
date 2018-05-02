#ifndef CONSUMER_H
#define CONSUMER_H

#include "fifo.h"
#include "database.h"

namespace eosio {

class consumer
{
public:
    consumer(std::shared_ptr<database> db);

    void applied_block(const chain::block_trace& bt);
    void applied_irreversible_block(const chain::signed_block& b);

private:
    std::shared_ptr<database> m_db;
    fifo<chain::block_trace> m_block_trace_fifo;
    fifo<chain::signed_block> m_block_trace_process_fifo;
};

} // namespace

#endif // CONSUMER_H
