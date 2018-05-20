#ifndef ACTIONS_TABLE_H
#define ACTIONS_TABLE_H

#include <memory>

#include <soci/soci.h>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <eosio/chain/block_state.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/abi_serializer.hpp>

namespace eosio {

using std::string;

class actions_table
{
public:
    actions_table(std::shared_ptr<soci::session> session);

    void drop();
    void create();
    void add(chain::action action);

private:
    std::shared_ptr<soci::session> m_session;
};

} // namespace

#endif // ACTIONS_TABLE_H
