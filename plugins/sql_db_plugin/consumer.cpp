#include "consumer.h"

namespace eosio {

consumer::consumer(std::shared_ptr<database> db):
    m_db(db)
{

}

void consumer::applied_block(const chain::block_trace &bt)
{
    ilog("applied_block");
    m_block_trace_fifo.push(bt);
}

void consumer::applied_irreversible_block(const chain::signed_block &b)
{
    ilog("applied_irreversible_block");
    m_block_trace_process_fifo.push(b);
}

} // namespace
