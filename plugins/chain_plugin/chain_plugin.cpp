#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/chain/fork_database.hpp>
#include <eos/chain/block_log.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/config.hpp>
#include <eos/chain/types.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/native_contract_chain_administrator.hpp>
#include <eos/native_contract/staked_balance_objects.hpp>
#include <eos/native_contract/balance_object.hpp>
#include <eos/native_contract/genesis_state.hpp>

#include <eos/utilities/key_conversion.hpp>
#include <eos/chain/wast_to_wasm.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>

namespace eos {

using namespace eos;
using fc::flat_map;
using chain::block_id_type;
using chain::fork_database;
using chain::block_log;
using chain::chain_id_type;
using chain::account_object;
using chain::key_value_object;
using chain::key128x128_value_object;
using chain::key64x64x64_value_object;
using chain::by_name;
using chain::by_scope_primary;
using chain::uint128_t;


class chain_plugin_impl {
public:
   bfs::path                        block_log_dir;
   bfs::path                        genesis_file;
   chain::Time                      genesis_timestamp;
   uint32_t                         skip_flags = chain_controller::skip_nothing;
   bool                             readonly = false;
   flat_map<uint32_t,block_id_type> loaded_checkpoints;

   fc::optional<fork_database>      fork_db;
   fc::optional<block_log>          block_logger;
   fc::optional<chain_controller>   chain;
   chain_id_type                    chain_id;
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
         ;
   cli.add_options()
         ("replay-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain database and replay all blocks")
         ("resync-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain database and block log")
         ("skip-transaction-signatures", bpo::bool_switch()->default_value(false),
          "Disable Transaction signature verification. ONLY for TESTING.")
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
       auto diff = my->genesis_timestamp.sec_since_epoch() % config::BlockIntervalSeconds;
       if (diff > 0) {
         auto delay =  (config::BlockIntervalSeconds - diff);
         my->genesis_timestamp += delay;
         dlog ("pausing ${s} seconds to the next interval",("s",delay));
       }
     }
     else {
       my->genesis_timestamp = chain::Time::from_iso_string (tstr);
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
      app().get_plugin<database_plugin>().wipe_database();
   }
   if (options.at("resync-blockchain").as<bool>()) {
      ilog("Resync requested: wiping blocks");
      app().get_plugin<database_plugin>().wipe_database();
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

      my->skip_flags |= chain_controller::skip_transaction_signatures;
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
}

void chain_plugin::plugin_startup() 
{ try {
   auto& db = app().get_plugin<database_plugin>().db();

   FC_ASSERT( fc::exists( my->genesis_file ), 
              "unable to find genesis file '${f}', check --genesis-json argument", 
              ("f",my->genesis_file.generic_string()) );

   auto genesis = fc::json::from_file(my->genesis_file).as<native_contract::genesis_state_type>();
   if (my->genesis_timestamp.sec_since_epoch() > 0) {
     genesis.initial_timestamp = my->genesis_timestamp;
   }

   native_contract::native_contract_chain_initializer initializer(genesis);

   my->fork_db = fork_database();
   my->block_logger = block_log(my->block_log_dir);
   my->chain_id = genesis.compute_chain_id();
   my->chain = chain_controller(db, *my->fork_db, *my->block_logger,
                                initializer, native_contract::make_administrator());

   if(!my->readonly) {
      ilog("starting chain in read/write mode");
      my->chain->add_checkpoints(my->loaded_checkpoints);
   }

   ilog("Blockchain started; head block is #${num}, genesis timestamp is ${ts}",
        ("num", my->chain->head_block_num())("ts", genesis.initial_timestamp.to_iso_string()));

} FC_CAPTURE_AND_RETHROW( (my->genesis_file.generic_string()) ) }

void chain_plugin::plugin_shutdown() {
}

chain_apis::read_write chain_plugin::get_read_write_api() {
   return chain_apis::read_write(chain(), my->skip_flags);
}

bool chain_plugin::accept_block(const chain::signed_block& block, bool currently_syncing) {
   if (currently_syncing && block.block_num() % 10000 == 0) {
      ilog("Syncing Blockchain --- Got block: #${n} time: ${t} producer: ${p}",
           ("t", block.timestamp)
           ("n", block.block_num())
           ("p", block.producer));
   }

   return chain().push_block(block, my->skip_flags);
}

void chain_plugin::accept_transaction(const chain::SignedTransaction& trx) {
   chain().push_transaction(trx, my->skip_flags);
}

bool chain_plugin::block_is_on_preferred_chain(const chain::block_id_type& block_id) {
   // If it's not known, it's not preferred.
   if (!chain().is_known_block(block_id)) return false;
   // Extract the block number from block_id, and fetch that block number's ID from the database.
   // If the database's block ID matches block_id, then block_id is on the preferred chain. Otherwise, it's on a fork.
   return chain().get_block_id_for_num(chain::block_header::num_from_id(block_id)) == block_id;
}

bool chain_plugin::is_skipping_transaction_signatures() const {
   return my->skip_flags & chain_controller::skip_transaction_signatures;
}

chain_controller& chain_plugin::chain() { return *my->chain; }
const chain::chain_controller& chain_plugin::chain() const { return *my->chain; }

