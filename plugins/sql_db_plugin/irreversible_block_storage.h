#ifndef IRREVERSIBLE_BLOCK_STORAGE_H
#define IRREVERSIBLE_BLOCK_STORAGE_H

#include "consumer_core.h"

#include <eosio/chain/block_state.hpp>

namespace eosio {

class irreversible_block_storage : public consumer_core<chain::block_state_ptr>
{
public:
    void consume(const std::vector<chain::block_state_ptr>& blocks) override;
};

} // namespace

#endif // IRREVERSIBLE_BLOCK_STORAGE_H
