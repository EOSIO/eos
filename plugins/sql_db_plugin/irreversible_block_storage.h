#ifndef IRREVERSIBLE_BLOCK_STORAGE_H
#define IRREVERSIBLE_BLOCK_STORAGE_H

#include "consumer.h"

namespace eosio {

class irreversible_block_storage : public consumer<chain::signed_block>
{
public:
    void consume(const std::vector<chain::signed_block>& blocks) override;
};

} // namespace

#endif // IRREVERSIBLE_BLOCK_STORAGE_H
