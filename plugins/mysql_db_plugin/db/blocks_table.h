#ifndef BLOCKS_TABLE_H
#define BLOCKS_TABLE_H

#include <memory>
#include <chrono>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <eosio/chain/block_state.hpp>

#include "connection_pool.h"

namespace eosio {

class blocks_table
{
public:
    blocks_table(std::shared_ptr<connection_pool> pool);

    void drop();
    void create();
    void add(chain::signed_block_ptr block);
    void process_irreversible(chain::signed_block_ptr block);
private:
    std::shared_ptr<connection_pool> m_pool;
};

} // namespace

#endif // BLOCKS_TABLE_H
