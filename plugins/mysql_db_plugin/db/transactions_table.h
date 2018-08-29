#ifndef TRANSACTIONS_TABLE_H
#define TRANSACTIONS_TABLE_H

#include <memory>
#include <eosio/chain/transaction_metadata.hpp>
#include "connection_pool.h"

namespace eosio {

class transactions_table
{
public:
    transactions_table(std::shared_ptr<connection_pool> pool);

    void drop();
    void create();
    void add(uint32_t block_id, chain::transaction transaction);

private:
    std::shared_ptr<connection_pool> m_pool;
};

} // namespace

#endif // TRANSACTIONS_TABLE_H
