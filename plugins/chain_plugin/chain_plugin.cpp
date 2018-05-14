/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/resource_limits.hpp>

#include <eosio/chain/eosio_contract.hpp>

#include <eosio/utilities/key_conversion.hpp>
#include <eosio/utilities/common.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <eosio/chain/plugin_interface.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

namespace eosio {

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::config;
using namespace eosio::chain::plugin_interface;
using vm_type = wasm_interface::vm_type;
using fc::flat_map;

//using txn_msg_rate_limits = controller::txn_msg_rate_limits;


class chain_plugin_impl {
public:
   chain_plugin_impl()
   :accepted_block_header_channel(app().get_channel<channels::accepted_block_header>())
   ,accepted_block_channel(app().get_channel<channels::accepted_block>())
   ,irreversible_block_channel(app().get_channel<channels::irreversible_block>())
   ,accepted_transaction_channel(app().get_channel<channels::accepted_transaction>())
   ,applied_transaction_channel(app().get_channel<channels::applied_transaction>())
   ,accepted_confirmation_channel(app().get_channel<channels::accepted_confirmation>())
   ,incoming_block_channel(app().get_channel<incoming::channels::block>())
   ,incoming_block_sync_method(app().get_method<incoming::methods::block_sync>())
   ,incoming_transaction_sync_method(app().get_method<incoming::methods::transaction_sync>())
   {}
   
   bfs::path                        block_log_dir;
   bfs::path                        genesis_file;
   time_point                       genesis_timestamp;
   bool                             readonly = false;
   uint64_t                         shared_memory_size;
   flat_map<uint32_t,block_id_type> loaded_checkpoints;

   fc::optional<fork_database>      fork_db;
   fc::optional<block_log>          block_logger;
   fc::optional<controller::config> chain_config = controller::config();
   fc::optional<controller>         chain;
   chain_id_type                    chain_id;
   //txn_msg_rate_limits              rate_limits;
   fc::optional<vm_type>            wasm_runtime;

   // retained references to channels for easy publication
   channels::accepted_block_header::channel_type&  accepted_block_header_channel;
   channels::accepted_block::channel_type&         accepted_block_channel;
   channels::irreversible_block::channel_type&     irreversible_block_channel;
   channels::accepted_transaction::channel_type&   accepted_transaction_channel;
   channels::applied_transaction::channel_type&    applied_transaction_channel;
   channels::accepted_confirmation::channel_type&  accepted_confirmation_channel;
   incoming::channels::block::channel_type&         incoming_block_channel;

   // retained references to methods for easy calling
   incoming::methods::block_sync::method_type&       incoming_block_sync_method;
   incoming::methods::transaction_sync::method_type& incoming_transaction_sync_method;

   // method provider handles
   methods::get_block_by_number::method_type::handle                 get_block_by_number_provider;
   methods::get_block_by_id::method_type::handle                     get_block_by_id_provider;
   methods::get_head_block_id::method_type::handle                   get_head_block_id_provider;
   methods::get_last_irreversible_block_number::method_type::handle  get_last_irreversible_block_number_provider;

};

chain_plugin::chain_plugin()
:my(new chain_plugin_impl()) {
}

chain_plugin::~chain_plugin(){}

void chain_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("genesis-json", bpo::value<bfs::path>()->default_value("genesis.json"), "File to read Genesis State from")
         ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
         ("block-log-dir", bpo::value<bfs::path>()->default_value("blocks"),
          "the location of the block log (absolute path or relative to application data dir)")
         ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("wasm-runtime", bpo::value<eosio::chain::wasm_interface::vm_type>()->value_name("wavm/binaryen"), "Override default WASM runtime")
         ("shared-memory-size-mb", bpo::value<uint64_t>()->default_value(config::default_shared_memory_size / (1024  * 1024)), "Maximum size MB of database shared memory file")

#warning TODO: rate limiting
         /*("per-authorized-account-transaction-msg-rate-limit-time-frame-sec", bpo::value<uint32_t>()->default_value(default_per_auth_account_time_frame_seconds),
          "The time frame, in seconds, that the per-authorized-account-transaction-msg-rate-limit is imposed over.")
         ("per-authorized-account-transaction-msg-rate-limit", bpo::value<uint32_t>()->default_value(default_per_auth_account),
          "Limits the maximum rate of transaction messages that an account is allowed each per-authorized-account-transaction-msg-rate-limit-time-frame-sec.")
          ("per-code-account-transaction-msg-rate-limit-time-frame-sec", bpo::value<uint32_t>()->default_value(default_per_code_account_time_frame_seconds),
           "The time frame, in seconds, that the per-code-account-transaction-msg-rate-limit is imposed over.")
          ("per-code-account-transaction-msg-rate-limit", bpo::value<uint32_t>()->default_value(default_per_code_account),
           "Limits the maximum rate of transaction messages that an account's code is allowed each per-code-account-transaction-msg-rate-limit-time-frame-sec.")*/
         ;
   cli.add_options()
         ("replay-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain database and replay all blocks")
         ("resync-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain database and block log")
         ;
}

