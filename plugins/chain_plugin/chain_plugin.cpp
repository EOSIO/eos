/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/producer_object.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>


#include <eosio/chain/contracts/chain_initializer.hpp>
#include <eosio/chain/contracts/genesis_state.hpp>
#include <eosio/chain/contracts/eos_contract.hpp>

#include <eosio/utilities/key_conversion.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

namespace eosio {

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::config;
using fc::flat_map;

//using txn_msg_rate_limits = chain_controller::txn_msg_rate_limits;


class chain_plugin_impl {
public:
   bfs::path                        block_log_dir;
   bfs::path                        genesis_file;
   time_point                       genesis_timestamp;
   uint32_t                         skip_flags = skip_nothing;
   bool                             readonly = false;
   flat_map<uint32_t,block_id_type> loaded_checkpoints;

   fc::optional<fork_database>      fork_db;
   fc::optional<block_log>          block_logger;
   fc::optional<chain_controller::controller_config> chain_config = chain_controller::controller_config();
   fc::optional<chain_controller>   chain;
   chain_id_type                    chain_id;
   uint32_t                         max_reversible_block_time_ms;
   uint32_t                         max_pending_transaction_time_ms;
   //txn_msg_rate_limits              rate_limits;
};

chain_plugin::chain_plugin()
:my(new chain_plugin_impl()) {
}

chain_plugin::~chain_plugin(){}

void chain_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("genesis-json", bpo::value<boost::filesystem::path>(), "File to read Genesis State from")
         ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
         ("block-log-dir", bpo::value<bfs::path>()->default_value("blocks"),
          "the location of the block log (absolute path or relative to application data dir)")
         ("checkpoint,c", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("max-reversible-block-time", bpo::value<int32_t>()->default_value(-1),
          "Limits the maximum time (in milliseconds) that a reversible block is allowed to run before being considered invalid")
         ("max-pending-transaction-time", bpo::value<int32_t>()->default_value(-1),
          "Limits the maximum time (in milliseconds) that is allowed a pushed transaction's code to execute before being considered invalid")
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
         ("skip-transaction-signatures", bpo::bool_switch()->default_value(false),
          "Disable transaction signature verification. ONLY for TESTING.")
         ;
}

void chain_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing chain plugin");

