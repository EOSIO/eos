#ifndef BLOCK_STORAGE_H
#define BLOCK_STORAGE_H

#include "consumer_core.h"

#include <eosio/chain/block_trace.hpp>

namespace eosio {

class block_storage : public consumer_core<chain::block_trace>
{
public:
    void consume(const std::vector<chain::block_trace>& blocks) override;
};

}

#endif // BLOCK_STORAGE_H