void chain_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing chain plugin");

   if(options.count("genesis-json")) {
      auto genesis = options.at("genesis-json").as<bfs::path>();
      if(genesis.is_relative())
         my->genesis_file = app().config_dir() / genesis;
      else
         my->genesis_file = genesis;
   }
   if(options.count("genesis-timestamp")) {
     string tstr = options.at("genesis-timestamp").as<string>();
     if (strcasecmp (tstr.c_str(), "now") == 0) {
       my->genesis_timestamp = fc::time_point::now();
       auto epoch_ms = my->genesis_timestamp.time_since_epoch().count() / 1000;
       auto diff_ms = epoch_ms % block_interval_ms;
       if (diff_ms > 0) {
         auto delay_ms =  (block_interval_ms - diff_ms);
         my->genesis_timestamp += fc::microseconds(delay_ms * 10000);
         dlog ("pausing ${ms} milliseconds to the next interval",("ms",delay_ms));
       }
     }
     else {
       my->genesis_timestamp = time_point::from_iso_string (tstr);
     }
   }
   if (options.count("block-log-dir")) {
      auto bld = options.at("block-log-dir").as<bfs::path>();
      if(bld.is_relative())
         my->block_log_dir = app().data_dir() / bld;
      else
         my->block_log_dir = bld;
   }
   if (options.count("shared-memory-size-mb")) {
      my->shared_memory_size = options.at("shared-memory-size-mb").as<uint64_t>() * 1024 * 1024;
   }

   if (options.at("replay-blockchain").as<bool>()) {
      ilog("Replay requested: wiping database");
      fc::remove_all(app().data_dir() / default_shared_memory_dir);
   }
   if (options.at("resync-blockchain").as<bool>()) {
      ilog("Resync requested: wiping database and blocks");
      fc::remove_all(app().data_dir() / default_shared_memory_dir);
      fc::remove_all(my->block_log_dir);
   }

   if(options.count("checkpoint"))
   {
      auto cps = options.at("checkpoint").as<vector<string>>();
      my->loaded_checkpoints.reserve(cps.size());
      for(auto cp : cps)
      {
         auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type>>();
         my->loaded_checkpoints[item.first] = item.second;
      }
   }

   if(options.count("wasm-runtime"))
      my->wasm_runtime = options.at("wasm-runtime").as<vm_type>();

   if( !fc::exists( my->genesis_file ) ) {
      wlog( "\n generating default genesis file ${f}", ("f", my->genesis_file.generic_string() ) );
      genesis_state default_genesis;
      fc::json::save_to_file( default_genesis, my->genesis_file, true );
   }
   my->chain_config->block_log_dir = my->block_log_dir;
   my->chain_config->shared_memory_dir = app().data_dir() / default_shared_memory_dir;
   my->chain_config->read_only = my->readonly;
   my->chain_config->shared_memory_size = my->shared_memory_size;
   my->chain_config->genesis = fc::json::from_file(my->genesis_file).as<genesis_state>();
   if (my->genesis_timestamp.sec_since_epoch() > 0) {
      my->chain_config->genesis.initial_timestamp = my->genesis_timestamp;
   }

   if(my->wasm_runtime)
      my->chain_config->wasm_runtime = *my->wasm_runtime;

   my->chain.emplace(*my->chain_config);

   // set up method providers
   my->get_block_by_number_provider = app().get_method<methods::get_block_by_number>().register_provider([this](uint32_t block_num) -> signed_block_ptr {
      return my->chain->fetch_block_by_number(block_num);
   });

   my->get_block_by_id_provider = app().get_method<methods::get_block_by_id>().register_provider([this](block_id_type id) -> signed_block_ptr {
      return my->chain->fetch_block_by_id(id);
   });

   my->get_head_block_id_provider = app().get_method<methods::get_head_block_id>().register_provider([this](){
      return my->chain->head_block_id();
   });

   my->get_last_irreversible_block_number_provider = app().get_method<methods::get_last_irreversible_block_number>().register_provider([this](){
      return my->chain->last_irreversible_block_num();
   });

   // relay signals to channels
   my->chain->accepted_block_header.connect([this](const block_state_ptr& blk) {
      my->accepted_block_header_channel.publish(blk);
   });

   my->chain->accepted_block.connect([this](const block_state_ptr& blk) {
      my->accepted_block_channel.publish(blk);
   });

   my->chain->irreversible_block.connect([this](const block_state_ptr& blk) {
      my->irreversible_block_channel.publish(blk);
   });

   my->chain->accepted_transaction.connect([this](const transaction_metadata_ptr& meta){
      my->accepted_transaction_channel.publish(meta);
   });

   my->chain->applied_transaction.connect([this](const transaction_trace_ptr& trace){
      my->applied_transaction_channel.publish(trace);
   });

   my->chain->accepted_confirmation.connect([this](const header_confirmation& conf){
      my->accepted_confirmation_channel.publish(conf);
   });

}

