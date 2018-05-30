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
#include <eosio/chain/reversible_block_object.hpp>

#include <eosio/chain/eosio_contract.hpp>

#include <eosio/utilities/key_conversion.hpp>
#include <eosio/utilities/common.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <boost/signals2/connection.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <signal.h>

namespace eosio {

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::config;
using namespace eosio::chain::plugin_interface;
using vm_type = wasm_interface::vm_type;
using fc::flat_map;

using boost::signals2::scoped_connection;

//using txn_msg_rate_limits = controller::txn_msg_rate_limits;

#define CATCH_AND_CALL(NEXT)\
   catch ( const fc::exception& err ) {\
      NEXT(err.dynamic_copy_exception());\
   } catch ( const std::exception& e ) {\
      fc::exception fce( \
         FC_LOG_MESSAGE( warn, "rethrow ${what}: ", ("what",e.what())),\
         fc::std_exception_code,\
         BOOST_CORE_TYPEID(e).name(),\
         e.what() ) ;\
      NEXT(fce.dynamic_copy_exception());\
   } catch( ... ) {\
      fc::unhandled_exception e(\
         FC_LOG_MESSAGE(warn, "rethrow"),\
         std::current_exception());\
      NEXT(e.dynamic_copy_exception());\
   }


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
   ,incoming_transaction_async_method(app().get_method<incoming::methods::transaction_async>())
   {}

   bfs::path                        blocks_dir;
   bool                             readonly = false;
   flat_map<uint32_t,block_id_type> loaded_checkpoints;

   fc::optional<fork_database>      fork_db;
   fc::optional<block_log>          block_logger;
   fc::optional<controller::config> chain_config;
   fc::optional<controller>         chain;
   fc::optional<chain_id_type>      chain_id;
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
   incoming::methods::block_sync::method_type&        incoming_block_sync_method;
   incoming::methods::transaction_async::method_type& incoming_transaction_async_method;

   // method provider handles
   methods::get_block_by_number::method_type::handle                 get_block_by_number_provider;
   methods::get_block_by_id::method_type::handle                     get_block_by_id_provider;
   methods::get_head_block_id::method_type::handle                   get_head_block_id_provider;
   methods::get_last_irreversible_block_number::method_type::handle  get_last_irreversible_block_number_provider;

   // scoped connections for chain controller
   fc::optional<scoped_connection>                                   accepted_block_header_connection;
   fc::optional<scoped_connection>                                   accepted_block_connection;
   fc::optional<scoped_connection>                                   irreversible_block_connection;
   fc::optional<scoped_connection>                                   accepted_transaction_connection;
   fc::optional<scoped_connection>                                   applied_transaction_connection;
   fc::optional<scoped_connection>                                   accepted_confirmation_connection;


};

chain_plugin::chain_plugin()
:my(new chain_plugin_impl()) {
}

chain_plugin::~chain_plugin(){}

