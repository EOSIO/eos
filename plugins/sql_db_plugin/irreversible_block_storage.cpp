#include "irreversible_block_storage.h"

namespace eosio {

irreversible_block_storage::irreversible_block_storage(std::shared_ptr<database> db):
    m_db(db)
{

}

void irreversible_block_storage::consume(const std::vector<chain::block_state_ptr>& blocks)
{
    for (const chain::block_state_ptr& block : blocks)
    {
        ilog(block->id.str());
        m_db->add(block);
    }
}

} // namespace