void chain_plugin::plugin_startup()
{ try {
   my->chain->startup();

   if(!my->readonly) {
      ilog("starting chain in read/write mode");
      /// TODO: my->chain->add_checkpoints(my->loaded_checkpoints);
   }

   ilog("Blockchain started; head block is #${num}, genesis timestamp is ${ts}",
        ("num", my->chain->head_block_num())("ts", (std::string)my->chain_config->genesis.initial_timestamp));

   my->chain_config.reset();

} FC_CAPTURE_LOG_AND_RETHROW( (my->genesis_file.generic_string()) ) }

void chain_plugin::plugin_shutdown() {
   my->chain.reset();
}

chain_apis::read_write chain_plugin::get_read_write_api() {
   return chain_apis::read_write(chain());
}

void chain_plugin::accept_block(const signed_block_ptr& block ) {
   my->incoming_block_sync_method(block);
}

void chain_plugin::accept_transaction(const packed_transaction& trx) {
   my->incoming_transaction_sync_method(std::make_shared<packed_transaction>(trx));
}

bool chain_plugin::block_is_on_preferred_chain(const block_id_type& block_id) {
   auto b = chain().fetch_block_by_number( block_header::num_from_id(block_id) );
   return b && b->id() == block_id;
}

controller::config& chain_plugin::chain_config() {
   // will trigger optional assert if called before/after plugin_initialize()
   return *my->chain_config;
}

controller& chain_plugin::chain() { return *my->chain; }
const controller& chain_plugin::chain() const { return *my->chain; }

void chain_plugin::get_chain_id(chain_id_type &cid)const {
   memcpy(cid.data(), my->chain_id.data(), cid.data_size());
}

