#ifndef BLOCKS_TABLE_H
#define BLOCKS_TABLE_H

#include <memory>
#include <soci/soci.h>
#include <eosio/chain/block_state.hpp>

namespace eosio {

class blocks_table
{
public:
    blocks_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();
    void add(eosio::chain::block_state_ptr block);

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // BLOCKS_TABLE_H
