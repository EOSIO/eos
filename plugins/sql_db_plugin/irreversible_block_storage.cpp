#include "irreversible_block_storage.h"

namespace eosio {

void irreversible_block_storage::consume(const std::vector<chain::signed_block>& blocks)
{
    for (const auto& block : blocks)
        ilog(block.id().str());
}

} // namespace
