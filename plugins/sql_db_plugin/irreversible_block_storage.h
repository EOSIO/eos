#ifndef IRREVERSIBLE_BLOCK_STORAGE_H
#define IRREVERSIBLE_BLOCK_STORAGE_H

#include "consumer_core.h"

#include <eosio/chain/block.hpp>

namespace eosio {

class irreversible_block_storage : public consumer_core<chain::signed_block>
{
public:
    void consume(const std::vector<chain::signed_block>& blocks) override;
};

} // namespace

#endif // IRREVERSIBLE_BLOCK_STORAGE_H
