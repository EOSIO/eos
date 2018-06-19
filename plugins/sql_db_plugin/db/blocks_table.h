#ifndef BLOCKS_TABLE_H
#define BLOCKS_TABLE_H

#include <memory>
#include <chrono>

#include <soci/soci.h>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <eosio/chain/block_state.hpp>

namespace eosio {

class blocks_table
{
public:
    blocks_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();
    void add(chain::signed_block_ptr block);

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // BLOCKS_TABLE_H
