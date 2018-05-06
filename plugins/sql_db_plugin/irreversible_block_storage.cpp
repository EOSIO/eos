#include "irreversible_block_storage.h"

namespace eosio {

irreversible_block_storage::irreversible_block_storage()
{

}

void irreversible_block_storage::storage(const std::vector<chain::signed_block>& blocks)
{
    for (const auto& block : blocks)
        ilog(block.id().str());
}

} // namespace
