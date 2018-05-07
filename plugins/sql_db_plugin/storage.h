#ifndef STORAGE_H
#define STORAGE_H

#include <vector>
#include <eosio/chain/block.hpp>

namespace eosio {

class storage {
    virtual void store(const std::vector<chain::signed_block>& blocks) = 0;
};

} // namespace

#endif // STORAGE_H

