/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/permission_link_object.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

typedef chain::account_object::id_type co_id_type;
typedef chain::permission_usage_object::id_type puo_id_type;
typedef chain::permission_object::id_type po_id_type;
typedef chain::permission_link_object::id_type plo_id_type;
typedef chain::key_value_object::id_type kvo_id_type;


enum op_type_enum { 
   TABLE_REMOVE = 0,
   ROW_CREATE = 1, 
   ROW_MODIFY = 2, 
   ROW_REMOVE = 3,
   REV_UNDO   = 4,
   REV_COMMIT = 5,
};

struct db_account {
    account_name                name;
    uint8_t                     vm_type      = 0;
    uint8_t                     vm_version   = 0;
    bool                        privileged   = false;

    chain::time_point           last_code_update;
    chain::digest_type          code_version;
    chain::block_timestamp_type creation_date;
};

struct db_table {
   op_type_enum        op_type;
   account_name        code;
   fc::string          scope;
   chain::table_name   table;
   account_name        payer;
};

struct db_op : db_table {
   uint64_t            id;
   fc::variant         value;
};
   
struct db_rev {
   op_type_enum        op_type;
   int64_t             revision;
};

class statetrack_plugin : public plugin<statetrack_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   statetrack_plugin();
   statetrack_plugin(const statetrack_plugin&) = delete;
   statetrack_plugin(statetrack_plugin&&) = delete;
   statetrack_plugin& operator=(const statetrack_plugin&) = delete;
   statetrack_plugin& operator=(statetrack_plugin&&) = delete;
   virtual ~statetrack_plugin() override = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class statetrack_plugin_impl> my;
};

}

FC_REFLECT_ENUM( eosio::op_type_enum, (TABLE_REMOVE)(ROW_CREATE)(ROW_MODIFY)(ROW_REMOVE)(REV_UNDO)(REV_COMMIT) )
FC_REFLECT( eosio::db_account, (name)(vm_type)(vm_version)(privileged)(last_code_update)(code_version)(creation_date) )                 
FC_REFLECT( eosio::db_table, (op_type)(code)(scope)(table)(payer) )
FC_REFLECT_DERIVED( eosio::db_op, (eosio::db_table), (id)(value) )
FC_REFLECT( eosio::db_rev, (op_type)(revision) )