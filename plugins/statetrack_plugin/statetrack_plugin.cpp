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

   ti_index.connect_emplace( [&](const auto& tio ) {
        elog("STATETRACK table_id_object emplace: ${tio}", ("tio", tio.id));
   });

   ti_index.connect_modify( [&](const auto& tio ) {
        elog("STATETRACK table_id_object modify: ${tio}", ("tio", tio.id));
   });

   ti_index.connect_remove( [&](const auto& tio ) {
        elog("STATETRACK table_id_object remove: ${tio}", ("tio", tio.id));
   });

   auto& kv_index = db.get_index<kv_index_type>();
   
   kv_index.connect_emplace( [&](const auto& kvo ) {
        elog("STATETRACK kv_index_type emplace: ${kvo}", ("kvo", kvo.id));
   });

   kv_index.connect_modify( [&](const auto& kvo ) {
        elog("STATETRACK kv_index_type modify: ${kvo}", ("kvo", kvo.id));
   });

   kv_index.connect_remove( [&](const auto& kvo ) {
        elog("STATETRACK kv_index_type remove: ${kvo}", ("kvo", kvo.id));
   });
}

}