namespace chain_apis {

const string read_only::KEYi64 = "i64";

read_only::get_info_results read_only::get_info(const read_only::get_info_params&) const {
   const auto& rm = db.get_resource_limits_manager();
   return {
      eosio::utilities::common::itoh(static_cast<uint32_t>(app().version())),
      db.head_block_num(),
      db.last_irreversible_block_num(),
      db.last_irreversible_block_id(),
      db.head_block_id(),
      db.head_block_time(),
      db.head_block_producer(),
      rm.get_virtual_block_cpu_limit(),
      rm.get_virtual_block_net_limit(),
      rm.get_block_cpu_limit(),
      rm.get_block_net_limit()
      //std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string(),
      //__builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0
   };
}

abi_def get_abi( const controller& db, const name& account ) {
   const auto &d = db.db();
   const account_object *code_accnt = d.find<account_object, by_name>(account);
   EOS_ASSERT(code_accnt != nullptr, chain::account_query_exception, "Fail to retrieve account for ${account}", ("account", account) );
   abi_def abi;
   abi_serializer::to_abi(code_accnt->abi, abi);
   return abi;
}

string get_table_type( const abi_def& abi, const name& table_name ) {
   for( const auto& t : abi.tables ) {
      if( t.name == table_name ){
         return t.index_type;
      }
   }
   EOS_ASSERT( false, chain::contract_table_query_exception, "Table ${table} is not specified in the ABI", ("table",table_name) );
}

read_only::get_table_rows_result read_only::get_table_rows( const read_only::get_table_rows_params& p )const {
   const abi_def abi = get_abi( db, p.code );
   auto table_type = get_table_type( abi, p.table );

   if( table_type == KEYi64 ) {
      return get_table_rows_ex<key_value_index, by_scope_primary>(p,abi);
   }

   EOS_ASSERT( false, chain::contract_table_query_exception,  "Invalid table type ${type}", ("type",table_type)("abi",abi));
}

vector<asset> read_only::get_currency_balance( const read_only::get_currency_balance_params& p )const {

   const abi_def abi = get_abi( db, p.code );
   auto table_type = get_table_type( abi, "accounts" );

   vector<asset> results;
   walk_table<key_value_index, by_scope_primary>(p.code, p.account, N(accounts), [&](const key_value_object& obj){
      EOS_ASSERT( obj.value.size() >= sizeof(asset), chain::asset_type_exception, "Invalid data on table");

      asset cursor;
      fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
      fc::raw::unpack(ds, cursor);

      EOS_ASSERT( cursor.get_symbol().valid(), chain::asset_type_exception, "Invalid asset");

      if( !p.symbol || boost::iequals(cursor.symbol_name(), *p.symbol) ) {
        results.emplace_back(cursor);
      }

      // return false if we are looking for one and found it, true otherwise
      return !(p.symbol && boost::iequals(cursor.symbol_name(), *p.symbol));
   });

   return results;
}

fc::variant read_only::get_currency_stats( const read_only::get_currency_stats_params& p )const {
   fc::mutable_variant_object results;

   const abi_def abi = get_abi( db, p.code );
   auto table_type = get_table_type( abi, "stat" );

   uint64_t scope = ( eosio::chain::string_to_symbol( 0, boost::algorithm::to_upper_copy(p.symbol).c_str() ) >> 8 );

   walk_table<key_value_index, by_scope_primary>(p.code, scope, N(stat), [&](const key_value_object& obj){
      EOS_ASSERT( obj.value.size() >= sizeof(read_only::get_currency_stats_result), chain::asset_type_exception, "Invalid data on table");

      fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
      read_only::get_currency_stats_result result;

      fc::raw::unpack(ds, result.supply);
      fc::raw::unpack(ds, result.max_supply);
      fc::raw::unpack(ds, result.issuer);

      results[result.supply.symbol_name()] = result;
      return true;
   });

   return results;
}

template<typename Api>
struct resolver_factory {
   static auto make(const Api *api) {
      return [api](const account_name &name) -> optional<abi_serializer> {
         const auto *accnt = api->db.db().template find<account_object, by_name>(name);
         if (accnt != nullptr) {
            abi_def abi;
            if (abi_serializer::to_abi(accnt->abi, abi)) {
               return abi_serializer(abi);
            }
         }

         return optional<abi_serializer>();
      };
   }
};

template<typename Api>
auto make_resolver(const Api *api) {
   return resolver_factory<Api>::make(api);
}

fc::variant read_only::get_block(const read_only::get_block_params& params) const {
   signed_block_ptr block;
   try {
      block = db.fetch_block_by_id(fc::json::from_string(params.block_num_or_id).as<block_id_type>());
      if (!block) {
         block = db.fetch_block_by_number(fc::to_uint64(params.block_num_or_id));
      }

   } EOS_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))

   EOS_ASSERT( block, unknown_block_exception, "Could not find block: ${block}", ("block", params.block_num_or_id));

   fc::variant pretty_output;
   abi_serializer::to_variant(*block, pretty_output, make_resolver(this));

   uint32_t ref_block_prefix = block->id()._hash[1];

   return fc::mutable_variant_object(pretty_output.get_object())
           ("id", block->id())
           ("block_num",block->block_num())
           ("ref_block_prefix", ref_block_prefix);
}