void chain_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   cfg.add_options()
         ("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"),
          "the location of the blocks directory (absolute path or relative to application data dir)")
         ("checkpoint", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("wasm-runtime", bpo::value<eosio::chain::wasm_interface::vm_type>()->value_name("wavm/binaryen"), "Override default WASM runtime")
         ("chain-state-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_state_size / (1024  * 1024)), "Maximum size (in MB) of the chain state database")
         ("reversible-blocks-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_reversible_cache_size / (1024  * 1024)), "Maximum size (in MB) of the reversible blocks database")
         ("contracts-console", bpo::bool_switch()->default_value(false),
          "print contract's output to console")
         ("actor-whitelist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Account added to actor whitelist (may specify multiple times)")
         ("actor-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Account added to actor blacklist (may specify multiple times)")
         ("contract-whitelist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Contract account added to contract whitelist (may specify multiple times)")
         ("contract-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Contract account added to contract blacklist (may specify multiple times)")
         ;

#warning TODO: rate limiting
         /*("per-authorized-account-transaction-msg-rate-limit-time-frame-sec", bpo::value<uint32_t>()->default_value(default_per_auth_account_time_frame_seconds),
          "The time frame, in seconds, that the per-authorized-account-transaction-msg-rate-limit is imposed over.")
         ("per-authorized-account-transaction-msg-rate-limit", bpo::value<uint32_t>()->default_value(default_per_auth_account),
          "Limits the maximum rate of transaction messages that an account is allowed each per-authorized-account-transaction-msg-rate-limit-time-frame-sec.")
          ("per-code-account-transaction-msg-rate-limit-time-frame-sec", bpo::value<uint32_t>()->default_value(default_per_code_account_time_frame_seconds),
           "The time frame, in seconds, that the per-code-account-transaction-msg-rate-limit is imposed over.")
          ("per-code-account-transaction-msg-rate-limit", bpo::value<uint32_t>()->default_value(default_per_code_account),
           "Limits the maximum rate of transaction messages that an account's code is allowed each per-code-account-transaction-msg-rate-limit-time-frame-sec.")*/

   cli.add_options()
         ("genesis-json", bpo::value<bfs::path>(), "File to read Genesis State from")
         ("genesis-timestamp", bpo::value<string>(), "override the initial timestamp in the Genesis State file")
         ("print-genesis-json", bpo::bool_switch()->default_value(false),
           "extract genesis_state from blocks.log as JSON, print to console, and exit")
         ("extract-genesis-json", bpo::value<bfs::path>(),
           "extract genesis_state from blocks.log as JSON, write into specified file, and exit")
         ("fix-reversible-blocks", bpo::bool_switch()->default_value(false),
          "recovers reversible block database if that database is in a bad state")
         ("force-all-checks", bpo::bool_switch()->default_value(false),
          "do not skip any checks that can be skipped while replaying irreversible blocks")
         ("replay-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain state database and replay all blocks")
         ("hard-replay-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain state database, recover as many blocks as possible from the block log, and then replay those blocks")
         ("delete-all-blocks", bpo::bool_switch()->default_value(false),
          "clear chain state database and block log")
         ;
}

#define LOAD_VALUE_SET(options, name, container) \
if( options.count(name) ) { \
   const std::vector<std::string>& ops = options[name].as<std::vector<std::string>>(); \
   std::copy(ops.begin(), ops.end(), std::inserter(container, container.end())); \
}

fc::time_point calculate_genesis_timestamp( string tstr ) {
   fc::time_point genesis_timestamp;
   if( strcasecmp (tstr.c_str(), "now") == 0 ) {
      genesis_timestamp = fc::time_point::now();
   } else {
      genesis_timestamp = time_point::from_iso_string( tstr );
   }

   auto epoch_us = genesis_timestamp.time_since_epoch().count();
   auto diff_us = epoch_us % config::block_interval_us;
   if (diff_us > 0) {
      auto delay_us = (config::block_interval_us - diff_us);
      genesis_timestamp += fc::microseconds(delay_us);
      dlog("pausing ${us} microseconds to the next interval",("us",delay_us));
   }

   ilog( "Adjusting genesis timestamp to ${timestamp}", ("timestamp", genesis_timestamp) );
   return genesis_timestamp;
}

void chain_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing chain plugin");

   try {
      genesis_state gs; // Check if EOSIO_ROOT_KEY is bad
   } catch( const fc::exception& ) {
      elog( "EOSIO_ROOT_KEY ('${root_key}') is invalid. Recompile with a valid public key.",
            ("root_key", genesis_state::eosio_root_key) );
      throw;
   }

   my->chain_config = controller::config();

   LOAD_VALUE_SET(options, "actor-whitelist", my->chain_config->actor_whitelist);
   LOAD_VALUE_SET(options, "actor-blacklist", my->chain_config->actor_blacklist);
   LOAD_VALUE_SET(options, "contract-whitelist", my->chain_config->contract_whitelist);
   LOAD_VALUE_SET(options, "contract-blacklist", my->chain_config->contract_blacklist);

   if( options.count("blocks-dir") )
   {
      auto bld = options.at("blocks-dir").as<bfs::path>();
      if(bld.is_relative())
         my->blocks_dir = app().data_dir() / bld;
      else
         my->blocks_dir = bld;
   }

   if( options.count("checkpoint") )
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

   my->chain_config->blocks_dir = my->blocks_dir;
   my->chain_config->state_dir = app().data_dir() / config::default_state_dir_name;
   my->chain_config->read_only = my->readonly;

   if (options.count("chain-state-db-size-mb"))
      my->chain_config->state_size = options.at("chain-state-db-size-mb").as<uint64_t>() * 1024 * 1024;

   if (options.count("reversible-blocks-db-size-mb"))
      my->chain_config->reversible_cache_size = options.at("reversible-blocks-db-size-mb").as<uint64_t>() * 1024 * 1024;

   if( my->wasm_runtime )
      my->chain_config->wasm_runtime = *my->wasm_runtime;

   my->chain_config->force_all_checks  = options.at("force-all-checks").as<bool>();
   my->chain_config->contracts_console = options.at("contracts-console").as<bool>();

   if( options.count("extract-genesis-json") ||  options.at("print-genesis-json").as<bool>() ) {
      genesis_state gs;

      if( fc::exists( my->blocks_dir / "blocks.log" ) ) {
         gs = block_log::extract_genesis_state( my->blocks_dir );
      } else {
         wlog( "No blocks.log found at '${p}'. Using default genesis state.", ("p", (my->blocks_dir / "blocks.log").generic_string()) );
      }

      if( options.at("print-genesis-json").as<bool>() ) {
         ilog( "Genesis JSON:\n${genesis}", ("genesis", json::to_pretty_string(gs)) );
      }

      if( options.count("extract-genesis-json") ) {
         auto p = options.at("extract-genesis-json").as<bfs::path>();

         if( p.is_relative() ) {
            p = bfs::current_path() / p;
         }

         fc::json::save_to_file( gs, p, true );
         ilog( "Saved genesis JSON to '${path}'", ("path", p.generic_string()) );
      }

      EOS_THROW( extract_genesis_state_exception, "extracted genesis state from blocks.log" );
   }

   if( options.at("delete-all-blocks").as<bool>() ) {
      ilog("Deleting state database and blocks");
      fc::remove_all( my->chain_config->state_dir );
      fc::remove_all(my->blocks_dir);
   } else if( options.at("hard-replay-blockchain").as<bool>() ) {
      ilog("Hard replay requested: deleting state database");
      fc::remove_all( my->chain_config->state_dir );
      auto backup_dir = block_log::repair_log( my->blocks_dir );
      if( fc::exists(backup_dir/config::reversible_blocks_dir_name) || options.at("fix-reversible-blocks").as<bool>() ) {
         // Do not try to recover reversible blocks if the directory does not exist, unless the option was explicitly provided.
         if( !recover_reversible_blocks( backup_dir/config::reversible_blocks_dir_name,
                                          my->chain_config->reversible_cache_size,
                                          my->chain_config->blocks_dir/config::reversible_blocks_dir_name ) ) {
            ilog("Reversible blocks database was not corrupted. Copying from backup to blocks directory.");
            fc::copy( backup_dir/config::reversible_blocks_dir_name, my->chain_config->blocks_dir/config::reversible_blocks_dir_name );
            fc::copy( backup_dir/"reversible/shared_memory.bin", my->chain_config->blocks_dir/"reversible/shared_memory.bin" );
            fc::copy( backup_dir/"reversible/shared_memory.meta", my->chain_config->blocks_dir/"reversible/shared_memory.meta" );
         }
      }
   } else if( options.at("replay-blockchain").as<bool>() ) {
      ilog("Replay requested: deleting state database");
      fc::remove_all( my->chain_config->state_dir );
      if( options.at("fix-reversible-blocks").as<bool>() ) {
         if( !recover_reversible_blocks( my->chain_config->blocks_dir/config::reversible_blocks_dir_name,
                                          my->chain_config->reversible_cache_size ) ) {
            ilog("Reversible blocks database was not corrupted.");
         }
      }
   } else if( options.at("fix-reversible-blocks").as<bool>() ) {
      if( !recover_reversible_blocks( my->chain_config->blocks_dir/config::reversible_blocks_dir_name,
                                       my->chain_config->reversible_cache_size ) ) {
         ilog("Reversible blocks database verified to not be corrupted. Now exiting...");
      } else {
         ilog("Exiting after fixing reversible blocks database...");
      }
      EOS_THROW( fixed_reversible_db_exception, "fixed corrupted reversible blocks database" );
   }

   if( options.count("genesis-json") ) {
      FC_ASSERT( !fc::exists( my->blocks_dir / "blocks.log" ), "Genesis state can only be set on a fresh blockchain." );

      auto genesis_file = options.at("genesis-json").as<bfs::path>();
      if( genesis_file.is_relative() ) {
         genesis_file = bfs::current_path() / genesis_file;
      }

      FC_ASSERT( fc::is_regular_file( genesis_file ),
                 "Specified genesis file '${genesis}' does not exist.",
                 ("genesis", genesis_file.generic_string()) );

      my->chain_config->genesis = fc::json::from_file(genesis_file).as<genesis_state>();

      ilog( "Using genesis state provided in '${genesis}'", ("genesis", genesis_file.generic_string()) );

      if( options.count("genesis-timestamp") ) {
         my->chain_config->genesis.initial_timestamp = calculate_genesis_timestamp( options.at("genesis-timestamp").as<string>() );
      }

      wlog( "Starting up fresh blockchain with provided genesis state." );
   } else if( options.count("genesis-timestamp") ) {
      FC_ASSERT( !fc::exists( my->blocks_dir / "blocks.log" ), "Genesis state can only be set on a fresh blockchain." );

      my->chain_config->genesis.initial_timestamp = calculate_genesis_timestamp( options.at("genesis-timestamp").as<string>() );

      wlog( "Starting up fresh blockchain with default genesis state but with adjusted genesis timestamp." );
   } else if( fc::is_regular_file( my->blocks_dir / "blocks.log" ) ) {
      my->chain_config->genesis = block_log::extract_genesis_state( my->blocks_dir );
   } else {
      wlog( "Starting up fresh blockchain with default genesis state." );
   }

   my->chain.emplace(*my->chain_config);
   my->chain_id.emplace(my->chain->get_chain_id());

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
   my->accepted_block_header_connection = my->chain->accepted_block_header.connect([this](const block_state_ptr& blk) {
      my->accepted_block_header_channel.publish(blk);
   });

   my->accepted_block_connection = my->chain->accepted_block.connect([this](const block_state_ptr& blk) {
      my->accepted_block_channel.publish(blk);
   });

   my->irreversible_block_connection = my->chain->irreversible_block.connect([this](const block_state_ptr& blk) {
      my->irreversible_block_channel.publish(blk);
   });

   my->accepted_transaction_connection = my->chain->accepted_transaction.connect([this](const transaction_metadata_ptr& meta){
      my->accepted_transaction_channel.publish(meta);
   });

   my->applied_transaction_connection = my->chain->applied_transaction.connect([this](const transaction_trace_ptr& trace){
      my->applied_transaction_channel.publish(trace);
   });

   my->accepted_confirmation_connection = my->chain->accepted_confirmation.connect([this](const header_confirmation& conf){
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
} FC_CAPTURE_AND_RETHROW() }

void chain_plugin::plugin_shutdown() {
   my->accepted_block_header_connection.reset();
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();
   my->accepted_confirmation_connection.reset();
   my->chain.reset();
}

chain_apis::read_write chain_plugin::get_read_write_api() {
   return chain_apis::read_write(chain());
}

void chain_plugin::accept_block(const signed_block_ptr& block ) {
   my->incoming_block_sync_method(block);
}

void chain_plugin::accept_transaction(const chain::packed_transaction& trx, next_function<chain::transaction_trace_ptr> next) {
   my->incoming_transaction_async_method(std::make_shared<packed_transaction>(trx), false, std::forward<decltype(next)>(next));
}

bool chain_plugin::block_is_on_preferred_chain(const block_id_type& block_id) {
   auto b = chain().fetch_block_by_number( block_header::num_from_id(block_id) );
   return b && b->id() == block_id;
}

bool chain_plugin::recover_reversible_blocks( const fc::path& db_dir, uint32_t cache_size, optional<fc::path> new_db_dir )const {
   try {
      chainbase::database reversible( db_dir, database::read_only); // Test if dirty
      return false; // If it reaches here, then the reversible database is not dirty
   } catch( const std::runtime_error& ) {
   } catch( ... ) {
      throw;
   }
   // Reversible block database is dirty. So back it up (unless already moved) and then create a new one.

   auto reversible_dir = fc::canonical( db_dir );
   if( reversible_dir.filename().generic_string() == "." ) {
      reversible_dir = reversible_dir.parent_path();
   }
   fc::path backup_dir;

   if( new_db_dir ) {
      backup_dir = reversible_dir;
      reversible_dir = *new_db_dir;
   } else {
      auto now = fc::time_point::now();

      auto reversible_dir_name = reversible_dir.filename().generic_string();
      FC_ASSERT( reversible_dir_name != ".", "Invalid path to reversible directory" );
      backup_dir = reversible_dir.parent_path() / reversible_dir_name.append("-").append( now );

      FC_ASSERT( !fc::exists(backup_dir),
                 "Cannot move existing reversible directory to already existing directory '${backup_dir}'",
                 ("backup_dir", backup_dir) );

      fc::rename( reversible_dir, backup_dir );
      ilog( "Moved existing reversible directory to backup location: '${new_db_dir}'", ("new_db_dir", backup_dir) );
   }

   fc::create_directories( reversible_dir );

   ilog( "Reconstructing '${reversible_dir}' from backed up reversible directory", ("reversible_dir", reversible_dir) );

   chainbase::database  old_reversible( backup_dir, database::read_only, 0, true );
   chainbase::database  new_reversible( reversible_dir, database::read_write, cache_size );

   uint32_t num = 0;
   uint32_t start = 0;
   uint32_t end = 0;
   try {
      old_reversible.add_index<reversible_block_index>();
      new_reversible.add_index<reversible_block_index>();
      const auto& ubi = old_reversible.get_index<reversible_block_index,by_num>();
      auto itr = ubi.begin();
      if( itr != ubi.end() ) {
         start = itr->blocknum;
         end = start - 1;
      }
      for( ; itr != ubi.end(); ++itr ) {
         FC_ASSERT( itr->blocknum == end + 1, "gap in reversible block database" );
         new_reversible.create<reversible_block_object>( [&]( auto& ubo ) {
            ubo.blocknum = itr->blocknum;
            ubo.set_block( itr->get_block() ); // get_block and set_block rather than copying the packed data acts as additional validation
         });
         end = itr->blocknum;
         ++num;
      }
   } catch( ... ) {}

   if( num == 0 )
      ilog( "There were no recoverable blocks in the reversible block database" );
   else if( num == 1 )
      ilog( "Recovered 1 block from reversible block database: block ${start}", ("start", start) );
   else
      ilog( "Recovered ${num} blocks from reversible block database: blocks ${start} to ${end}",
            ("num", num)("start", start)("end", end) );

   return true;
}

controller::config& chain_plugin::chain_config() {
   // will trigger optional assert if called before/after plugin_initialize()
   return *my->chain_config;
}

controller& chain_plugin::chain() { return *my->chain; }
const controller& chain_plugin::chain() const { return *my->chain; }

chain::chain_id_type chain_plugin::get_chain_id()const {
   FC_ASSERT( my->chain_id.valid(), "chain ID has not been initialized yet" );
   return *my->chain_id;
}

namespace chain_apis {

const string read_only::KEYi64 = "i64";

read_only::get_info_results read_only::get_info(const read_only::get_info_params&) const {
   const auto& rm = db.get_resource_limits_manager();
   return {
      eosio::utilities::common::itoh(static_cast<uint32_t>(app().version())),
      db.get_chain_id(),
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

// TODO: move this and similar functions to a header. Copied from wasm_interface.cpp.
// TODO: fix strict aliasing violation
static float64_t to_softfloat64( double d ) {
   return *reinterpret_cast<float64_t*>(&d);
}

static fc::variant get_global_row( const database& db, const abi_def& abi, const abi_serializer& abis ) {
   const auto table_type = get_table_type(abi, N(global));
   EOS_ASSERT(table_type == read_only::KEYi64, chain::contract_table_query_exception, "Invalid table type ${type} for table global", ("type",table_type));

   const auto* const table_id = db.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(N(eosio), N(eosio), N(global)));
   EOS_ASSERT(table_id, chain::contract_table_query_exception, "Missing table global");

   const auto& kv_index = db.get_index<key_value_index, by_scope_primary>();
   const auto it = kv_index.find(boost::make_tuple(table_id->id, N(global)));
   EOS_ASSERT(it != kv_index.end(), chain::contract_table_query_exception, "Missing row in table global");

   vector<char> data;
   read_only::copy_inline_row(*it, data);
   return abis.binary_to_variant(abis.get_table_type(N(global)), data);
}

read_only::get_producers_result read_only::get_producers( const read_only::get_producers_params& p ) const {
   const abi_def abi = get_abi(db, N(eosio));
   const auto table_type = get_table_type(abi, N(producers));
   const abi_serializer abis{ abi };
   EOS_ASSERT(table_type == KEYi64, chain::contract_table_query_exception, "Invalid table type ${type} for table producers", ("type",table_type));

   const auto& d = db.db();
   const auto lower = name{p.lower_bound};

   static const uint8_t secondary_index_num = 0;
   const auto* const table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(N(eosio), N(eosio), N(producers)));
   const auto* const secondary_table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(boost::make_tuple(N(eosio), N(eosio), N(producers) | secondary_index_num));
   EOS_ASSERT(table_id && secondary_table_id, chain::contract_table_query_exception, "Missing producers table");

   const auto& kv_index = d.get_index<key_value_index, by_scope_primary>();
   const auto& secondary_index = d.get_index<index_double_index>().indices();
   const auto& secondary_index_by_primary = secondary_index.get<by_primary>();
   const auto& secondary_index_by_secondary = secondary_index.get<by_secondary>();

   read_only::get_producers_result result;
   const auto stopTime = fc::time_point::now() + fc::microseconds(1000 * 10); // 10ms
   vector<char> data;

   auto it = [&]{
      if(lower.value == 0)
         return secondary_index_by_secondary.lower_bound(
            boost::make_tuple(secondary_table_id->id, to_softfloat64(std::numeric_limits<double>::lowest()), 0));
      else
         return secondary_index.project<by_secondary>(
            secondary_index_by_primary.lower_bound(
               boost::make_tuple(secondary_table_id->id, lower.value)));
   }();

   for( ; it != secondary_index_by_secondary.end() && it->t_id == secondary_table_id->id; ++it ) {
      if (result.rows.size() >= p.limit || fc::time_point::now() > stopTime) {
         result.more = name{it->primary_key}.to_string();
         break;
      }
      copy_inline_row(*kv_index.find(boost::make_tuple(table_id->id, it->primary_key)), data);
      if (p.json)
         result.rows.emplace_back(abis.binary_to_variant(abis.get_table_type(N(producers)), data));
      else
         result.rows.emplace_back(fc::variant(data));
   }

   result.total_producer_vote_weight = get_global_row(d, abi, abis)["total_producer_vote_weight"].as_double();
   return result;
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

void read_write::push_block(const read_write::push_block_params& params, next_function<read_write::push_block_results> next) {
   try {
      app().get_method<incoming::methods::block_sync>()(std::make_shared<signed_block>(params));
      next(read_write::push_block_results{});
   } catch ( boost::interprocess::bad_alloc& ) {
      raise(SIGUSR1);
   } CATCH_AND_CALL(next);
}

void read_write::push_transaction(const read_write::push_transaction_params& params, next_function<read_write::push_transaction_results> next) {

   try {
      auto pretty_input = std::make_shared<packed_transaction>();
      auto resolver = make_resolver(this);
      try {
         abi_serializer::from_variant(params, *pretty_input, resolver);
      } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

      app().get_method<incoming::methods::transaction_async>()(pretty_input, true, [this, next](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result) -> void{
         if (result.contains<fc::exception_ptr>()) {
            next(result.get<fc::exception_ptr>());
         } else {
            auto trx_trace_ptr = result.get<transaction_trace_ptr>();

            try {
               fc::variant pretty_output;
               pretty_output = db.to_variant_with_abi(*trx_trace_ptr);
               //abi_serializer::to_variant(*trx_trace_ptr, pretty_output, resolver);

               chain::transaction_id_type id = trx_trace_ptr->id;
               next(read_write::push_transaction_results{id, pretty_output});
            } CATCH_AND_CALL(next);
         }
      });


   } catch ( boost::interprocess::bad_alloc& ) {
      raise(SIGUSR1);
   } CATCH_AND_CALL(next);
}

static void push_recurse(read_write* rw, int index, const std::shared_ptr<read_write::push_transactions_params>& params, const std::shared_ptr<read_write::push_transactions_results>& results, const next_function<read_write::push_transactions_results>& next) {
   auto wrapped_next = [=](const fc::static_variant<fc::exception_ptr, read_write::push_transaction_results>& result) {
      if (result.contains<fc::exception_ptr>()) {
         const auto& e = result.get<fc::exception_ptr>();
         results->emplace_back( read_write::push_transaction_results{ transaction_id_type(), fc::mutable_variant_object( "error", e->to_detail_string() ) } );
      } else {
         const auto& r = result.get<read_write::push_transaction_results>();
         results->emplace_back( r );
      }

      int next_index = index + 1;
      if (next_index < params->size()) {
         push_recurse(rw, next_index, params, results, next );
      } else {
         next(*results);
      }
   };

   rw->push_transaction(params->at(index), wrapped_next);
}

void read_write::push_transactions(const read_write::push_transactions_params& params, next_function<read_write::push_transactions_results> next) {
   try {
      FC_ASSERT( params.size() <= 1000, "Attempt to push too many transactions at once" );
      auto params_copy = std::make_shared<read_write::push_transactions_params>(params.begin(), params.end());
      auto result = std::make_shared<read_write::push_transactions_results>();
      result->reserve(params.size());

      push_recurse(this, 0, params_copy, result, next);

   } CATCH_AND_CALL(next);
}

read_only::get_code_results read_only::get_code( const get_code_params& params )const {
   get_code_results result;
   result.account_name = params.account_name;
   const auto& d = db.db();
   const auto& accnt  = d.get<account_object,by_name>( params.account_name );

   if( accnt.code.size() ) {
      if (params.code_as_wasm) {
         result.wasm = string(accnt.code.begin(), accnt.code.end());
      } else {
         result.wast = wasm_to_wast( (const uint8_t*)accnt.code.data(), accnt.code.size() );
      }
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

static variant action_abi_to_variant( const abi_def& abi, type_name action_type ) {
   variant v;
   auto it = std::find_if(abi.structs.begin(), abi.structs.end(), [&](auto& x){return x.name == action_type;});
   if( it != abi.structs.end() )
      to_variant( it->fields,  v );
   return v;
};

read_only::abi_json_to_bin_result read_only::abi_json_to_bin( const read_only::abi_json_to_bin_params& params )const try {
   abi_json_to_bin_result result;
   const auto code_account = db.db().find<account_object,by_name>( params.code );
   EOS_ASSERT(code_account != nullptr, contract_query_exception, "Contract can't be found ${contract}", ("contract", params.code));

   abi_def abi;
   if( abi_serializer::to_abi(code_account->abi, abi) ) {
      abi_serializer abis( abi );
      auto action_type = abis.get_action_type(params.action);
      EOS_ASSERT(!action_type.empty(), action_validate_exception, "Unknown action ${action} in contract ${contract}", ("action", params.action)("contract", params.code));
      try {
         result.binargs = abis.variant_to_binary(action_type, params.args);
      } EOS_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
                                "'${args}' is invalid args for action '${action}' code '${code}'. expected '${proto}'",
                                ("args", params.args)("action", params.action)("code", params.code)("proto", action_abi_to_variant(abi, action_type)))
   } else {
      EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
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
   } else {
      EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
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
