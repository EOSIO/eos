#ifndef ACTIONS_TABLE_H
#define ACTIONS_TABLE_H

#include <memory>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <eosio/chain/block_state.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/abi_serializer.hpp>

#include "connection_pool.h"

namespace eosio {

using std::string;

class actions_table
{
public:
    actions_table(std::shared_ptr<connection_pool> pool);

    void drop();
    void create();
    void add(chain::action action, chain::transaction_id_type transaction_id, fc::time_point_sec transaction_time, int32_t seq);

private:
    std::shared_ptr<connection_pool> m_pool;

    void
    parse_actions(chain::action action, fc::variant variant);
};

} // namespace

#endif // ACTIONS_TABLE_H
