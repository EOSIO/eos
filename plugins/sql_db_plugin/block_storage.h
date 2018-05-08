#ifndef BLOCK_STORAGE_H
#define BLOCK_STORAGE_H

#include "storage.h"

#include <eosio/chain/block_trace.hpp>

namespace eosio {

class block_storage : public storage<chain::block_trace>
{
public:
    void store(const std::vector<chain::block_trace>& blocks) override;
};

}

#endif // BLOCK_STORAGE_H