read_write::push_block_results read_write::push_block(const read_write::push_block_params& params) {
   db.push_block( std::make_shared<signed_block>(params) );
   return read_write::push_block_results();
}

read_write::push_transaction_results read_write::push_transaction(const read_write::push_transaction_params& params) {
   auto pretty_input = std::make_shared<packed_transaction>();
   auto resolver = make_resolver(this);
   try {
      abi_serializer::from_variant(params, *pretty_input, resolver);
   } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

   auto trx_trace_ptr = app().get_method<incoming::methods::transaction_sync>()(pretty_input);

   fc::variant pretty_output = db.to_variant_with_abi( *trx_trace_ptr );;
   //abi_serializer::to_variant(*trx_trace_ptr, pretty_output, resolver);
   return read_write::push_transaction_results{ trx_trace_ptr->id, pretty_output };
}

read_write::push_transactions_results read_write::push_transactions(const read_write::push_transactions_params& params) {
   FC_ASSERT( params.size() <= 1000, "Attempt to push too many transactions at once" );

   push_transactions_results result;
   result.reserve(params.size());
   for( const auto& item : params ) {
      try {
        result.emplace_back( push_transaction( item ) );
      } catch ( const fc::exception& e ) {
        result.emplace_back( read_write::push_transaction_results{ transaction_id_type(),
                          fc::mutable_variant_object( "error", e.to_detail_string() ) } );
      }
   }
   return result;
}

read_only::get_code_results read_only::get_code( const get_code_params& params )const {
   get_code_results result;
   result.account_name = params.account_name;
   const auto& d = db.db();
   const auto& accnt  = d.get<account_object,by_name>( params.account_name );

   if( accnt.code.size() ) {
      result.wast = wasm_to_wast( (const uint8_t*)accnt.code.data(), accnt.code.size() );
      result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   }

   abi_def abi;
   if( abi_serializer::to_abi(accnt.abi, abi) ) {

      result.abi = std::move(abi);
   }

   return result;
}

