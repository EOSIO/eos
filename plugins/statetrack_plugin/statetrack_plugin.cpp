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

class statetrack_plugin_impl {
   public:
     abi_def get_kvo_abi( const chainbase::database& db, const chain::key_value_object& tio );
     
};

abi_def statetrack_plugin_impl::get_kvo_abi( const chainbase::database& db, const chain::key_value_object& kvo ) {
   const auto &d = db.db();

   const chain::table_id_object *tio = d.find<chain::table_id_object>(kvo.t_id);
   EOS_ASSERT(tio != nullptr, chain::account_query_exception, "Fail to retrieve table_id_object for ${t_id}", ("t_id", kvo.t_id) );
   
   const account_object *co = d.find<chain::account_object, by_name>(tio->code);
   EOS_ASSERT(co != nullptr, chain::account_query_exception, "Fail to retrieve account for ${account}", ("account", account) );
   
   abi_def abi;
   abi_serializer::to_abi(co->abi, abi);
   return abi;
}
   
void statetrack_plugin::plugin_startup() {
    const chainbase::database& db = app().get_plugin<chain_plugin>().chain().db();
   
    auto& kv_index = db.get_index<kv_index_type>();
   
    kv_index.applied_emplace = [&](const chain::key_value_object& kvo) {
         elog("STATETRACK key_value_object emplace: ${kvo}", ("kvo", kvo.id));
       
         
    };
   
    kv_index.applied_modify = [&](const chain::key_value_object& kvo) {
         elog("STATETRACK key_value_object modify: ${kvo}", ("kvo", kvo.id));
    };
   
    kv_index.applied_remove = [&](const chain::key_value_object& kvo) {
         elog("STATETRACK key_value_object remove: ${kvo}", ("kvo", kvo.id));
    };
}

}
