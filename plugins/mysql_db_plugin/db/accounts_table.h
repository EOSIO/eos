#ifndef ACCOUNTS_TABLE_H
#define ACCOUNTS_TABLE_H

#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>
#include <memory>

#include "connection_pool.h"

namespace eosio {

using std::string;
using chain::account_name;
using chain::action_name;
using chain::block_id_type;
using chain::permission_name;
using chain::transaction;
using chain::signed_transaction;
using chain::signed_block;
using chain::transaction_id_type;
using chain::packed_transaction;

class accounts_table
{
public:
    accounts_table(std::shared_ptr<connection_pool> pool);

    void drop();
    void create();
    void update_account(chain::action action);
    void add(string name);
    bool exist(string name);

    void add_account_control(const chain::vector<chain::permission_level_weight>& controlling_accounts,
                            const account_name& name, const permission_name& permission,
                            const std::chrono::milliseconds& now);
    void remove_account_control( const account_name& name, const permission_name& permission );
    void add_pub_keys(const vector<chain::key_weight>& keys, const account_name& name, const permission_name& permission);
    void remove_pub_keys(const account_name& name, const permission_name& permission);

private:
    std::shared_ptr<connection_pool> m_pool;
};

} // namespace

#endif // ACCOUNTS_TABLE_H
