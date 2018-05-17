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
        m_db->add(block);
        for (const chain::transaction_metadata_ptr& transaction : block->trxs) {
            m_db->add(transaction);
        }
    }
}

} // namespace
