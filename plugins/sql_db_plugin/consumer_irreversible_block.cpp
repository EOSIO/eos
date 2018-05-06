#include "consumer_irreversible_block.h"

namespace eosio {

void consumer_irreversible_block::consume(const std::vector<chain::signed_block>& blocks)
{
    for (const auto& block : blocks)
        ilog(block.id().str());
}

} // namespace