  void chain_plugin::get_chain_id (chain_id_type &cid)const {
    memcpy (cid.data(), my->chain_id.data(), cid.data_size());
  }

namespace chain_apis {

read_only::get_info_results read_only::get_info(const read_only::get_info_params&) const {
   return {
      db.head_block_num(),
      db.last_irreversible_block_num(),
      db.head_block_id(),
      db.head_block_time(),
      db.head_block_producer(),
      std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string(),
      __builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0
   };
}

types::Abi getAbi( const chain_controller& db, const Name& account ) {
   const auto& d = db.get_database();
   const auto& code_accnt  = d.get<account_object,by_name>( account );

   eos::types::Abi abi;
   if( code_accnt.abi.size() > 4 ) {
      fc::datastream<const char*> ds( code_accnt.abi.data(), code_accnt.abi.size() );
      fc::raw::unpack( ds, abi );
   }
   return abi;
}

string getTableType( const types::Abi& abi, const Name& tablename ) {
   for( const auto& t : abi.tables ) {
      if( t.table == tablename )
         return t.indextype;
   }
   FC_ASSERT( !"Abi does not define table", "Table ${table} not specified in ABI", ("table",tablename) );
}

read_only::get_table_rows_result read_only::get_table_rows( const read_only::get_table_rows_params& p )const {
   const auto& d = db.get_database();

   const types::Abi abi = getAbi( db, p.code );
   auto table_type = getTableType( abi, p.table );
   auto table_key = PRIMARY;

   if( table_type == KEYi64 ) {
      return get_table_rows_ex<chain::key_value_index, chain::by_scope_primary>(p,abi);
   } else if( table_type == KEYi128i128 ) { 
      if( table_key == PRIMARY )
         return get_table_rows_ex<chain::key128x128_value_index, chain::by_scope_primary>(p,abi);
      if( table_key == SECONDARY )
         return get_table_rows_ex<chain::key128x128_value_index, chain::by_scope_secondary>(p,abi);
   } else if( table_type == KEYi64i64i64 ) {
      if( table_key == PRIMARY )
         return get_table_rows_ex<chain::key64x64x64_value_index, chain::by_scope_primary>(p,abi);
      if( table_key == SECONDARY )
         return get_table_rows_ex<chain::key64x64x64_value_index, chain::by_scope_secondary>(p,abi);
      if( table_key == TERTIARY )
         return get_table_rows_ex<chain::key64x64x64_value_index, chain::by_scope_tertiary>(p,abi);
   }
   FC_ASSERT( false, "invalid table type/key ${type}/${key}", ("type",table_type)("key",table_key)("abi",abi));
}

read_only::get_block_results read_only::get_block(const read_only::get_block_params& params) const {
   try {
      if (auto block = db.fetch_block_by_id(fc::json::from_string(params.block_num_or_id).as<chain::block_id_type>()))
         return *block;
   } catch (fc::bad_cast_exception) {/* do nothing */}
   try {
      if (auto block = db.fetch_block_by_number(fc::to_uint64(params.block_num_or_id)))
         return *block;
   } catch (fc::bad_cast_exception) {/* do nothing */}

   FC_THROW_EXCEPTION(chain::unknown_block_exception,
                      "Could not find block: ${block}", ("block", params.block_num_or_id));
}

read_write::push_block_results read_write::push_block(const read_write::push_block_params& params) {
   db.push_block(params);
   return read_write::push_block_results();
}

read_write::push_transaction_results read_write::push_transaction(const read_write::push_transaction_params& params) {
   auto pretty_input = db.transaction_from_variant( params );
   auto ptrx = db.push_transaction(pretty_input, skip_flags);
   auto pretty_trx = db.transaction_to_variant( ptrx );
   return read_write::push_transaction_results{ pretty_input.id(), pretty_trx };
}

read_write::push_transactions_results read_write::push_transactions(const read_write::push_transactions_params& params) {
   FC_ASSERT( params.size() <= 1000, "Attempt to push too many transactions at once" );

   push_transactions_results result;
   result.reserve(params.size());
   for( const auto& item : params ) {
      try {
        result.emplace_back( push_transaction( item ) ); 
      } catch ( const fc::exception& e ) {
        result.emplace_back( read_write::push_transaction_results{ chain::transaction_id_type(), 
                          fc::mutable_variant_object( "error", e.to_detail_string() ) } );
      }
   }
   return result;
}

read_only::get_code_results read_only::get_code( const get_code_params& params )const {
   get_code_results result;
   result.name = params.name;
   const auto& d = db.get_database();
   const auto& accnt  = d.get<account_object,by_name>( params.name );

   if( accnt.code.size() ) {
      result.wast = chain::wasm_to_wast( (const uint8_t*)accnt.code.data(), accnt.code.size() );
      result.code_hash = fc::sha256::hash( accnt.code.data(), accnt.code.size() );
   }
   if( accnt.abi.size() > 4 ) {
      eos::types::Abi abi;
      fc::datastream<const char*> ds( accnt.abi.data(), accnt.abi.size() );
      fc::raw::unpack( ds, abi );
      result.abi = std::move(abi);
   }
   return result;
}

read_only::get_account_results read_only::get_account( const get_account_params& params )const {
   using namespace native::eos;

   get_account_results result;
   result.name = params.name;

   const auto& d = db.get_database();
   const auto& accnt          = d.get<account_object,by_name>( params.name );
   const auto& balance        = d.get<BalanceObject,byOwnerName>( params.name );
   const auto& staked_balance = d.get<StakedBalanceObject,byOwnerName>( params.name );

   result.eos_balance          = Asset(balance.balance, EOS_SYMBOL);
   result.staked_balance       = Asset(staked_balance.stakedBalance);
   result.unstaking_balance    = Asset(staked_balance.unstakingBalance);
   result.last_unstaking_time  = staked_balance.lastUnstakingTime;

   const auto& permissions = d.get_index<permission_index,by_owner>();
   auto perm = permissions.lower_bound( boost::make_tuple( params.name ) );
   while( perm != permissions.end() && perm->owner == params.name ) {
      /// TODO: lookup perm->parent name 
      Name parent;

      const auto* p = d.find<permission_object,by_id>( perm->parent );
      if( p ) parent = p->name;

      result.permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
      ++perm;
   }


   return result;
}
read_only::abi_json_to_bin_result read_only::abi_json_to_bin( const read_only::abi_json_to_bin_params& params )const {
   abi_json_to_bin_result result;
   result.binargs = db.message_to_binary( params.code, params.action, params.args );
   return result;
}
read_only::abi_bin_to_json_result read_only::abi_bin_to_json( const read_only::abi_bin_to_json_params& params )const {
   abi_bin_to_json_result result;
   result.args = db.message_from_binary( params.code, params.action, params.binargs );
   return result;
}

read_only::get_required_keys_result read_only::get_required_keys( const get_required_keys_params& params )const {
   auto pretty_input = db.transaction_from_variant(params.transaction);
   auto required_keys_set = db.get_required_keys(pretty_input, params.available_keys);
   get_required_keys_result result;
   result.required_keys = required_keys_set;
   return result;
}


} // namespace chain_apis
} // namespace eos
