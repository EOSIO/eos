#ifndef ACTIONS_TABLE_H
#define ACTIONS_TABLE_H

#include <memory>
#include <soci/soci.h>
#include <eosio/chain/block_state.hpp>

namespace eosio {

class actions_table
{
public:
    actions_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();
    void add(eosio::chain::action action);

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // ACTIONS_TABLE_H
