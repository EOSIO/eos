/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain_to_mongo_plugin/chain_to_mongo_plugin.hpp>

namespace eosio {

static appbase::abstract_plugin& _chain_to_mongo_plugin = app().register_plugin<chain_to_mongo_plugin>();

using namespace eosio;

typedef typename get_index_type<key_value_object>::type index_type;

void chain_to_mongo_plugin::plugin_startup() {
   const chainbase::database& db = app().get_plugin<chain_plugin>().chain().db();
   
   db.get_mutable_index<index_type>().applied_emplace.connect( [&]( const value_type& t ) {
        elog("CHAIN_TO_MONGO emplace: ${t}", ("t", t.to_string()));
   });

   db.get_mutable_index<index_type>().applied_modify.connect( [&]( const value_type& t ) {
        elog("CHAIN_TO_MONGO modify: ${t}", ("t", t.to_string()));
   });

   db.get_mutable_index<index_type>().applied_remove.connect( [&]( const value_type& t ) {
        elog("CHAIN_TO_MONGO remove: ${t}", ("t", t.to_string()));
   });
}

}
