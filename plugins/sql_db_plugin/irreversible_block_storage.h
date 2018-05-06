#ifndef IRREVERSIBLE_BLOCK_STORAGE_H
#define IRREVERSIBLE_BLOCK_STORAGE_H

#include <vector>
#include <eosio/chain/block.hpp>

namespace eosio {

class irreversible_block_storage
{
public:
    irreversible_block_storage();

    void storage(const std::vector<chain::signed_block>& blocks);
};

} // namespace

#endif // IRREVERSIBLE_BLOCK_STORAGE_H
