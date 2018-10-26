/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/statetrack_plugin/statetrack_plugin.hpp>

namespace eosio {

static appbase::abstract_plugin& _statetrack_plugin = app().register_plugin<statetrack_plugin>();

using namespace eosio;


typedef typename chainbase::get_index_type<chain::key_value_object>::type kv_index_type;
typedef typename chainbase::get_index_type<chain::table_id_object>::type ti_index_type;

void statetrack_plugin::plugin_startup() {
   const chainbase::database& db = app().get_plugin<chain_plugin>().chain().db();

   auto& ti_index = db.get_index<ti_index_type>();

   ti_index.connect_emplace( [&](decltype(v) v ) {
        elog("STATETRACK table_id_object emplace: ${v}", ("v", v.id));
   });

   ti_index.connect_modify( [&](decltype(v) v ) {
        elog("STATETRACK table_id_object modify: ${v}", ("v", tio.id));
   });

   ti_index.connect_remove( [&](decltype(v) v ) {
        elog("STATETRACK table_id_object remove: ${v}", ("v", tio.id));
   });

   auto& kv_index = db.get_index<kv_index_type>();
   
   kv_index.connect_emplace( [&](decltype(v) v ) {
        elog("STATETRACK kv_index_type emplace: ${v}", ("v", kvo.id));
   });

   kv_index.connect_modify( [&](decltype(v) v ) {
        elog("STATETRACK kv_index_type modify: ${v}", ("v", kvo.id));
   });

   kv_index.connect_remove( [&](decltype(v) v ) {
        elog("STATETRACK kv_index_type remove: ${v}", ("v", kvo.id));
   });
}

}
