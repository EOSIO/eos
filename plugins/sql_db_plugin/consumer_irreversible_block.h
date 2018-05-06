#ifndef CONSUMER_IRREVERSIBLE_BLOCK_H
#define CONSUMER_IRREVERSIBLE_BLOCK_H

#include "consumer.h"

#include <eosio/chain/block.hpp>
#include <eosio/chain/block_trace.hpp>

namespace eosio {

class consumer_irreversible_block : public consumer<chain::signed_block>
{
private:
    void consume(const std::vector<chain::signed_block> &blocks) override;
};

} // namespace

#endif // CONSUMER_IRREVERSIBLE_BLOCK_H