   if(options.count("genesis-json")) {
      my->genesis_file = options.at("genesis-json").as<bfs::path>();
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

   if (options.at("replay-blockchain").as<bool>()) {
      ilog("Replay requested: wiping database");
      fc::remove_all(app().data_dir() / default_shared_memory_dir);
   }
   if (options.at("resync-blockchain").as<bool>()) {
      ilog("Resync requested: wiping database and blocks");
      fc::remove_all(app().data_dir() / default_shared_memory_dir);
      fc::remove_all(my->block_log_dir);
   }
   if (options.at("skip-transaction-signatures").as<bool>()) {
      ilog("Setting skip_transaction_signatures");
      elog("Setting skip_transaction_signatures\n"
           "\n"
           "**************************************\n"
           "*                                    *\n"
           "*   -- EOSD IGNORING SIGNATURES --   *\n"
           "*   -         TEST MODE          -   *\n"
           "*   ------------------------------   *\n"
           "*                                    *\n"
           "**************************************\n");

      my->skip_flags |= skip_transaction_signatures;
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

   my->max_reversible_block_time_ms = options.at("max-reversible-block-time").as<int32_t>();
   my->max_pending_transaction_time_ms = options.at("max-pending-transaction-time").as<int32_t>();

#warning TODO: Rate Limits
   /*my->rate_limits.per_auth_account_time_frame_sec = fc::time_point_sec(options.at("per-authorized-account-transaction-msg-rate-limit-time-frame-sec").as<uint32_t>());
   my->rate_limits.per_auth_account = options.at("per-authorized-account-transaction-msg-rate-limit").as<uint32_t>();

   my->rate_limits.per_code_account_time_frame_sec = fc::time_point_sec(options.at("per-code-account-transaction-msg-rate-limit-time-frame-sec").as<uint32_t>());
   my->rate_limits.per_code_account = options.at("per-code-account-transaction-msg-rate-limit").as<uint32_t>();*/
}

void chain_plugin::plugin_startup()
{ try {
   FC_ASSERT( fc::exists( my->genesis_file ),
              "unable to find genesis file '${f}', check --genesis-json argument", 
              ("f",my->genesis_file.generic_string()) );
   my->chain_config->block_log_dir = my->block_log_dir;
   my->chain_config->shared_memory_dir = app().data_dir() / default_shared_memory_dir;
   my->chain_config->read_only = my->readonly;
   my->chain_config->genesis = fc::json::from_file(my->genesis_file).as<contracts::genesis_state_type>();
   if (my->genesis_timestamp.sec_since_epoch() > 0) {
      my->chain_config->genesis.initial_timestamp = my->genesis_timestamp;
   }

   if (my->max_reversible_block_time_ms > 0) {
      my->chain_config->limits.max_push_block_us = fc::milliseconds(my->max_reversible_block_time_ms);
   }

   if (my->max_pending_transaction_time_ms > 0) {
      my->chain_config->limits.max_push_transaction_us = fc::milliseconds(my->max_pending_transaction_time_ms);
   }

   my->chain.emplace(*my->chain_config);

   if(!my->readonly) {
      ilog("starting chain in read/write mode");
      my->chain->add_checkpoints(my->loaded_checkpoints);
   }

   ilog("Blockchain started; head block is #${num}, genesis timestamp is ${ts}",
        ("num", my->chain->head_block_num())("ts", (std::string)my->chain_config->genesis.initial_timestamp));

   my->chain_config.reset();

   } FC_CAPTURE_AND_RETHROW( (my->genesis_file.generic_string()) ) }

void chain_plugin::plugin_shutdown() {
   my->chain.reset();
}

chain_apis::read_write chain_plugin::get_read_write_api() {
   return chain_apis::read_write(chain(), my->skip_flags);
}

bool chain_plugin::accept_block(const signed_block& block, bool currently_syncing) {
   if (currently_syncing && block.block_num() % 10000 == 0) {
      ilog("Syncing Blockchain --- Got block: #${n} time: ${t} producer: ${p}",
           ("t", block.timestamp)
           ("n", block.block_num())
           ("p", block.producer));
   }

#warning TODO: This used to be sync now it isnt?
   chain().push_block(block, my->skip_flags);
   return true;
}

void chain_plugin::accept_transaction(const packed_transaction& trx) {
   chain().push_transaction(trx, my->skip_flags);
}

bool chain_plugin::block_is_on_preferred_chain(const block_id_type& block_id) {
   // If it's not known, it's not preferred.
   if (!chain().is_known_block(block_id)) return false;
   // Extract the block number from block_id, and fetch that block number's ID from the database.
   // If the database's block ID matches block_id, then block_id is on the preferred chain. Otherwise, it's on a fork.
   return chain().get_block_id_for_num(block_header::num_from_id(block_id)) == block_id;
}

bool chain_plugin::is_skipping_transaction_signatures() const {
   return my->skip_flags & skip_transaction_signatures;
}

chain_controller::controller_config& chain_plugin::chain_config() {
   // will trigger optional assert if called before/after plugin_initialize()
   return *my->chain_config;
}

chain_controller& chain_plugin::chain() { return *my->chain; }
const chain_controller& chain_plugin::chain() const { return *my->chain; }

  void chain_plugin::get_chain_id (chain_id_type &cid)const {
    memcpy (cid.data(), my->chain_id.data(), cid.data_size());
  }

namespace chain_apis {

const string read_only::KEYi64 = "i64";
const string read_only::KEYstr = "str";
const string read_only::KEYi128i128 = "i128i128";
const string read_only::KEYi64i64i64 = "i64i64i64";
const string read_only::PRIMARY = "primary";
const string read_only::SECONDARY = "secondary";
const string read_only::TERTIARY = "tertiary";

read_only::get_info_results read_only::get_info(const read_only::get_info_params&) const {
   auto itoh = [](uint32_t n, size_t hlen = sizeof(uint32_t)<<1) {
    static const char* digits = "0123456789abcdef";
    std::string r(hlen, '0');
    for(size_t i = 0, j = (hlen - 1) * 4 ; i < hlen; ++i, j -= 4)
      r[i] = digits[(n>>j) & 0x0f];
    return r;
  };
   return {
      itoh(static_cast<uint32_t>(app().version())),
      db.head_block_num(),
      db.last_irreversible_block_num(),
      db.head_block_id(),
      db.head_block_time(),
      db.head_block_producer(),
      std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string(),
      __builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0
   };
}

abi_def get_abi( const chain_controller& db, const name& account ) {
   const auto& d = db.get_database();
   const auto& code_accnt  = d.get<account_object,by_name>( account );

   abi_def abi;
   abi_serializer::to_abi(code_accnt.abi, abi);
   return abi;
}

string get_table_type( const abi_def& abi, const name& table_name ) {
   for( const auto& t : abi.tables ) {
      if( t.name == table_name ){
         return t.index_type;
      }
   }
   FC_ASSERT( !"ABI does not define table", "Table ${table} not specified in ABI", ("table",table_name) );
}

read_only::get_table_rows_result read_only::get_table_rows( const read_only::get_table_rows_params& p )const {
   const abi_def abi = get_abi( db, p.code );
   auto table_type = get_table_type( abi, p.table );
   auto table_key = PRIMARY;

   if( table_type == KEYi64 ) {
      return get_table_rows_ex<contracts::key_value_index, contracts::by_scope_primary>(p,abi);
   } else if( table_type == KEYstr ) {
      return get_table_rows_ex<contracts::keystr_value_index, contracts::by_scope_primary>(p,abi);
   } else if( table_type == KEYi128i128 ) { 
      if( table_key == PRIMARY )
         return get_table_rows_ex<contracts::key128x128_value_index, contracts::by_scope_primary>(p,abi);
      if( table_key == SECONDARY )
         return get_table_rows_ex<contracts::key128x128_value_index, contracts::by_scope_secondary>(p,abi);
   } else if( table_type == KEYi64i64i64 ) {
      if( table_key == PRIMARY )
         return get_table_rows_ex<contracts::key64x64x64_value_index, contracts::by_scope_primary>(p,abi);
      if( table_key == SECONDARY )
         return get_table_rows_ex<contracts::key64x64x64_value_index, contracts::by_scope_secondary>(p,abi);
      if( table_key == TERTIARY )
         return get_table_rows_ex<contracts::key64x64x64_value_index, contracts::by_scope_tertiary>(p,abi);
   }
   FC_ASSERT( false, "invalid table type/key ${type}/${key}", ("type",table_type)("key",table_key)("abi",abi));
}

vector<asset> read_only::get_currency_balance( const read_only::get_currency_balance_params& p )const {
   vector<asset> results;
   walk_table<contracts::key_value_index, contracts::by_scope_primary>(p.code, p.account, N(account), [&](const contracts::key_value_object& obj){
      share_type balance;
      fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
      fc::raw::unpack(ds, balance);
      auto cursor = asset(balance, symbol(obj.primary_key));

      if (p.symbol || cursor.symbol_name().compare(*p.symbol) == 0) {
         results.emplace_back(balance, symbol(obj.primary_key));
      }

      // return false if we are looking for one and found it, true otherwise
      return p.symbol || cursor.symbol_name().compare(*p.symbol) != 0;
   });

   return results;
}

fc::variant read_only::get_currency_stats( const read_only::get_currency_stats_params& p )const {
   fc::mutable_variant_object results;
   walk_table<contracts::key_value_index, contracts::by_scope_primary>(p.code, p.code, N(stat), [&](const contracts::key_value_object& obj){
      share_type balance;
      fc::datastream<const char *> ds(obj.value.data(), obj.value.size());
      fc::raw::unpack(ds, balance);
      auto cursor = asset(balance, symbol(obj.primary_key));

      read_only::get_currency_stats_result result;
      result.supply = cursor;
      results[cursor.symbol_name()] = result;
      return true;
   });

   return results;
}

template<typename Api>
struct resolver_factory {
   static auto make(const Api *api) {
      return [api](const account_name &name) -> optional<abi_serializer> {
         const auto *accnt = api->db.get_database().template find<account_object, by_name>(name);
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
   optional<signed_block> block;
   try {
      block = db.fetch_block_by_id(fc::json::from_string(params.block_num_or_id).as<block_id_type>());
      if (!block) {
         block = db.fetch_block_by_number(fc::to_uint64(params.block_num_or_id));
      }

   } catch (fc::bad_cast_exception) {/* do nothing */}

   if (!block)
      FC_THROW_EXCEPTION(unknown_block_exception,
                      "Could not find block: ${block}", ("block", params.block_num_or_id));

   fc::variant pretty_output;
   abi_serializer::to_variant(*block, pretty_output, make_resolver(this));




   return fc::mutable_variant_object(pretty_output.get_object())
           ("id", block->id())
           ("block_num",block->block_num())
           ("ref_block_prefix", block->id()._hash[1]);
}

read_write::push_block_results read_write::push_block(const read_write::push_block_params& params) {
   db.push_block(params, skip_nothing);
   return read_write::push_block_results();
}

read_write::push_transaction_results read_write::push_transaction(const read_write::push_transaction_params& params) {
   packed_transaction pretty_input;
   auto resolver = make_resolver(this);
   abi_serializer::from_variant(params, pretty_input, resolver);
   auto result = db.push_transaction(pretty_input, skip_flags);
#warning TODO: get transaction results asynchronously
   fc::variant pretty_output;
   abi_serializer::to_variant(result, pretty_output, resolver);
   return read_write::push_transaction_results{ result.id, pretty_output };
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
   const auto& d = db.get_database();
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
   using namespace eosio::contracts;

   get_account_results result;
   result.account_name = params.account_name;

   const auto& d = db.get_database();

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


   return result;
}

read_only::abi_json_to_bin_result read_only::abi_json_to_bin( const read_only::abi_json_to_bin_params& params )const try {
   abi_json_to_bin_result result;
   const auto& code_account = db.get_database().get<account_object,by_name>( params.code );
   abi_def abi;
   if( abi_serializer::to_abi(code_account.abi, abi) ) {
      abi_serializer abis( abi );
      result.binargs = abis.variant_to_binary( abis.get_action_type( params.action ), params.args );
   }
   return result;
} FC_CAPTURE_AND_RETHROW( (params.code)(params.action)(params.args) )

read_only::abi_bin_to_json_result read_only::abi_bin_to_json( const read_only::abi_bin_to_json_params& params )const {
   abi_bin_to_json_result result;
   const auto& code_account = db.get_database().get<account_object,by_name>( params.code );
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
   auto required_keys_set = db.get_required_keys(pretty_input, params.available_keys);
   get_required_keys_result result;
   result.required_keys = required_keys_set;
   return result;
}


} // namespace chain_apis
} // namespace eosio
