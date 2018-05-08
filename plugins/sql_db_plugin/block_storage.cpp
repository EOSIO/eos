#include "block_storage.h"

namespace eosio {

void block_storage::store(const std::vector<chain::block_trace> &blocks)
{
    for (auto block : blocks)
        ilog(block.block.id().str());
}

}
