/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/statetrack_plugin/statetrack_plugin.hpp>

namespace eosio {

static appbase::abstract_plugin& _statetrack_plugin = app().register_plugin<statetrack_plugin>();

using namespace eosio;


typedef typename chainbase::get_index_type<chain::key_value_object>::type kv_index_type;

class statetrack_plugin_impl {
   public:
     const chain::table_id_object& get_kvo_tio(const chainbase::database& db, const chain::key_value_object& kvo);
     const chain::account_object& get_tio_co(const chainbase::database& db, const chain::table_id_object& tio);
     const abi_def& get_co_abi(const chain::account_object& co );
     fc::variant get_kvo_row(const chainbase::database& db, const chain::table_id_object& tio, const chain::key_value_object& kvo, bool json = true);
     db_op_row get_db_op_row(const chainbase::database& db, const chain::key_value_object& kvo, op_type_enum op_type, bool json = true);

     static void copy_inline_row(const chain::key_value_object& obj, vector<char>& data) {
        data.resize( obj.value.size() );
        memcpy( data.data(), obj.value.data(), obj.value.size() );
    }
    private:
        fc::microseconds abi_serializer_max_time = fc::microseconds(1000 * 1000);
        bool shorten_abi_errors = true;
};

// statetrack plugin implementation

const chain::table_id_object& statetrack_plugin_impl::get_kvo_tio(const chainbase::database& db, const chain::key_value_object& kvo) {
    const chain::table_id_object *tio = db.find<chain::table_id_object>(kvo.t_id);
    EOS_ASSERT(tio != nullptr, chain::contract_table_query_exception, "Fail to retrieve table for ${t_id}", ("t_id", kvo.t_id) );
    return *tio;
}

const chain::account_object& statetrack_plugin_impl::get_tio_co(const chainbase::database& db, const chain::table_id_object& tio) {
    const chain::account_object *co = db.find<chain::account_object, chain::by_name>(tio.code);
    EOS_ASSERT(co != nullptr, chain::account_query_exception, "Fail to retrieve account for ${code}", ("code", tio.code) );
    return *co;
}

const abi_def& statetrack_plugin_impl::get_co_abi( const chain::account_object& co ) {   
   abi_def* abi = new abi_def();
   abi_serializer::to_abi(co.abi, *abi);
   return *abi;
}

fc::variant statetrack_plugin_impl::get_kvo_row(const chainbase::database& db, const chain::table_id_object& tio, const chain::key_value_object& kvo, bool json) {
    vector<char> data;
    copy_inline_row(kvo, data);

    if (json) {
        const chain::account_object& co = get_tio_co(db, tio);
        const abi_def& abi = get_co_abi(co);

        abi_serializer abis;
        abis.set_abi(abi, abi_serializer_max_time);

        return abis.binary_to_variant( abis.get_table_type(tio.table), data, abi_serializer_max_time, shorten_abi_errors );
    } else {
        return fc::variant(data);
    }
}

db_op_row statetrack_plugin_impl::get_db_op_row(const chainbase::database& db, const chain::key_value_object& kvo, op_type_enum op_type, bool json) {
    db_op_row row;

    const chain::table_id_object& tio = get_kvo_tio(db, kvo);

    row.id = kvo.primary_key;
    row.op_type = op_type;
    row.code = tio.code;
    row.scope = tio.scope;
    row.table = tio.table;

    if(op_type == op_type_enum::CREATE || 
       op_type == op_type_enum::MODIFY) {
        row.value = get_kvo_row(db, tio, kvo, json);
    }

    return row;
}

// Plugin implementation

statetrack_plugin::statetrack_plugin():my(new statetrack_plugin_impl()){}

void statetrack_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("option-name", bpo::value<string>()->default_value("default value"),
          "Option Description")
         ;
}

void statetrack_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing statetrack plugin");
   try {
      if( options.count( "option-name" )) {
         // Handle the option
      }
   }
   FC_LOG_AND_RETHROW()
}

void statetrack_plugin::plugin_startup() {
    try {
      const chainbase::database& db = app().get_plugin<chain_plugin>().chain().db();
   
        auto& kv_index = db.get_index<kv_index_type>();
    
        kv_index.applied_emplace = [&](const chain::key_value_object& kvo) {
            fc::string data = fc::json::to_string(my->get_db_op_row(db, kvo, op_type_enum::CREATE));
            ilog("STATETRACK key_value_object emplace: ${data}", ("data", data));
        };
    
        kv_index.applied_modify = [&](const chain::key_value_object& kvo) {
            fc::string data = fc::json::to_string(my->get_db_op_row(db, kvo, op_type_enum::MODIFY));
            ilog("STATETRACK key_value_object modify: ${data}", ("data", data));
        };
    
        kv_index.applied_remove = [&](const chain::key_value_object& kvo) {
            fc::string data = fc::json::to_string(my->get_db_op_row(db, kvo, op_type_enum::REMOVE));
            ilog("STATETRACK key_value_object remove: ${data}", ("data", data));
        };
   }
   FC_LOG_AND_RETHROW()
}

void statetrack_plugin::plugin_shutdown() {
   ilog("statetrack plugin shutdown");
}

}
