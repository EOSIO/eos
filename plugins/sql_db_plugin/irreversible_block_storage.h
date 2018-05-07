#ifndef IRREVERSIBLE_BLOCK_STORAGE_H
#define IRREVERSIBLE_BLOCK_STORAGE_H

#include "storage.h"

namespace eosio {

class irreversible_block_storage : public storage
{
public:
    void store(const std::vector<chain::signed_block>& blocks) override;
};

} // namespace

#endif // IRREVERSIBLE_BLOCK_STORAGE_H
