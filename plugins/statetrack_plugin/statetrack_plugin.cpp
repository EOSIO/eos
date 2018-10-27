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

typedef boost::signals2::signal<void (const chain::table_id_object&)> st1;
typedef st1::slot_type sst1;


class statetrack_plugin_impl {
   public:
    void ti_callback(const chain::table_id_object& tio);

};

void statetrack_plugin_impl::ti_callback(const chain::table_id_object& tio) {
    elog("STATETRACK table_id_object emplace: ${tio}", ("tio", tio.id));
}

void statetrack_plugin::plugin_startup() {
    const chainbase::database& db = app().get_plugin<chain_plugin>().chain().db();

    const auto& ti_index = db.get_index<ti_index_type>();

    ti_index.applied_emplace = [&](const chain::table_id_object& tio) {
         elog("STATETRACK table_id_object modify: ${tio}", ("tio", tio.id));
    };

//    ti_index.connect_modify( [&](decltype(tio) tio ) {
//         elog("STATETRACK table_id_object modify: ${tio}", ("tio", tio.id));
//    });

//    ti_index.connect_remove( [&](decltype(tio) tio ) {
//         elog("STATETRACK table_id_object remove: ${tio}", ("tio", tio.id));
//    });

//    auto& kv_index = db.get_index<kv_index_type>();
   
//    kv_index.connect_emplace( [&](decltype(kvo) kvo ) {
//         elog("STATETRACK kv_index_type emplace: ${kvo}", ("kvo", kvo.id));
//    });

//    kv_index.connect_modify( [&](decltype(kvo) kvo ) {
//         elog("STATETRACK kv_index_type modify: ${kvo}", ("kvo", kvo.id));
//    });

//    kv_index.connect_remove( [&](decltype(kvo) kvo ) {
//         elog("STATETRACK kv_index_type remove: ${kvo}", ("kvo", kvo.id));
//    });
}

}
