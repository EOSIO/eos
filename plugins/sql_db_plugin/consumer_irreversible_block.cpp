#include "consumer_irreversible_block.h"

namespace eosio {

consumer_irreversible_block::consumer_irreversible_block(std::shared_ptr<database> db):
    consumer(db),
    m_fifo(signed_block_fifo::behavior::blocking)
{

}

void consumer_irreversible_block::push(const chain::signed_block &block)
{
    m_fifo.push(block);
}

void consumer_irreversible_block::consume()
{
    auto blocks = m_fifo.pop_all();
    for (const auto& block : blocks)
        ilog(block.id().str());
}

void consumer_irreversible_block::stop()
{
    m_fifo.set_behavior(signed_block_fifo::behavior::not_blocking);
    consumer::stop();
}

} // namespace