read_only::get_account_results read_only::get_account( const get_account_params& params )const {
   get_account_results result;
   result.account_name = params.account_name;

   const auto& d = db.db();
   const auto& rm = db.get_resource_limits_manager();

   rm.get_account_limits( result.account_name, result.ram_quota, result.net_weight, result.cpu_weight );

   const auto& a = db.get_account(result.account_name);

   result.privileged       = a.privileged;
   result.last_code_update = a.last_code_update;
   result.created          = a.creation_date;

   result.net_limit = rm.get_account_net_limit_ex( result.account_name );
   result.cpu_limit = rm.get_account_cpu_limit_ex( result.account_name );
   result.ram_usage = rm.get_account_ram_usage( result.account_name );

   const auto& permissions = d.get_index<permission_index,by_owner>();
   auto perm = permissions.lower_bound( boost::make_tuple( params.account_name ) );
   while( perm != permissions.end() && perm->owner == params.account_name ) {
      /// TODO: lookup perm->parent name
      name parent;

      // Don't lookup parent if null
      if( perm->parent._id ) {
         const auto* p = d.find<permission_object,by_id>( perm->parent );
         if( p ) {
            FC_ASSERT(perm->owner == p->owner, "Invalid parent");
            parent = p->name;
         }
      }

      result.permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
      ++perm;
   }

   const auto& code_account = db.db().get<account_object,by_name>( N(eosio) );
   //const abi_def abi = get_abi( db, N(eosio) );
   abi_def abi;
   if( abi_serializer::to_abi(code_account.abi, abi) ) {
      abi_serializer abis( abi );
      //get_table_rows_ex<key_value_index, by_scope_primary>(p,abi);
      const auto* t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, params.account_name, N(userres) ));
      if (t_id != nullptr) {
         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
         if ( it != idx.end() ) {
            vector<char> data;
            copy_inline_row(*it, data);
            result.total_resources = abis.binary_to_variant( "user_resources", data );
         }
      }

      t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, params.account_name, N(delband) ));
      if (t_id != nullptr) {
         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
         if ( it != idx.end() ) {
            vector<char> data;
            copy_inline_row(*it, data);
            result.delegated_bandwidth = abis.binary_to_variant( "delegated_bandwidth", data );
         }
      }

      t_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple( config::system_account_name, config::system_account_name, N(voters) ));
      if (t_id != nullptr) {
         const auto &idx = d.get_index<key_value_index, by_scope_primary>();
         auto it = idx.find(boost::make_tuple( t_id->id, params.account_name ));
         if ( it != idx.end() ) {
            vector<char> data;
            copy_inline_row(*it, data);
            result.voter_info = abis.binary_to_variant( "voter_info", data );
         }
      }
   }
   return result;
}

read_only::abi_json_to_bin_result read_only::abi_json_to_bin( const read_only::abi_json_to_bin_params& params )const try {
   abi_json_to_bin_result result;
   const auto code_account = db.db().find<account_object,by_name>( params.code );
   EOS_ASSERT(code_account != nullptr, contract_query_exception, "Contract can't be found ${contract}", ("contract", params.code));

   abi_def abi;
   if( abi_serializer::to_abi(code_account->abi, abi) ) {
      abi_serializer abis( abi );
      try {
         result.binargs = abis.variant_to_binary(abis.get_action_type(params.action), params.args);
      } EOS_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
                                "'${args}' is invalid args for action '${action}' code '${code}'",
                                ("args", params.args)("action", params.action)("code", params.code))
   }
   return result;
} FC_CAPTURE_AND_RETHROW( (params.code)(params.action)(params.args) )

read_only::abi_bin_to_json_result read_only::abi_bin_to_json( const read_only::abi_bin_to_json_params& params )const {
   abi_bin_to_json_result result;
   const auto& code_account = db.db().get<account_object,by_name>( params.code );
   abi_def abi;
   if( abi_serializer::to_abi(code_account.abi, abi) ) {
      abi_serializer abis( abi );
      result.args = abis.binary_to_variant( abis.get_action_type( params.action ), params.binargs );
   }
   return result;
}

read_only::get_required_keys_result read_only::get_required_keys( const get_required_keys_params& params )const {
   transaction pretty_input;
   from_variant(params.transaction, pretty_input);
   auto required_keys_set = db.get_authorization_manager().get_required_keys(pretty_input, params.available_keys);
   get_required_keys_result result;
   result.required_keys = required_keys_set;
   return result;
}


} // namespace chain_apis
} // namespace eosio
