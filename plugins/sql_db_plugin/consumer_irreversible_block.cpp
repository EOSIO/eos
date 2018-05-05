#include "consumer_irreversible_block.h"

namespace eosio {

consumer_irreversible_block::consumer_irreversible_block(std::shared_ptr<database> db):
    consumer(db)
{

}

void consumer_irreversible_block::push(const chain::signed_block &block)
{
    m_fifo.push(block);
}

void consumer_irreversible_block::consume()
{
    auto block = m_fifo.pop();
    ilog(block.id().str());
}

} // namespace
