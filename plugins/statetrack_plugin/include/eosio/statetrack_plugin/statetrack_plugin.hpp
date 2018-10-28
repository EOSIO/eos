/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

enum op_type_enum { 
   CREATE, 
   MODIFY, 
   REMOVE 
};

struct db_op_row {
   uint64_t            id;
   op_type_enum        op_type;
   account_name        code;
   chain::scope_name   scope;
   chain::table_name   table;
   account_name        payer;
   fc::variant         value;
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

FC_REFLECT_ENUM( eosio::op_type_enum, (CREATE)(MODIFY)(REMOVE) )
FC_REFLECT( eosio::db_op_row, (id)(op_type)(code)(scope)(table)(payer)(value) )