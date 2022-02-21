#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain_plugin/blockvault_sync_strategy.hpp>
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/authorization_manager.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/reversible_block_object.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/generated_transaction_object.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/chain/combined_database.hpp>
#include <eosio/chain/backing_store/kv_context.hpp>
#include <eosio/to_key.hpp>

#include <eosio/chain/eosio_contract.hpp>
#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>

#include <chainbase/environment.hpp>

#include <boost/signals2/connection.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/lexical_cast.hpp>

#include <fc/io/json.hpp>
#include <fc/variant.hpp>
#include <fc/log/trace.hpp>
#include <signal.h>
#include <cstdlib>

// reflect chainbase::environment for --print-build-info option
FC_REFLECT_ENUM( chainbase::environment::os_t,
                 (OS_LINUX)(OS_MACOS)(OS_WINDOWS)(OS_OTHER) )
FC_REFLECT_ENUM( chainbase::environment::arch_t,
                 (ARCH_X86_64)(ARCH_ARM)(ARCH_RISCV)(ARCH_OTHER) )
FC_REFLECT(chainbase::environment, (debug)(os)(arch)(boost_version)(compiler) )

const fc::string deep_mind_logger_name("deep-mind");
fc::logger _deep_mind_log;

namespace eosio {

//declare operator<< and validate funciton for read_mode in the same namespace as read_mode itself
namespace chain {

std::ostream& operator<<(std::ostream& osm, eosio::chain::db_read_mode m) {
   if ( m == eosio::chain::db_read_mode::SPECULATIVE ) {
      osm << "speculative";
   } else if ( m == eosio::chain::db_read_mode::HEAD ) {
      osm << "head";
   } else if ( m == eosio::chain::db_read_mode::READ_ONLY ) { // deprecated
      osm << "read-only";
   } else if ( m == eosio::chain::db_read_mode::IRREVERSIBLE ) {
      osm << "irreversible";
   }

   return osm;
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              eosio::chain::db_read_mode* /* target_type */,
              int)
{
  using namespace boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  std::string const& s = validators::get_single_string(values);

  if ( s == "speculative" ) {
     v = boost::any(eosio::chain::db_read_mode::SPECULATIVE);
  } else if ( s == "head" ) {
     v = boost::any(eosio::chain::db_read_mode::HEAD);
  } else if ( s == "read-only" ) {
     v = boost::any(eosio::chain::db_read_mode::READ_ONLY);
  } else if ( s == "irreversible" ) {
     v = boost::any(eosio::chain::db_read_mode::IRREVERSIBLE);
  } else {
     throw validation_error(validation_error::invalid_option_value);
  }
}

std::ostream& operator<<(std::ostream& osm, eosio::chain::validation_mode m) {
   if ( m == eosio::chain::validation_mode::FULL ) {
      osm << "full";
   } else if ( m == eosio::chain::validation_mode::LIGHT ) {
      osm << "light";
   }

   return osm;
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              eosio::chain::validation_mode* /* target_type */,
              int)
{
  using namespace boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  std::string const& s = validators::get_single_string(values);

  if ( s == "full" ) {
     v = boost::any(eosio::chain::validation_mode::FULL);
  } else if ( s == "light" ) {
     v = boost::any(eosio::chain::validation_mode::LIGHT);
  } else {
     throw validation_error(validation_error::invalid_option_value);
  }
}

std::ostream& operator<<(std::ostream& osm, eosio::chain::backing_store_type b) {
   if ( b == eosio::chain::backing_store_type::CHAINBASE ) {
      osm << "chainbase";
   } else if ( b == eosio::chain::backing_store_type::ROCKSDB ) {
      osm << "rocksdb";
   }

   return osm;
}

void validate(boost::any& v,
              const std::vector<std::string>& values,
              eosio::chain::backing_store_type* /* target_type */,
              int)
{
  using namespace boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  std::string const& s = validators::get_single_string(values);

  if ( s == "chainbase" ) {
     v = boost::any(eosio::chain::backing_store_type::CHAINBASE);
  } else if ( s == "rocksdb" ) {
     v = boost::any(eosio::chain::backing_store_type::ROCKSDB);
  } else {
     throw validation_error(validation_error::invalid_option_value);
  }
}
}

using namespace eosio;
using namespace eosio::chain;
using namespace eosio::chain::config;
using namespace eosio::chain::plugin_interface;
using vm_type = wasm_interface::vm_type;
using fc::flat_map;

using boost::signals2::scoped_connection;

class chain_plugin_impl {
public:
   chain_plugin_impl()
   :pre_accepted_block_channel(app().get_channel<channels::pre_accepted_block>())
   ,accepted_block_header_channel(app().get_channel<channels::accepted_block_header>())
   ,accepted_block_channel(app().get_channel<channels::accepted_block>())
   ,irreversible_block_channel(app().get_channel<channels::irreversible_block>())
   ,accepted_transaction_channel(app().get_channel<channels::accepted_transaction>())
   ,applied_transaction_channel(app().get_channel<channels::applied_transaction>())
   ,incoming_block_channel(app().get_channel<incoming::channels::block>())
   ,incoming_block_sync_method(app().get_method<incoming::methods::block_sync>())
   ,incoming_blockvault_sync_method(app().get_method<incoming::methods::blockvault_sync>())
   ,incoming_transaction_async_method(app().get_method<incoming::methods::transaction_async>())
   {}

   bfs::path                        blocks_dir;
   bool                             readonly = false;
   flat_map<uint32_t,block_id_type> loaded_checkpoints;
   bool                             accept_transactions = false;
   bool                             api_accept_transactions = true;
   bool                             account_queries_enabled = false;

   std::optional<fork_database>      fork_db;
   std::optional<controller::config> chain_config;
   std::optional<controller>         chain;
   std::optional<genesis_state>      genesis;
   //txn_msg_rate_limits              rate_limits;
   std::optional<vm_type>            wasm_runtime;
   fc::microseconds                  abi_serializer_max_time_us;
   std::optional<bfs::path>          snapshot_path;


   // retained references to channels for easy publication
   channels::pre_accepted_block::channel_type&     pre_accepted_block_channel;
   channels::accepted_block_header::channel_type&  accepted_block_header_channel;
   channels::accepted_block::channel_type&         accepted_block_channel;
   channels::irreversible_block::channel_type&     irreversible_block_channel;
   channels::accepted_transaction::channel_type&   accepted_transaction_channel;
   channels::applied_transaction::channel_type&    applied_transaction_channel;
   incoming::channels::block::channel_type&        incoming_block_channel;

   // retained references to methods for easy calling
   incoming::methods::block_sync::method_type&        incoming_block_sync_method;
   incoming::methods::blockvault_sync::method_type&        incoming_blockvault_sync_method;
   incoming::methods::transaction_async::method_type& incoming_transaction_async_method;

   // method provider handles
   methods::get_block_by_number::method_type::handle                 get_block_by_number_provider;
   methods::get_block_by_id::method_type::handle                     get_block_by_id_provider;
   methods::get_head_block_id::method_type::handle                   get_head_block_id_provider;
   methods::get_last_irreversible_block_number::method_type::handle  get_last_irreversible_block_number_provider;

   // scoped connections for chain controller
   std::optional<scoped_connection>                                   pre_accepted_block_connection;
   std::optional<scoped_connection>                                   accepted_block_header_connection;
   std::optional<scoped_connection>                                   accepted_block_connection;
   std::optional<scoped_connection>                                   irreversible_block_connection;
   std::optional<scoped_connection>                                   accepted_transaction_connection;
   std::optional<scoped_connection>                                   applied_transaction_connection;

   std::optional<chain_apis::account_query_db>                        _account_query_db;

   void do_non_snapshot_startup(std::function<void()> shutdown, std::function<bool()> check_shutdown) {
       if (genesis) {
           chain->startup(shutdown, check_shutdown, *genesis);
       }else {
           chain->startup(shutdown, check_shutdown);
       }
   }
};

chain_plugin::chain_plugin()
   : my(new chain_plugin_impl()) {
   app().register_config_type<eosio::chain::db_read_mode>();
   app().register_config_type<eosio::chain::validation_mode>();
   app().register_config_type<eosio::chain::backing_store_type>();
   app().register_config_type<chainbase::pinnable_mapped_file::map_mode>();
   app().register_config_type<eosio::chain::wasm_interface::vm_type>();
}

chain_plugin::~chain_plugin(){}

void chain_plugin::set_program_options(options_description& cli, options_description& cfg)
{
   // build wasm_runtime help text
   std::string wasm_runtime_opt = "Override default WASM runtime (";
   std::string wasm_runtime_desc;
   std::string delim;
#ifdef EOSIO_EOS_VM_JIT_RUNTIME_ENABLED
   wasm_runtime_opt += " \"eos-vm-jit\"";
   wasm_runtime_desc += "\"eos-vm-jit\" : A WebAssembly runtime that compiles WebAssembly code to native x86 code prior to execution.\n";
   delim = ", ";
#endif

#ifdef EOSIO_EOS_VM_RUNTIME_ENABLED
   wasm_runtime_opt += delim + "\"eos-vm\"";
   wasm_runtime_desc += "\"eos-vm\" : A WebAssembly interpreter.\n";
   delim = ", ";
#endif

#ifdef EOSIO_EOS_VM_OC_DEVELOPER
   wasm_runtime_opt += delim + "\"eos-vm-oc\"";
   wasm_runtime_desc += "\"eos-vm-oc\" : Unsupported. Instead, use one of the other runtimes along with the option enable-eos-vm-oc.\n";
#endif
   wasm_runtime_opt += ")\n" + wasm_runtime_desc;

   std::string default_wasm_runtime_str= eosio::chain::wasm_interface::vm_type_string(eosio::chain::config::default_wasm_runtime);

   cfg.add_options()
         ("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"),
          "the location of the blocks directory (absolute path or relative to application data dir)")
         ("blocks-log-stride", bpo::value<uint32_t>()->default_value(config::default_blocks_log_stride),
         "split the block log file when the head block number is the multiple of the stride\n"
         "When the stride is reached, the current block log and index will be renamed '<blocks-retained-dir>/blocks-<start num>-<end num>.log/index'\n"
         "and a new current block log and index will be created with the most recent block. All files following\n"
         "this format will be used to construct an extended block log.")
         ("max-retained-block-files", bpo::value<uint16_t>()->default_value(config::default_max_retained_block_files),
          "the maximum number of blocks files to retain so that the blocks in those files can be queried.\n" 
          "When the number is reached, the oldest block file would be moved to archive dir or deleted if the archive dir is empty.\n"
          "The retained block log files should not be manipulated by users." )
         ("blocks-retained-dir", bpo::value<bfs::path>()->default_value(""),
          "the location of the blocks retained directory (absolute path or relative to blocks dir).\n"
          "If the value is empty, it is set to the value of blocks dir.")
         ("blocks-archive-dir", bpo::value<bfs::path>()->default_value(config::default_blocks_archive_dir_name),
          "the location of the blocks archive directory (absolute path or relative to blocks dir).\n"
          "If the value is empty, blocks files beyond the retained limit will be deleted.\n"
          "All files in the archive directory are completely under user's control, i.e. they won't be accessed by nodeos anymore.")
         ("fix-irreversible-blocks", bpo::value<bool>()->default_value("false"),
          "When the existing block log is inconsistent with the index, allows fixing the block log and index files automatically - that is, " 
          "it will take the highest indexed block if it is valid; otherwise it will repair the block log and reconstruct the index.")
         ("protocol-features-dir", bpo::value<bfs::path>()->default_value("protocol_features"),
          "the location of the protocol_features directory (absolute path or relative to application config dir)")
         ("checkpoint", bpo::value<vector<string>>()->composing(), "Pairs of [BLOCK_NUM,BLOCK_ID] that should be enforced as checkpoints.")
         ("wasm-runtime", bpo::value<eosio::chain::wasm_interface::vm_type>()->value_name("runtime")->notifier([](const auto& vm){
#ifndef EOSIO_EOS_VM_OC_DEVELOPER
            //throwing an exception here (like EOS_ASSERT) is just gobbled up with a "Failed to initialize" error :(
            if(vm == wasm_interface::vm_type::eos_vm_oc) {
               elog("EOS VM OC is a tier-up compiler and works in conjunction with the configured base WASM runtime. Enable EOS VM OC via 'eos-vm-oc-enable' option");
               EOS_ASSERT(false, plugin_exception, "");
            }
#endif
         })->default_value(eosio::chain::config::default_wasm_runtime, default_wasm_runtime_str), wasm_runtime_opt.c_str()
         )
         ("abi-serializer-max-time-ms", bpo::value<uint32_t>()->default_value(config::default_abi_serializer_max_time_us / 1000),
          "Override default maximum ABI serialization time allowed in ms")
         ("chain-state-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_state_size / (1024  * 1024)), "Maximum size (in MiB) of the chain state database")
         ("chain-state-db-guard-size-mb", bpo::value<uint64_t>()->default_value(config::default_state_guard_size / (1024  * 1024)), "Safely shut down node when free space remaining in the chain state database drops below this size (in MiB).")
         ("backing-store", boost::program_options::value<eosio::chain::backing_store_type>()->default_value(eosio::chain::backing_store_type::CHAINBASE),
          "The storage for state, chainbase or rocksdb")
         ("persistent-storage-num-threads", bpo::value<uint16_t>()->default_value(default_persistent_storage_num_threads),
          "Number of rocksdb threads for flush and compaction")
         ("persistent-storage-max-num-files", bpo::value<int>()->default_value(config::default_persistent_storage_max_num_files),
          "Max number of rocksdb files to keep open. -1 = unlimited.")
         ("persistent-storage-write-buffer-size-mb", bpo::value<uint64_t>()->default_value(config::default_persistent_storage_write_buffer_size / (1024  * 1024)),
          "Size of a single rocksdb memtable (in MiB)")
         ("persistent-storage-bytes-per-sync", bpo::value<uint64_t>()->default_value(config::default_persistent_storage_bytes_per_sync),
          "Rocksdb write rate of flushes and compactions.")
         ("persistent-storage-mbytes-snapshot-batch", bpo::value<uint32_t>()->default_value(config::default_persistent_storage_mbytes_batch),
          "Rocksdb batch size threshold before writing read in snapshot data to database.")

         ("reversible-blocks-db-size-mb", bpo::value<uint64_t>()->default_value(config::default_reversible_cache_size / (1024  * 1024)), "Maximum size (in MiB) of the reversible blocks database")
         ("reversible-blocks-db-guard-size-mb", bpo::value<uint64_t>()->default_value(config::default_reversible_guard_size / (1024  * 1024)), "Safely shut down node when free space remaining in the reverseible blocks database drops below this size (in MiB).")
         ("signature-cpu-billable-pct", bpo::value<uint32_t>()->default_value(config::default_sig_cpu_bill_pct / config::percent_1),
          "Percentage of actual signature recovery cpu to bill. Whole number percentages, e.g. 50 for 50%")
         ("chain-threads", bpo::value<uint16_t>()->default_value(config::default_controller_thread_pool_size),
          "Number of worker threads in controller thread pool")
         ("contracts-console", bpo::bool_switch()->default_value(false),
          "print contract's output to console")
         ("deep-mind", bpo::bool_switch()->default_value(false),
          "print deeper information about chain operations")
         ("telemetry-url", bpo::value<std::string>(),
          "Send Zipkin spans to url. e.g. http://127.0.0.1:9411/api/v2/spans" )
         ("telemetry-service-name", bpo::value<std::string>()->default_value("nodeos"),
          "Zipkin localEndpoint.serviceName sent with each span" )
         ("telemetry-timeout-us", bpo::value<uint32_t>()->default_value(200000),
          "Timeout for sending Zipkin span." )
         ("actor-whitelist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Account added to actor whitelist (may specify multiple times)")
         ("actor-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Account added to actor blacklist (may specify multiple times)")
         ("contract-whitelist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Contract account added to contract whitelist (may specify multiple times)")
         ("contract-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Contract account added to contract blacklist (may specify multiple times)")
         ("action-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Action (in the form code::action) added to action blacklist (may specify multiple times)")
         ("key-blacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Public key added to blacklist of keys that should not be included in authorities (may specify multiple times)")
         ("sender-bypass-whiteblacklist", boost::program_options::value<vector<string>>()->composing()->multitoken(),
          "Deferred transactions sent by accounts in this list do not have any of the subjective whitelist/blacklist checks applied to them (may specify multiple times)")
         ("read-mode", boost::program_options::value<eosio::chain::db_read_mode>()->default_value(eosio::chain::db_read_mode::SPECULATIVE),
          "Database read mode (\"speculative\", \"head\", \"read-only\", \"irreversible\").\n"
          "In \"speculative\" mode: database contains state changes by transactions in the blockchain up to the head block as well as some transactions not yet included in the blockchain.\n"
          "In \"head\" mode: database contains state changes by only transactions in the blockchain up to the head block; transactions received by the node are relayed if valid.\n"
          "In \"read-only\" mode: (DEPRECATED: see p2p-accept-transactions & api-accept-transactions) database contains state changes by only transactions in the blockchain up to the head block; transactions received via the P2P network are not relayed and transactions cannot be pushed via the chain API.\n"
          "In \"irreversible\" mode: database contains state changes by only transactions in the blockchain up to the last irreversible block; transactions received via the P2P network are not relayed and transactions cannot be pushed via the chain API.\n"
          )
         ( "api-accept-transactions", bpo::value<bool>()->default_value(true), "Allow API transactions to be evaluated and relayed if valid.")
         ("validation-mode", boost::program_options::value<eosio::chain::validation_mode>()->default_value(eosio::chain::validation_mode::FULL),
          "Chain validation mode (\"full\" or \"light\").\n"
          "In \"full\" mode all incoming blocks will be fully validated.\n"
          "In \"light\" mode all incoming blocks headers will be fully validated; transactions in those validated blocks will be trusted \n")
         ("disable-ram-billing-notify-checks", bpo::bool_switch()->default_value(false),
          "Disable the check which subjectively fails a transaction if a contract bills more RAM to another account within the context of a notification handler (i.e. when the receiver is not the code of the action).")
#ifdef EOSIO_DEVELOPER
         ("disable-all-subjective-mitigations", bpo::bool_switch()->default_value(false),
          "Disable all subjective mitigations checks in the entire codebase.")
#endif
         ("maximum-variable-signature-length", bpo::value<uint32_t>()->default_value(16384u),
          "Subjectively limit the maximum length of variable components in a variable legnth signature to this size in bytes")
         ("trusted-producer", bpo::value<vector<string>>()->composing(), "Indicate a producer whose blocks headers signed by it will be fully validated, but transactions in those validated blocks will be trusted.")
         ("database-map-mode", bpo::value<chainbase::pinnable_mapped_file::map_mode>()->default_value(chainbase::pinnable_mapped_file::map_mode::mapped),
          "Database map mode (\"mapped\", \"heap\", or \"locked\").\n"
          "In \"mapped\" mode database is memory mapped as a file.\n"
#ifndef _WIN32
          "In \"heap\" mode database is preloaded in to swappable memory and will use huge pages if available.\n"
          "In \"locked\" mode database is preloaded, locked in to memory, and will use huge pages if available.\n"
#endif
         )

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
         ("eos-vm-oc-cache-size-mb", bpo::value<uint64_t>()->default_value(eosvmoc::config().cache_size / (1024u*1024u)), "Maximum size (in MiB) of the EOS VM OC code cache")
         ("eos-vm-oc-compile-threads", bpo::value<uint64_t>()->default_value(1u)->notifier([](const auto t) {
               if(t == 0) {
                  elog("eos-vm-oc-compile-threads must be set to a non-zero value");
                  EOS_ASSERT(false, plugin_exception, "");
               }
         }), "Number of threads to use for EOS VM OC tier-up")
         ("eos-vm-oc-enable", bpo::bool_switch(), "Enable EOS VM OC tier-up runtime")
#endif
         ("enable-account-queries", bpo::value<bool>()->default_value(false), "enable queries to find accounts by various metadata.")
         ("max-nonprivileged-inline-action-size", bpo::value<uint32_t>()->default_value(config::default_max_nonprivileged_inline_action_size), "maximum allowed size (in bytes) of an inline action for a nonprivileged account")
         ;

// TODO: rate limiting
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
         ("print-build-info", bpo::bool_switch()->default_value(false),
          "print build environment information to console as JSON and exit")
         ("extract-build-info", bpo::value<bfs::path>(),
          "extract build environment information as JSON, write into specified file, and exit")
         ("fix-reversible-blocks", bpo::bool_switch()->default_value(false),
          "recovers reversible block database if that database is in a bad state")
         ("force-all-checks", bpo::bool_switch()->default_value(false),
          "do not skip any validation checks while replaying blocks (useful for replaying blocks from untrusted source)")
         ("disable-replay-opts", bpo::bool_switch()->default_value(false),
          "disable optimizations that specifically target replay")
         ("replay-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain state database and replay all blocks")
         ("hard-replay-blockchain", bpo::bool_switch()->default_value(false),
          "clear chain state database, recover as many blocks as possible from the block log, and then replay those blocks")
         ("delete-all-blocks", bpo::bool_switch()->default_value(false),
          "clear chain state database and block log")
         ("truncate-at-block", bpo::value<uint32_t>()->default_value(0),
          "stop hard replay / block log recovery at this block number (if set to non-zero number)")
         ("terminate-at-block", bpo::value<uint32_t>()->default_value(0),
          "terminate after reaching this block number (if set to a non-zero number)")
         ("import-reversible-blocks", bpo::value<bfs::path>(),
          "replace reversible block database with blocks imported from specified file and then exit")
         ("export-reversible-blocks", bpo::value<bfs::path>(),
           "export reversible block database in portable format into specified file and then exit")
         ("snapshot", bpo::value<bfs::path>(), "File to read Snapshot State from")
         ;

}

#define LOAD_VALUE_SET(options, op_name, container) \
if( options.count(op_name) ) { \
   const std::vector<std::string>& ops = options[op_name].as<std::vector<std::string>>(); \
   for( const auto& v : ops ) { \
      container.emplace( eosio::chain::name( v ) ); \
   } \
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

void clear_directory_contents( const fc::path& p ) {
   using boost::filesystem::directory_iterator;

   if( !fc::is_directory( p ) )
      return;

   for( directory_iterator enditr, itr{p}; itr != enditr; ++itr ) {
      fc::remove_all( itr->path() );
   }
}

std::optional<builtin_protocol_feature> read_builtin_protocol_feature( const fc::path& p  ) {
   try {
      return fc::json::from_file<builtin_protocol_feature>( p );
   } catch( const fc::exception& e ) {
      wlog( "problem encountered while reading '${path}':\n${details}",
            ("path", p.generic_string())("details",e.to_detail_string()) );
   } catch( ... ) {
      dlog( "unknown problem encountered while reading '${path}'",
            ("path", p.generic_string()) );
   }
   return {};
}

protocol_feature_set initialize_protocol_features( const fc::path& p, bool populate_missing_builtins = true ) {
   using boost::filesystem::directory_iterator;

   protocol_feature_set pfs;

   bool directory_exists = true;

   if( fc::exists( p ) ) {
      EOS_ASSERT( fc::is_directory( p ), plugin_exception,
                  "Path to protocol-features is not a directory: ${path}",
                  ("path", p.generic_string())
      );
   } else {
      if( populate_missing_builtins )
         bfs::create_directories( p );
      else
         directory_exists = false;
   }

   auto log_recognized_protocol_feature = []( const builtin_protocol_feature& f, const digest_type& feature_digest ) {
      if( f.subjective_restrictions.enabled ) {
         if( f.subjective_restrictions.preactivation_required ) {
            if( f.subjective_restrictions.earliest_allowed_activation_time == time_point{} ) {
               ilog( "Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled with preactivation required",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                     ("digest", feature_digest)
               );
            } else {
               ilog( "Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled with preactivation required and with an earliest allowed activation time of ${earliest_time}",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                     ("digest", feature_digest)
                     ("earliest_time", f.subjective_restrictions.earliest_allowed_activation_time)
               );
            }
         } else {
            if( f.subjective_restrictions.earliest_allowed_activation_time == time_point{} ) {
               ilog( "Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled without activation restrictions",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                     ("digest", feature_digest)
               );
            } else {
               ilog( "Support for builtin protocol feature '${codename}' (with digest of '${digest}') is enabled without preactivation required but with an earliest allowed activation time of ${earliest_time}",
                     ("codename", builtin_protocol_feature_codename(f.get_codename()))
                     ("digest", feature_digest)
                     ("earliest_time", f.subjective_restrictions.earliest_allowed_activation_time)
               );
            }
         }
      } else {
         ilog( "Recognized builtin protocol feature '${codename}' (with digest of '${digest}') but support for it is not enabled",
               ("codename", builtin_protocol_feature_codename(f.get_codename()))
               ("digest", feature_digest)
         );
      }
   };

   map<builtin_protocol_feature_t, fc::path>  found_builtin_protocol_features;
   map<digest_type, std::pair<builtin_protocol_feature, bool> > builtin_protocol_features_to_add;
   // The bool in the pair is set to true if the builtin protocol feature has already been visited to add
   map< builtin_protocol_feature_t, std::optional<digest_type> > visited_builtins;

   // Read all builtin protocol features
   if( directory_exists ) {
      for( directory_iterator enditr, itr{p}; itr != enditr; ++itr ) {
         auto file_path = itr->path();
         if( !fc::is_regular_file( file_path ) || file_path.extension().generic_string().compare( ".json" ) != 0 )
            continue;

         auto f = read_builtin_protocol_feature( file_path );

         if( !f ) continue;

         auto res = found_builtin_protocol_features.emplace( f->get_codename(), file_path );

         EOS_ASSERT( res.second, plugin_exception,
                     "Builtin protocol feature '${codename}' was already included from a previous_file",
                     ("codename", builtin_protocol_feature_codename(f->get_codename()))
                     ("current_file", file_path.generic_string())
                     ("previous_file", res.first->second.generic_string())
         );

         const auto feature_digest = f->digest();

         builtin_protocol_features_to_add.emplace( std::piecewise_construct,
                                                   std::forward_as_tuple( feature_digest ),
                                                   std::forward_as_tuple( *f, false ) );
      }
   }

   // Add builtin protocol features to the protocol feature manager in the right order (to satisfy dependencies)
   using itr_type = map<digest_type, std::pair<builtin_protocol_feature, bool>>::iterator;
   std::function<void(const itr_type&)> add_protocol_feature =
   [&pfs, &builtin_protocol_features_to_add, &visited_builtins, &log_recognized_protocol_feature, &add_protocol_feature]( const itr_type& itr ) -> void {
      if( itr->second.second ) {
         return;
      } else {
         itr->second.second = true;
         visited_builtins.emplace( itr->second.first.get_codename(), itr->first );
      }

      for( const auto& d : itr->second.first.dependencies ) {
         auto itr2 = builtin_protocol_features_to_add.find( d );
         if( itr2 != builtin_protocol_features_to_add.end() ) {
            add_protocol_feature( itr2 );
         }
      }

      pfs.add_feature( itr->second.first );

      log_recognized_protocol_feature( itr->second.first, itr->first );
   };

   for( auto itr = builtin_protocol_features_to_add.begin(); itr != builtin_protocol_features_to_add.end(); ++itr ) {
      add_protocol_feature( itr );
   }

   auto output_protocol_feature = [&p]( const builtin_protocol_feature& f, const digest_type& feature_digest ) {
      static constexpr int max_tries = 10;

      string filename( "BUILTIN-" );
      filename += builtin_protocol_feature_codename( f.get_codename() );
      filename += ".json";

      auto file_path = p / filename;

      EOS_ASSERT( !fc::exists( file_path ), plugin_exception,
                  "Could not save builtin protocol feature with codename '${codename}' because a file at the following path already exists: ${path}",
                  ("codename", builtin_protocol_feature_codename( f.get_codename() ))
                  ("path", file_path.generic_string())
      );

      if( fc::json::save_to_file( f, file_path ) ) {
         ilog( "Saved default specification for builtin protocol feature '${codename}' (with digest of '${digest}') to: ${path}",
               ("codename", builtin_protocol_feature_codename(f.get_codename()))
               ("digest", feature_digest)
               ("path", file_path.generic_string())
         );
      } else {
         elog( "Error occurred while writing default specification for builtin protocol feature '${codename}' (with digest of '${digest}') to: ${path}",
               ("codename", builtin_protocol_feature_codename(f.get_codename()))
               ("digest", feature_digest)
               ("path", file_path.generic_string())
         );
      }
   };

   std::function<digest_type(builtin_protocol_feature_t)> add_missing_builtins =
   [&pfs, &visited_builtins, &output_protocol_feature, &log_recognized_protocol_feature, &add_missing_builtins, populate_missing_builtins]
   ( builtin_protocol_feature_t codename ) -> digest_type {
      auto res = visited_builtins.emplace( codename, std::optional<digest_type>() );
      if( !res.second ) {
         EOS_ASSERT( res.first->second, protocol_feature_exception,
                     "invariant failure: cycle found in builtin protocol feature dependencies"
         );
         return *res.first->second;
      }

      auto f = protocol_feature_set::make_default_builtin_protocol_feature( codename,
      [&add_missing_builtins]( builtin_protocol_feature_t d ) {
         return add_missing_builtins( d );
      } );

      if( !populate_missing_builtins )
         f.subjective_restrictions.enabled = false;

      const auto& pf = pfs.add_feature( f );
      res.first->second = pf.feature_digest;

      log_recognized_protocol_feature( f, pf.feature_digest );

      if( populate_missing_builtins )
         output_protocol_feature( f, pf.feature_digest );

      return pf.feature_digest;
   };

   for( const auto& p : builtin_protocol_feature_codenames ) {
      auto itr = found_builtin_protocol_features.find( p.first );
      if( itr != found_builtin_protocol_features.end() ) continue;

      add_missing_builtins( p.first );
   }

   return pfs;
}

void
chain_plugin::do_hard_replay(const variables_map& options) {
         ilog( "Hard replay requested: deleting state database" );
         clear_directory_contents( my->chain_config->state_dir );
         auto backup_dir = block_log::repair_log( my->blocks_dir, options.at( "truncate-at-block" ).as<uint32_t>(),config::reversible_blocks_dir_name);
         if( fc::exists( backup_dir / config::reversible_blocks_dir_name ) ||
             options.at( "fix-reversible-blocks" ).as<bool>()) {
            // Do not try to recover reversible blocks if the directory does not exist, unless the option was explicitly provided.
            if( !recover_reversible_blocks( backup_dir / config::reversible_blocks_dir_name,
                                            my->chain_config->reversible_cache_size,
                                            my->chain_config->blog.log_dir / config::reversible_blocks_dir_name,
                                            options.at( "truncate-at-block" ).as<uint32_t>())) {
               ilog( "Reversible blocks database was not corrupted. Copying from backup to blocks directory." );
               fc::copy( backup_dir / config::reversible_blocks_dir_name,
                         my->chain_config->blog.log_dir / config::reversible_blocks_dir_name );
               fc::copy( backup_dir / config::reversible_blocks_dir_name / "shared_memory.bin",
                         my->chain_config->blog.log_dir / config::reversible_blocks_dir_name / "shared_memory.bin" );
            }
         }
}

void chain_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing chain plugin");

   try {
      try {
         genesis_state gs; // Check if EOSIO_ROOT_KEY is bad
      } catch ( const std::exception& ) {
         elog( "EOSIO_ROOT_KEY ('${root_key}') is invalid. Recompile with a valid public key.",
               ("root_key", genesis_state::eosio_root_key));
         throw;
      }

      my->chain_config = controller::config();

      if( options.at( "print-build-info" ).as<bool>() || options.count( "extract-build-info") ) {
         if( options.at( "print-build-info" ).as<bool>() ) {
            ilog( "Build environment JSON:\n${e}", ("e", json::to_pretty_string( chainbase::environment() )) );
         }
         if( options.count( "extract-build-info") ) {
            auto p = options.at( "extract-build-info" ).as<bfs::path>();

            if( p.is_relative()) {
               p = bfs::current_path() / p;
            }

            EOS_ASSERT( fc::json::save_to_file( chainbase::environment(), p, true ), misc_exception,
                        "Error occurred while writing build info JSON to '${path}'",
                        ("path", p.generic_string())
            );

            ilog( "Saved build info JSON to '${path}'", ("path", p.generic_string()) );
         }

         EOS_THROW( node_management_success, "reported build environment information" );
      }

      LOAD_VALUE_SET( options, "sender-bypass-whiteblacklist", my->chain_config->sender_bypass_whiteblacklist );
      LOAD_VALUE_SET( options, "actor-whitelist", my->chain_config->actor_whitelist );
      LOAD_VALUE_SET( options, "actor-blacklist", my->chain_config->actor_blacklist );
      LOAD_VALUE_SET( options, "contract-whitelist", my->chain_config->contract_whitelist );
      LOAD_VALUE_SET( options, "contract-blacklist", my->chain_config->contract_blacklist );

      LOAD_VALUE_SET( options, "trusted-producer", my->chain_config->trusted_producers );

      if( options.count( "action-blacklist" )) {
         const std::vector<std::string>& acts = options["action-blacklist"].as<std::vector<std::string>>();
         auto& list = my->chain_config->action_blacklist;
         for( const auto& a : acts ) {
            auto pos = a.find( "::" );
            EOS_ASSERT( pos != std::string::npos, plugin_config_exception, "Invalid entry in action-blacklist: '${a}'", ("a", a));
            account_name code( a.substr( 0, pos ));
            action_name act( a.substr( pos + 2 ));
            list.emplace( code, act );
         }
      }

      if( options.count( "key-blacklist" )) {
         const std::vector<std::string>& keys = options["key-blacklist"].as<std::vector<std::string>>();
         auto& list = my->chain_config->key_blacklist;
         for( const auto& key_str : keys ) {
            list.emplace( key_str );
         }
      }

      if( options.count( "blocks-dir" )) {
         auto bld = options.at( "blocks-dir" ).as<bfs::path>();
         if( bld.is_relative())
            my->blocks_dir = app().data_dir() / bld;
         else
            my->blocks_dir = bld;
      }

      protocol_feature_set pfs;
      {
         fc::path protocol_features_dir;
         auto pfd = options.at( "protocol-features-dir" ).as<bfs::path>();
         if( pfd.is_relative())
            protocol_features_dir = app().config_dir() / pfd;
         else
            protocol_features_dir = pfd;

         pfs = initialize_protocol_features( protocol_features_dir );
      }

      if( options.count("checkpoint") ) {
         auto cps = options.at("checkpoint").as<vector<string>>();
         my->loaded_checkpoints.reserve(cps.size());
         for( const auto& cp : cps ) {
            auto item = fc::json::from_string(cp).as<std::pair<uint32_t,block_id_type>>();
            auto itr = my->loaded_checkpoints.find(item.first);
            if( itr != my->loaded_checkpoints.end() ) {
               EOS_ASSERT( itr->second == item.second,
                           plugin_config_exception,
                          "redefining existing checkpoint at block number ${num}: original: ${orig} new: ${new}",
                          ("num", item.first)("orig", itr->second)("new", item.second)
               );
            } else {
               my->loaded_checkpoints[item.first] = item.second;
            }
         }
      }

      if( options.count( "wasm-runtime" ))
         my->wasm_runtime = options.at( "wasm-runtime" ).as<vm_type>();

      if(options.count("abi-serializer-max-time-ms")) {
         my->abi_serializer_max_time_us = fc::microseconds(options.at("abi-serializer-max-time-ms").as<uint32_t>() * 1000);
         my->chain_config->abi_serializer_max_time_us = my->abi_serializer_max_time_us;
      }

      my->chain_config->blog.log_dir                 = my->blocks_dir;
      my->chain_config->state_dir                    = app().data_dir() / config::default_state_dir_name;
      my->chain_config->read_only                    = my->readonly;
      my->chain_config->blog.retained_dir            = options.at("blocks-retained-dir").as<bfs::path>();
      my->chain_config->blog.archive_dir             = options.at("blocks-archive-dir").as<bfs::path>();
      my->chain_config->blog.stride                  = options.at("blocks-log-stride").as<uint32_t>();
      my->chain_config->blog.max_retained_files      = options.at("max-retained-block-files").as<uint16_t>();
      my->chain_config->blog.fix_irreversible_blocks = options.at("fix-irreversible-blocks").as<bool>();

      if (auto resmon_plugin = app().find_plugin<resource_monitor_plugin>()) {
        resmon_plugin->monitor_directory(my->chain_config->blog.log_dir);
        resmon_plugin->monitor_directory(my->chain_config->state_dir);
      }

      if( options.count( "chain-state-db-size-mb" ))
         my->chain_config->state_size = options.at( "chain-state-db-size-mb" ).as<uint64_t>() * 1024 * 1024;

      if( options.count( "chain-state-db-guard-size-mb" ))
         my->chain_config->state_guard_size = options.at( "chain-state-db-guard-size-mb" ).as<uint64_t>() * 1024 * 1024;

      my->chain_config->backing_store = options.at( "backing-store" ).as<backing_store_type>();

      if( options.count( "persistent-storage-num-threads" )) {
         my->chain_config->persistent_storage_num_threads = options.at( "persistent-storage-num-threads" ).as<uint16_t>();
         EOS_ASSERT( my->chain_config->persistent_storage_num_threads > 0, plugin_config_exception,
                     "persistent-storage-num-threads ${num} must be greater than 0", ("num", my->chain_config->persistent_storage_num_threads) );
      }

      if( options.count( "persistent-storage-max-num-files" )) {
         my->chain_config->persistent_storage_max_num_files = options.at( "persistent-storage-max-num-files" ).as<int>();
         EOS_ASSERT( my->chain_config->persistent_storage_max_num_files == -1 || my->chain_config->persistent_storage_max_num_files > 0, plugin_config_exception,
                     "persistent-storage-max-num-files ${num} must be equal to -1 or be greater than 0", ("num", my->chain_config->persistent_storage_max_num_files) );
      }

      if( options.count( "persistent-storage-write-buffer-size-mb" )) {
         my->chain_config->persistent_storage_write_buffer_size = options.at( "persistent-storage-write-buffer-size-mb" ).as<uint64_t>() * 1024 * 1024;
         EOS_ASSERT( my->chain_config->persistent_storage_write_buffer_size > 0, plugin_config_exception,
                     "persistent-storage-write-buffer-size-mb ${num} must be greater than 0", ("num", my->chain_config->persistent_storage_write_buffer_size) );
      }

      if( options.count( "persistent-storage-bytes-per-sync" )) {
         my->chain_config->persistent_storage_bytes_per_sync = options.at( "persistent-storage-bytes-per-sync" ).as<uint64_t>();
         EOS_ASSERT( my->chain_config->persistent_storage_bytes_per_sync > 0, plugin_config_exception,
                     "persistent-storage-bytes-per-sync ${num} must be greater than 0", ("num", my->chain_config->persistent_storage_bytes_per_sync) );
      }

      my->chain_config->persistent_storage_mbytes_batch = options.at( "persistent-storage-mbytes-snapshot-batch" ).as<uint32_t>();
      EOS_ASSERT( my->chain_config->persistent_storage_mbytes_batch > 0, plugin_config_exception,
                  "persistent-storage-mbytes-snapshot-batch ${num} must be greater than 0", ("num", my->chain_config->persistent_storage_mbytes_batch) );

      if( options.count( "reversible-blocks-db-size-mb" ))
         my->chain_config->reversible_cache_size =
               options.at( "reversible-blocks-db-size-mb" ).as<uint64_t>() * 1024 * 1024;

      if( options.count( "reversible-blocks-db-guard-size-mb" ))
         my->chain_config->reversible_guard_size = options.at( "reversible-blocks-db-guard-size-mb" ).as<uint64_t>() * 1024 * 1024;

      if( options.count( "max-nonprivileged-inline-action-size" ))
         my->chain_config->max_nonprivileged_inline_action_size = options.at( "max-nonprivileged-inline-action-size" ).as<uint32_t>();

      if( options.count( "chain-threads" )) {
         my->chain_config->thread_pool_size = options.at( "chain-threads" ).as<uint16_t>();
         EOS_ASSERT( my->chain_config->thread_pool_size > 0, plugin_config_exception,
                     "chain-threads ${num} must be greater than 0", ("num", my->chain_config->thread_pool_size) );
      }

      my->chain_config->sig_cpu_bill_pct = options.at("signature-cpu-billable-pct").as<uint32_t>();
      EOS_ASSERT( my->chain_config->sig_cpu_bill_pct >= 0 && my->chain_config->sig_cpu_bill_pct <= 100, plugin_config_exception,
                  "signature-cpu-billable-pct must be 0 - 100, ${pct}", ("pct", my->chain_config->sig_cpu_bill_pct) );
      my->chain_config->sig_cpu_bill_pct *= config::percent_1;

      if( my->wasm_runtime )
         my->chain_config->wasm_runtime = *my->wasm_runtime;

      my->chain_config->force_all_checks = options.at( "force-all-checks" ).as<bool>();
      my->chain_config->disable_replay_opts = options.at( "disable-replay-opts" ).as<bool>();
      my->chain_config->contracts_console = options.at( "contracts-console" ).as<bool>();
      my->chain_config->allow_ram_billing_in_notify = options.at( "disable-ram-billing-notify-checks" ).as<bool>();

#ifdef EOSIO_DEVELOPER
      my->chain_config->disable_all_subjective_mitigations = options.at( "disable-all-subjective-mitigations" ).as<bool>();
#endif

      my->chain_config->maximum_variable_signature_length = options.at( "maximum-variable-signature-length" ).as<uint32_t>();

      if( options.count( "terminate-at-block" ))
         my->chain_config->terminate_at_block = options.at( "terminate-at-block" ).as<uint32_t>();

      if( options.count( "extract-genesis-json" ) || options.at( "print-genesis-json" ).as<bool>()) {
         std::optional<genesis_state> gs;

         if( fc::exists( my->blocks_dir / "blocks.log" )) {
            gs = block_log::extract_genesis_state( my->blocks_dir );
            EOS_ASSERT( gs,
                        plugin_config_exception,
                        "Block log at '${path}' does not contain a genesis state, it only has the chain-id.",
                        ("path", (my->blocks_dir / "blocks.log").generic_string())
            );
         } else {
            wlog( "No blocks.log found at '${p}'. Using default genesis state.",
                  ("p", (my->blocks_dir / "blocks.log").generic_string()));
            gs.emplace();
         }

         if( options.at( "print-genesis-json" ).as<bool>()) {
            ilog( "Genesis JSON:\n${genesis}", ("genesis", json::to_pretty_string( *gs )));
         }

         if( options.count( "extract-genesis-json" )) {
            auto p = options.at( "extract-genesis-json" ).as<bfs::path>();

            if( p.is_relative()) {
               p = bfs::current_path() / p;
            }

            EOS_ASSERT( fc::json::save_to_file( *gs, p, true ),
                        misc_exception,
                        "Error occurred while writing genesis JSON to '${path}'",
                        ("path", p.generic_string())
            );

            ilog( "Saved genesis JSON to '${path}'", ("path", p.generic_string()) );
         }

         EOS_THROW( extract_genesis_state_exception, "extracted genesis state from blocks.log" );
      }

      if( options.count("export-reversible-blocks") ) {
         auto p = options.at( "export-reversible-blocks" ).as<bfs::path>();

         if( p.is_relative()) {
            p = bfs::current_path() / p;
         }

         if( export_reversible_blocks( my->chain_config->blog.log_dir/config::reversible_blocks_dir_name, p ) )
            ilog( "Saved all blocks from reversible block database into '${path}'", ("path", p.generic_string()) );
         else
            ilog( "Saved recovered blocks from reversible block database into '${path}'", ("path", p.generic_string()) );

         EOS_THROW( node_management_success, "exported reversible blocks" );
      }

      if( options.at( "delete-all-blocks" ).as<bool>()) {
         ilog( "Deleting state database and blocks" );
         if( options.at( "truncate-at-block" ).as<uint32_t>() > 0 )
            wlog( "The --truncate-at-block option does not make sense when deleting all blocks." );
         clear_directory_contents( my->chain_config->state_dir );
         clear_directory_contents( my->blocks_dir );
      } else if( options.at( "hard-replay-blockchain" ).as<bool>()) {
         do_hard_replay(options);
      } else if( options.at( "replay-blockchain" ).as<bool>()) {
         ilog( "Replay requested: deleting state database" );
         if( options.at( "truncate-at-block" ).as<uint32_t>() > 0 )
            wlog( "The --truncate-at-block option does not work for a regular replay of the blockchain." );
         eosio::chain::combined_database::destroy( my->chain_config->state_dir );
         if( options.at( "fix-reversible-blocks" ).as<bool>()) {
            if( !recover_reversible_blocks( my->chain_config->blog.log_dir / config::reversible_blocks_dir_name,
                                            my->chain_config->reversible_cache_size )) {
               ilog( "Reversible blocks database was not corrupted." );
            }
         }
      } else if( options.at( "fix-reversible-blocks" ).as<bool>()) {
         if( !recover_reversible_blocks( my->chain_config->blog.log_dir / config::reversible_blocks_dir_name,
                                         my->chain_config->reversible_cache_size,
                                         std::optional<fc::path>(),
                                         options.at( "truncate-at-block" ).as<uint32_t>())) {
            ilog( "Reversible blocks database verified to not be corrupted. Now exiting..." );
         } else {
            ilog( "Exiting after fixing reversible blocks database..." );
         }
         EOS_THROW( fixed_reversible_db_exception, "fixed corrupted reversible blocks database" );
      } else if( options.at( "truncate-at-block" ).as<uint32_t>() > 0 ) {
         wlog( "The --truncate-at-block option can only be used with --fix-reversible-blocks without a replay or with --hard-replay-blockchain." );
      } else if( options.count("import-reversible-blocks") ) {
         auto reversible_blocks_file = options.at("import-reversible-blocks").as<bfs::path>();
         ilog("Importing reversible blocks from '${file}'", ("file", reversible_blocks_file.generic_string()) );
         fc::remove_all( my->chain_config->blog.log_dir/config::reversible_blocks_dir_name );

         import_reversible_blocks( my->chain_config->blog.log_dir/config::reversible_blocks_dir_name,
                                   my->chain_config->reversible_cache_size, reversible_blocks_file );

         EOS_THROW( node_management_success, "imported reversible blocks" );
      }

      if( options.count("import-reversible-blocks") ) {
         wlog("The --import-reversible-blocks option should be used by itself.");
      }

      std::optional<chain_id_type> chain_id;
      if (options.count( "snapshot" )) {
         my->snapshot_path = options.at( "snapshot" ).as<bfs::path>();
         EOS_ASSERT( fc::exists(*my->snapshot_path), plugin_config_exception,
                     "Cannot load snapshot, ${name} does not exist", ("name", my->snapshot_path->generic_string()) );

         // recover genesis information from the snapshot
         // used for validation code below
         auto infile = std::ifstream(my->snapshot_path->generic_string(), (std::ios::in | std::ios::binary));
         istream_snapshot_reader reader(infile);
         reader.validate();
         chain_id = controller::extract_chain_id(reader);
         infile.close();

         EOS_ASSERT( options.count( "genesis-timestamp" ) == 0,
                 plugin_config_exception,
                 "--snapshot is incompatible with --genesis-timestamp as the snapshot contains genesis information");
         EOS_ASSERT( options.count( "genesis-json" ) == 0,
                     plugin_config_exception,
                     "--snapshot is incompatible with --genesis-json as the snapshot contains genesis information");

         auto shared_mem_path = my->chain_config->state_dir / "shared_memory.bin";
         EOS_ASSERT( !fc::is_regular_file(shared_mem_path),
                 plugin_config_exception,
                 "Snapshot can only be used to initialize an empty database." );

         if( fc::is_regular_file( my->blocks_dir / "blocks.log" )) {
            auto block_log_genesis = block_log::extract_genesis_state(my->blocks_dir);
            if( block_log_genesis ) {
               const auto& block_log_chain_id = block_log_genesis->compute_chain_id();
               EOS_ASSERT( *chain_id == block_log_chain_id,
                           plugin_config_exception,
                           "snapshot chain ID (${snapshot_chain_id}) does not match the chain ID from the genesis state in the block log (${block_log_chain_id})",
                           ("snapshot_chain_id",  *chain_id)
                           ("block_log_chain_id", block_log_chain_id)
               );
            } else {
               const auto& block_log_chain_id = block_log::extract_chain_id(my->blocks_dir);
               EOS_ASSERT( *chain_id == block_log_chain_id,
                           plugin_config_exception,
                           "snapshot chain ID (${snapshot_chain_id}) does not match the chain ID (${block_log_chain_id}) in the block log",
                           ("snapshot_chain_id",  *chain_id)
                           ("block_log_chain_id", block_log_chain_id)
               );
            }
         }

      } else {

         chain_id = controller::extract_chain_id_from_db( my->chain_config->state_dir );

         std::optional<genesis_state> block_log_genesis;
         std::optional<chain_id_type> block_log_chain_id;

         if( fc::is_regular_file( my->blocks_dir / "blocks.log" ) ) {
            block_log_genesis = block_log::extract_genesis_state( my->blocks_dir );
            if( block_log_genesis ) {
               block_log_chain_id = block_log_genesis->compute_chain_id();
            } else {
               block_log_chain_id = block_log::extract_chain_id( my->blocks_dir );
            }

            if( chain_id ) {
               EOS_ASSERT( *block_log_chain_id == *chain_id, block_log_exception,
                           "Chain ID in blocks.log (${block_log_chain_id}) does not match the existing "
                           " chain ID in state (${state_chain_id}).",
                           ("block_log_chain_id", *block_log_chain_id)
                           ("state_chain_id", *chain_id)
               );
            } else if( block_log_genesis ) {
               ilog( "Starting fresh blockchain state using genesis state extracted from blocks.log." );
               my->genesis = block_log_genesis;
               // Delay setting chain_id until later so that the code handling genesis-json below can know
               // that chain_id still only represents a chain ID extracted from the state (assuming it exists).
            }
         }

         if( options.count( "genesis-json" ) ) {
            bfs::path genesis_file = options.at( "genesis-json" ).as<bfs::path>();
            if( genesis_file.is_relative()) {
               genesis_file = bfs::current_path() / genesis_file;
            }

            EOS_ASSERT( fc::is_regular_file( genesis_file ),
                        plugin_config_exception,
                       "Specified genesis file '${genesis}' does not exist.",
                       ("genesis", genesis_file.generic_string()));

            genesis_state provided_genesis = fc::json::from_file( genesis_file ).as<genesis_state>();

            if( options.count( "genesis-timestamp" ) ) {
               provided_genesis.initial_timestamp = calculate_genesis_timestamp( options.at( "genesis-timestamp" ).as<string>() );

               ilog( "Using genesis state provided in '${genesis}' but with adjusted genesis timestamp",
                     ("genesis", genesis_file.generic_string()) );
            } else {
               ilog( "Using genesis state provided in '${genesis}'", ("genesis", genesis_file.generic_string()));
            }

            if( block_log_genesis ) {
               EOS_ASSERT( *block_log_genesis == provided_genesis, plugin_config_exception,
                           "Genesis state, provided via command line arguments, does not match the existing genesis state"
                           " in blocks.log. It is not necessary to provide genesis state arguments when a full blocks.log "
                           "file already exists."
               );
            } else {
               const auto& provided_genesis_chain_id = provided_genesis.compute_chain_id();
               if( chain_id ) {
                  EOS_ASSERT( provided_genesis_chain_id == *chain_id, plugin_config_exception,
                              "Genesis state, provided via command line arguments, has a chain ID (${provided_genesis_chain_id}) "
                              "that does not match the existing chain ID in the database state (${state_chain_id}). "
                              "It is not necessary to provide genesis state arguments when an initialized database state already exists.",
                              ("provided_genesis_chain_id", provided_genesis_chain_id)
                              ("state_chain_id", *chain_id)
                  );
               } else {
                  if( block_log_chain_id ) {
                     EOS_ASSERT( provided_genesis_chain_id == *block_log_chain_id, plugin_config_exception,
                                 "Genesis state, provided via command line arguments, has a chain ID (${provided_genesis_chain_id}) "
                                 "that does not match the existing chain ID in blocks.log (${block_log_chain_id}).",
                                 ("provided_genesis_chain_id", provided_genesis_chain_id)
                                 ("block_log_chain_id", *block_log_chain_id)
                     );
                  }

                  chain_id = provided_genesis_chain_id;

                  ilog( "Starting fresh blockchain state using provided genesis state." );
                  my->genesis = std::move(provided_genesis);
               }
            }
         } else {
            EOS_ASSERT( options.count( "genesis-timestamp" ) == 0,
                        plugin_config_exception,
                        "--genesis-timestamp is only valid if also passed in with --genesis-json");
         }

         if( !chain_id ) {
            if( my->genesis ) {
               // Uninitialized state database and genesis state extracted from block log
               chain_id = my->genesis->compute_chain_id();
            } else {
               // Uninitialized state database and no genesis state provided

               EOS_ASSERT( !block_log_chain_id, plugin_config_exception,
                           "Genesis state is necessary to initialize fresh blockchain state but genesis state could not be "
                           "found in the blocks log. Please either load from snapshot or find a blocks log that starts "
                           "from genesis."
               );

               ilog( "Starting fresh blockchain state using default genesis state." );
               my->genesis.emplace();
               chain_id = my->genesis->compute_chain_id();
            }
         }
      }

      if ( options.count("read-mode") ) {
         my->chain_config->read_mode = options.at("read-mode").as<db_read_mode>();
      }
      my->api_accept_transactions = options.at( "api-accept-transactions" ).as<bool>();

      if( my->chain_config->read_mode == db_read_mode::IRREVERSIBLE || my->chain_config->read_mode == db_read_mode::READ_ONLY ) {
         if( my->chain_config->read_mode == db_read_mode::READ_ONLY ) {
            wlog( "read-mode = read-only is deprecated use p2p-accept-transactions = false, api-accept-transactions = false instead." );
         }
         if( my->api_accept_transactions ) {
            my->api_accept_transactions = false;
            std::stringstream ss; ss << my->chain_config->read_mode;
            wlog( "api-accept-transactions set to false due to read-mode: ${m}", ("m", ss.str()) );
         }
      }
      if( my->api_accept_transactions ) {
         enable_accept_transactions();
      }

      if ( options.count("validation-mode") ) {
         my->chain_config->block_validation_mode = options.at("validation-mode").as<validation_mode>();
      }

      my->chain_config->db_map_mode = options.at("database-map-mode").as<pinnable_mapped_file::map_mode>();

#ifdef EOSIO_EOS_VM_OC_RUNTIME_ENABLED
      if( options.count("eos-vm-oc-cache-size-mb") )
         my->chain_config->eosvmoc_config.cache_size = options.at( "eos-vm-oc-cache-size-mb" ).as<uint64_t>() * 1024u * 1024u;
      if( options.count("eos-vm-oc-compile-threads") )
         my->chain_config->eosvmoc_config.threads = options.at("eos-vm-oc-compile-threads").as<uint64_t>();
      if( options["eos-vm-oc-enable"].as<bool>() )
         my->chain_config->eosvmoc_tierup = true;
#endif

      my->account_queries_enabled = options.at("enable-account-queries").as<bool>();

      my->chain.emplace( *my->chain_config, std::move(pfs), *chain_id );

      // initialize deep mind logging
      if ( options.at( "deep-mind" ).as<bool>() ) {
         // The actual `fc::dmlog_appender` implementation that is currently used by deep mind
         // logger is using `stdout` to prints it's log line out. Deep mind logging outputs
         // massive amount of data out of the process, which can lead under pressure to some
         // of the system calls (i.e. `fwrite`) to fail abruptly without fully writing the
         // entire line.
         //
         // Recovering from errors on a buffered (line or full) and continuing retrying write
         // is merely impossible to do right, because the buffer is actually held by the
         // underlying `libc` implementation nor the operation system.
         //
         // To ensure good functionalities of deep mind tracer, the `stdout` is made unbuffered
         // and the actual `fc::dmlog_appender` deals with retry when facing error, enabling a much
         // more robust deep mind output.
         //
         // Changing the standard `stdout` behavior from buffered to unbuffered can is disruptive
         // and can lead to weird scenarios in the logging process if `stdout` is used there too.
         //
         // In a future version, the `fc::dmlog_appender` implementation will switch to a `FIFO` file
         // approach, which will remove the dependency on `stdout` and hence this call.
         //
         // For the time being, when `deep-mind = true` is activated, we set `stdout` here to
         // be an unbuffered I/O stream.
         setbuf(stdout, NULL);

         my->chain->enable_deep_mind( &_deep_mind_log );
      }

      if (options.count("telemetry-url")) {
         fc::zipkin_config::init( options["telemetry-url"].as<std::string>(),
                                  options["telemetry-service-name"].as<std::string>(),
                                  options["telemetry-timeout-us"].as<uint32_t>() );
      }


      // set up method providers
      my->get_block_by_number_provider = app().get_method<methods::get_block_by_number>().register_provider(
            [this]( uint32_t block_num ) -> signed_block_ptr {
               return my->chain->fetch_block_by_number( block_num );
            } );

      my->get_block_by_id_provider = app().get_method<methods::get_block_by_id>().register_provider(
            [this]( block_id_type id ) -> signed_block_ptr {
               return my->chain->fetch_block_by_id( id );
            } );

      my->get_head_block_id_provider = app().get_method<methods::get_head_block_id>().register_provider( [this]() {
         return my->chain->head_block_id();
      } );

      my->get_last_irreversible_block_number_provider = app().get_method<methods::get_last_irreversible_block_number>().register_provider(
            [this]() {
               return my->chain->last_irreversible_block_num();
            } );

      // relay signals to channels
      my->pre_accepted_block_connection = my->chain->pre_accepted_block.connect([this](const signed_block_ptr& blk) {
         auto itr = my->loaded_checkpoints.find( blk->block_num() );
         if( itr != my->loaded_checkpoints.end() ) {
            auto id = blk->calculate_id();
            EOS_ASSERT( itr->second == id, checkpoint_exception,
                        "Checkpoint does not match for block number ${num}: expected: ${expected} actual: ${actual}",
                        ("num", blk->block_num())("expected", itr->second)("actual", id)
            );
         }

         my->pre_accepted_block_channel.publish(priority::medium, blk);
      });

      my->accepted_block_header_connection = my->chain->accepted_block_header.connect(
            [this]( const block_state_ptr& blk ) {
               my->accepted_block_header_channel.publish( priority::medium, blk );
            } );

      my->accepted_block_connection = my->chain->accepted_block.connect( [this]( const block_state_ptr& blk ) {
         if (auto dm_logger = my->chain->get_deep_mind_logger()) {
            auto packed_blk = fc::raw::pack(*blk);

            fc_dlog(*dm_logger, "ACCEPTED_BLOCK ${num} ${blk}",
               ("num", blk->block_num)
               ("blk", fc::to_hex(packed_blk))
            );
         }
         
          if (my->_account_query_db) {
            my->_account_query_db->commit_block(blk);
          }

         my->accepted_block_channel.publish( priority::high, blk );
      } );

      my->irreversible_block_connection = my->chain->irreversible_block.connect( [this]( const block_state_ptr& blk ) {
         my->irreversible_block_channel.publish( priority::low, blk );
      } );

      my->accepted_transaction_connection = my->chain->accepted_transaction.connect(
            [this]( const transaction_metadata_ptr& meta ) {
               my->accepted_transaction_channel.publish( priority::low, meta );
            } );

      my->applied_transaction_connection = my->chain->applied_transaction.connect(
            [this]( std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t ) {
               if (auto dm_logger = my->chain->get_deep_mind_logger()) {
                  auto packed_trace = fc::raw::pack(*std::get<0>(t));

                  fc_dlog(*dm_logger, "APPLIED_TRANSACTION ${block} ${traces}",
                     ("block", my->chain->head_block_num() + 1)
                     ("traces", fc::to_hex(packed_trace))
                  );
               }

               if (my->_account_query_db) {
                  my->_account_query_db->cache_transaction_trace(std::get<0>(t));
               }
               
               my->applied_transaction_channel.publish( priority::low, std::get<0>(t) );
            } );

      my->chain->add_indices();
   } FC_LOG_AND_RETHROW()

}

void chain_plugin::plugin_startup()
{ try {
   handle_sighup(); // Sets loggers

   EOS_ASSERT( my->chain_config->read_mode != db_read_mode::IRREVERSIBLE || !accept_transactions(), plugin_config_exception,
               "read-mode = irreversible. transactions should not be enabled by enable_accept_transactions" );
   try {
      auto shutdown = [](){ return app().quit(); };
      auto check_shutdown = [](){ return app().is_quiting(); };
      auto bvc_plug = app().find_plugin<blockvault_client_plugin>();
      auto blockvault_instance = bvc_plug ? bvc_plug->get() : nullptr;
      if (nullptr != blockvault_instance) {
          eosio::blockvault::blockvault_sync_strategy<chain_plugin_impl> bss(blockvault_instance, *my, shutdown, check_shutdown);
          bss.do_sync();
      } else if (my->snapshot_path) {
         auto infile = std::ifstream(my->snapshot_path->generic_string(), (std::ios::in | std::ios::binary));
         auto reader = std::make_shared<istream_snapshot_reader>(infile);
         my->chain->startup(shutdown, check_shutdown, reader);
         infile.close();
      } else {
         my->do_non_snapshot_startup(shutdown, check_shutdown);
      }
   } catch (const database_guard_exception& e) {
      log_guard_exception(e);
      // make sure to properly close the db
      my->chain.reset();
      throw;
   }

   if(!my->readonly) {
      ilog("starting chain in read/write mode");
   }

   if (my->genesis) {
      ilog("Blockchain started; head block is #${num}, genesis timestamp is ${ts}",
           ("num", my->chain->head_block_num())("ts", (std::string)my->genesis->initial_timestamp));
   }
   else {
      ilog("Blockchain started; head block is #${num}", ("num", my->chain->head_block_num()));
   }

   my->chain_config.reset();
  
   if (my->account_queries_enabled) {
      my->account_queries_enabled = false;
      try {
         my->_account_query_db.emplace(*my->chain);
         my->account_queries_enabled = true;
      } FC_LOG_AND_DROP(("Unable to enable account queries"));
   }



} FC_CAPTURE_AND_RETHROW() }

void chain_plugin::plugin_shutdown() {
   my->pre_accepted_block_connection.reset();
   my->accepted_block_header_connection.reset();
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   my->accepted_transaction_connection.reset();
   my->applied_transaction_connection.reset();
   if(app().is_quiting())
      my->chain->get_wasm_interface().indicate_shutting_down();
   my->chain.reset();
   zipkin_config::shutdown();
}

void chain_plugin::handle_sighup() {
   fc::logger::update( deep_mind_logger_name, _deep_mind_log );
}

chain_apis::read_write::read_write(controller& db, const fc::microseconds& abi_serializer_max_time, bool api_accept_transactions)
: db(db)
, abi_serializer_max_time(abi_serializer_max_time)
, api_accept_transactions(api_accept_transactions)
{
}

void chain_apis::read_write::validate() const {
   EOS_ASSERT( api_accept_transactions, missing_chain_api_plugin_exception,
               "Not allowed, node has api-accept-transactions = false" );
}

chain_apis::read_only chain_plugin::get_read_only_api() const {
   return chain_apis::read_only(chain(), my->_account_query_db, get_abi_serializer_max_time());
}

  
bool chain_plugin::accept_block(const signed_block_ptr& block, const block_id_type& id ) {
   return my->incoming_block_sync_method(block, id);
}

void chain_plugin::accept_transaction(const chain::packed_transaction_ptr& trx, next_function<chain::transaction_trace_ptr> next) {
   my->incoming_transaction_async_method(trx, false, std::move(next));
}

bool chain_plugin::recover_reversible_blocks( const fc::path& db_dir, uint32_t cache_size,
                                              std::optional<fc::path> new_db_dir, uint32_t truncate_at_block ) {
   try {
      chainbase::database reversible( db_dir, database::read_only); // Test if dirty
      // If it reaches here, then the reversible database is not dirty

      if( truncate_at_block == 0 )
         return false;

      reversible.add_index<reversible_block_index>();
      const auto& ubi = reversible.get_index<reversible_block_index,by_num>();

      auto itr = ubi.rbegin();
      if( itr != ubi.rend() && itr->blocknum <= truncate_at_block )
         return false; // Because we are not going to be truncating the reversible database at all.
   } catch( const std::runtime_error& ) {
   } catch( ... ) {
      throw;
   }
   // Reversible block database is dirty (or incompatible). So back it up (unless already moved) and then create a new one.

   auto reversible_dir = fc::canonical( db_dir );
   if( reversible_dir.filename().generic_string() == "." ) {
      reversible_dir = reversible_dir.parent_path();
   }
   fc::path backup_dir;

   auto now = fc::time_point::now();

   if( new_db_dir ) {
      backup_dir = reversible_dir;
      reversible_dir = *new_db_dir;
   } else {
      auto reversible_dir_name = reversible_dir.filename().generic_string();
      EOS_ASSERT( reversible_dir_name != ".", invalid_reversible_blocks_dir, "Invalid path to reversible directory" );
      backup_dir = reversible_dir.parent_path() / reversible_dir_name.append("-").append( now );

      EOS_ASSERT( !fc::exists(backup_dir),
                  reversible_blocks_backup_dir_exist,
                 "Cannot move existing reversible directory to already existing directory '${backup_dir}'",
                 ("backup_dir", backup_dir) );

      fc::rename( reversible_dir, backup_dir );
      ilog( "Moved existing reversible directory to backup location: '${new_db_dir}'", ("new_db_dir", backup_dir) );
   }

   fc::create_directories( reversible_dir );

   ilog( "Reconstructing '${reversible_dir}' from backed up reversible directory", ("reversible_dir", reversible_dir) );

   std::optional<chainbase::database> old_reversible;

   try {
      old_reversible = chainbase::database( backup_dir, database::read_only, 0, true );
   } catch (const std::runtime_error &) {
      // since we are allowing for dirty, it must be incompatible
      ilog( "Did not recover any reversible blocks since reversible database incompatible");
      return true;
   }

   chainbase::database  new_reversible( reversible_dir, database::read_write, cache_size );
   std::fstream         reversible_blocks;
   reversible_blocks.open( (reversible_dir.parent_path() / std::string("portable-reversible-blocks-").append( now ) ).generic_string().c_str(),
                           std::ios::out | std::ios::binary );

   uint32_t num = 0;
   uint32_t start = 0;
   uint32_t end = 0;
   old_reversible->add_index<reversible_block_index>();
   new_reversible.add_index<reversible_block_index>();
   const auto& ubi = old_reversible->get_index<reversible_block_index,by_num>();
   auto itr = ubi.begin();
   if( itr != ubi.end() ) {
      start = itr->blocknum;
      end = start - 1;
   }
   if( truncate_at_block > 0 && start > truncate_at_block ) {
      ilog( "Did not recover any reversible blocks since the specified block number to stop at (${stop}) is less than first block in the reversible database (${start}).", ("stop", truncate_at_block)("start", start) );
      return true;
   }
   try {
      for( ; itr != ubi.end(); ++itr ) {
         EOS_ASSERT( itr->blocknum == end + 1, gap_in_reversible_blocks_db,
                     "gap in reversible block database between ${end} and ${blocknum}",
                     ("end", end)("blocknum", itr->blocknum)
                   );
         reversible_blocks.write( itr->packedblock.data(), itr->packedblock.size() );
         new_reversible.create<reversible_block_object>( [&]( auto& ubo ) {
            ubo.blocknum = itr->blocknum;
            ubo.set_block( itr->get_block() ); // get_block and set_block rather than copying the packed data acts as additional validation
         });
         end = itr->blocknum;
         ++num;
         if( end == truncate_at_block )
            break;
      }
   } catch( const gap_in_reversible_blocks_db& e ) {
      wlog( "${details}", ("details", e.to_detail_string()) );
   } catch( ... ) {}

   if( end == truncate_at_block )
      ilog( "Stopped recovery of reversible blocks early at specified block number: ${stop}", ("stop", truncate_at_block) );

   if( num == 0 )
      ilog( "There were no recoverable blocks in the reversible block database" );
   else if( num == 1 )
      ilog( "Recovered 1 block from reversible block database: block ${start}", ("start", start) );
   else
      ilog( "Recovered ${num} blocks from reversible block database: blocks ${start} to ${end}",
            ("num", num)("start", start)("end", end) );

   return true;
}

bool chain_plugin::import_reversible_blocks( const fc::path& reversible_dir,
                                             uint32_t cache_size,
                                             const fc::path& reversible_blocks_file ) {
   std::fstream         reversible_blocks;
   chainbase::database  new_reversible( reversible_dir, database::read_write, cache_size );
   reversible_blocks.open( reversible_blocks_file.generic_string().c_str(), std::ios::in | std::ios::binary );

   reversible_blocks.seekg( 0, std::ios::end );
   auto end_pos = reversible_blocks.tellg();
   reversible_blocks.seekg( 0 );

   uint32_t num = 0;
   uint32_t start = 0;
   uint32_t end = 0;
   new_reversible.add_index<reversible_block_index>();
   try {
      while( reversible_blocks.tellg() < end_pos ) {
         signed_block_v0 tmp;
         fc::raw::unpack(reversible_blocks, tmp);
         num = tmp.block_num();

         if( start == 0 ) {
            start = num;
         } else {
            EOS_ASSERT( num == end + 1, gap_in_reversible_blocks_db,
                        "gap in reversible block database between ${end} and ${num}",
                        ("end", end)("num", num)
                      );
         }

         new_reversible.create<reversible_block_object>( [&]( auto& ubo ) {
            ubo.blocknum = num;
            ubo.set_block( std::make_shared<signed_block>(std::move(tmp), true) );
         });
         end = num;
      }
   } catch( gap_in_reversible_blocks_db& e ) {
      wlog( "${details}", ("details", e.to_detail_string()) );
      FC_RETHROW_EXCEPTION( e, warn, "rethrow" );
   } catch( ... ) {}

   ilog( "Imported blocks ${start} to ${end}", ("start", start)("end", end));

   if( num == 0 || end != num )
      return false;

   return true;
}

bool chain_plugin::export_reversible_blocks( const fc::path& reversible_dir,
                                             const fc::path& reversible_blocks_file ) {
   chainbase::database  reversible( reversible_dir, database::read_only, 0, true );
   std::fstream         reversible_blocks;
   reversible_blocks.open( reversible_blocks_file.generic_string().c_str(), std::ios::out | std::ios::binary );

   uint32_t num = 0;
   uint32_t start = 0;
   uint32_t end = 0;
   reversible.add_index<reversible_block_index>();
   const auto& ubi = reversible.get_index<reversible_block_index,by_num>();
   auto itr = ubi.begin();
   if( itr != ubi.end() ) {
      start = itr->blocknum;
      end = start - 1;
   }
   try {
      for( ; itr != ubi.end(); ++itr ) {
         EOS_ASSERT( itr->blocknum == end + 1, gap_in_reversible_blocks_db,
                     "gap in reversible block database between ${end} and ${blocknum}",
                     ("end", end)("blocknum", itr->blocknum)
                   );
         signed_block tmp;
         fc::datastream<const char *> ds( itr->packedblock.data(), itr->packedblock.size() );
         fc::raw::unpack(ds, tmp); // Verify that packed block has not been corrupted.
         const auto v0 = tmp.to_signed_block_v0(); // store in signed_block_v0 format
         auto packed_v0 = fc::raw::pack(*v0);
         reversible_blocks.write( packed_v0.data(), packed_v0.size() );
         end = itr->blocknum;
         ++num;
      }
   } catch( const gap_in_reversible_blocks_db& e ) {
      wlog( "${details}", ("details", e.to_detail_string()) );
   } catch( ... ) {}

   if( num == 0 ) {
      ilog( "There were no recoverable blocks in the reversible block database" );
      return false;
   }
   else if( num == 1 )
      ilog( "Exported 1 block from reversible block database: block ${start}", ("start", start) );
   else
      ilog( "Exported ${num} blocks from reversible block database: blocks ${start} to ${end}",
            ("num", num)("start", start)("end", end) );

   return (end >= start) && ((end - start + 1) == num);
}

controller& chain_plugin::chain() { return *my->chain; }
const controller& chain_plugin::chain() const { return *my->chain; }

chain::chain_id_type chain_plugin::get_chain_id()const {
   return my->chain->get_chain_id();
}

fc::microseconds chain_plugin::get_abi_serializer_max_time() const {
   return my->abi_serializer_max_time_us;
}

bool chain_plugin::api_accept_transactions() const{
   return my->api_accept_transactions;
}

bool chain_plugin::accept_transactions() const {
   return my->accept_transactions;
}

void chain_plugin::enable_accept_transactions() {
   my->accept_transactions = true;
}


void chain_plugin::log_guard_exception(const chain::guard_exception&e ) {
   if (e.code() == chain::database_guard_exception::code_value) {
      elog("Database has reached an unsafe level of usage, shutting down to avoid corrupting the database.  "
           "Please increase the value set for \"chain-state-db-size-mb\" and restart the process!");
   } else if (e.code() == chain::reversible_guard_exception::code_value) {
      elog("Reversible block database has reached an unsafe level of usage, shutting down to avoid corrupting the database.  "
           "Please increase the value set for \"reversible-blocks-db-size-mb\" and restart the process!");
   }

   dlog("Details: ${details}", ("details", e.to_detail_string()));
}

void chain_plugin::handle_guard_exception(const chain::guard_exception& e) {
   log_guard_exception(e);

   elog("database chain::guard_exception, quitting..."); // log string searched for in: tests/nodeos_under_min_avail_ram.py
   // quit the app
   app().quit();
}

void chain_plugin::handle_db_exhaustion() {
   elog("database memory exhausted: increase chain-state-db-size-mb and/or reversible-blocks-db-size-mb");
   //return 1 -- it's what programs/nodeos/main.cpp considers "BAD_ALLOC"
   std::_Exit(1);
}

void chain_plugin::handle_bad_alloc() {
   elog("std::bad_alloc - memory exhausted");
   //return -2 -- it's what programs/nodeos/main.cpp reports for std::exception
   std::_Exit(-2);
}
  
bool chain_plugin::account_queries_enabled() const {
   return my->account_queries_enabled;
}


namespace chain_apis {

const string read_only::KEYi64 = "i64";

template<typename I>
std::string itoh(I n, size_t hlen = sizeof(I)<<1) {
   static const char* digits = "0123456789abcdef";
   std::string r(hlen, '0');
   for(size_t i = 0, j = (hlen - 1) * 4 ; i < hlen; ++i, j -= 4)
      r[i] = digits[(n>>j) & 0x0f];
   return r;
}

read_only::get_info_results read_only::get_info(const read_only::get_info_params&) const {
   const auto& rm = db.get_resource_limits_manager();
   return {
      itoh(static_cast<uint32_t>(app().version())),
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
      rm.get_block_net_limit(),
      //std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string(),
      //__builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0,
      app().version_string(),
      db.fork_db_pending_head_block_num(),
      db.fork_db_pending_head_block_id(),
      app().full_version_string(),
      db.last_irreversible_block_time()
   };
}

read_only::get_activated_protocol_features_results
read_only::get_activated_protocol_features( const read_only::get_activated_protocol_features_params& params )const {
   read_only::get_activated_protocol_features_results result;
   const auto& pfm = db.get_protocol_feature_manager();

   uint32_t lower_bound_value = std::numeric_limits<uint32_t>::lowest();
   uint32_t upper_bound_value = std::numeric_limits<uint32_t>::max();

   if( params.lower_bound ) {
      lower_bound_value = *params.lower_bound;
   }

   if( params.upper_bound ) {
      upper_bound_value = *params.upper_bound;
   }

   if( upper_bound_value < lower_bound_value )
      return result;

   auto walk_range = [&]( auto itr, auto end_itr, auto&& convert_iterator ) {
      fc::mutable_variant_object mvo;
      mvo( "activation_ordinal", 0 );
      mvo( "activation_block_num", 0 );

      auto& activation_ordinal_value   = mvo["activation_ordinal"];
      auto& activation_block_num_value = mvo["activation_block_num"];

      auto cur_time = fc::time_point::now();
      auto end_time = cur_time + fc::microseconds(1000 * 10); /// 10ms max time
      for( unsigned int count = 0;
           cur_time <= end_time && count < params.limit && itr != end_itr;
           ++itr, cur_time = fc::time_point::now() )
      {
         const auto& conv_itr = convert_iterator( itr );
         activation_ordinal_value   = conv_itr.activation_ordinal();
         activation_block_num_value = conv_itr.activation_block_num();

         result.activated_protocol_features.emplace_back( conv_itr->to_variant( false, &mvo ) );
         ++count;
      }
      if( itr != end_itr ) {
         result.more = convert_iterator( itr ).activation_ordinal() ;
      }
   };

   auto get_next_if_not_end = [&pfm]( auto&& itr ) {
      if( itr == pfm.cend() ) return itr;

      ++itr;
      return itr;
   };

   auto lower = ( params.search_by_block_num ? pfm.lower_bound( lower_bound_value )
                                             : pfm.at_activation_ordinal( lower_bound_value ) );

   auto upper = ( params.search_by_block_num ? pfm.upper_bound( lower_bound_value )
                                             : get_next_if_not_end( pfm.at_activation_ordinal( upper_bound_value ) ) );

   if( params.reverse ) {
      walk_range( std::make_reverse_iterator(upper), std::make_reverse_iterator(lower),
                  []( auto&& ritr ) { return --(ritr.base()); } );
   } else {
      walk_range( lower, upper, []( auto&& itr ) { return itr; } );
   }

   return result;
}

uint64_t read_only::get_table_index_name(const read_only::get_table_rows_params& p, bool& primary) {
   using boost::algorithm::starts_with;
   // see multi_index packing of index name
   const uint64_t table = p.table.to_uint64_t();
   uint64_t index = table & 0xFFFFFFFFFFFFFFF0ULL;
   EOS_ASSERT( index == table, chain::contract_table_query_exception, "Unsupported table name: ${n}", ("n", p.table) );

   primary = false;
   uint64_t pos = 0;
   if (p.index_position.empty() || p.index_position == "first" || p.index_position == "primary" || p.index_position == "one") {
      primary = true;
   } else if (starts_with(p.index_position, "sec") || p.index_position == "two") { // second, secondary
   } else if (starts_with(p.index_position , "ter") || starts_with(p.index_position, "th")) { // tertiary, ternary, third, three
      pos = 1;
   } else if (starts_with(p.index_position, "fou")) { // four, fourth
      pos = 2;
   } else if (starts_with(p.index_position, "fi")) { // five, fifth
      pos = 3;
   } else if (starts_with(p.index_position, "six")) { // six, sixth
      pos = 4;
   } else if (starts_with(p.index_position, "sev")) { // seven, seventh
      pos = 5;
   } else if (starts_with(p.index_position, "eig")) { // eight, eighth
      pos = 6;
   } else if (starts_with(p.index_position, "nin")) { // nine, ninth
      pos = 7;
   } else if (starts_with(p.index_position, "ten")) { // ten, tenth
      pos = 8;
   } else {
      try {
         pos = fc::to_uint64( p.index_position );
      } catch(...) {
         EOS_ASSERT( false, chain::contract_table_query_exception, "Invalid index_position: ${p}", ("p", p.index_position));
      }
      if (pos < 2) {
         primary = true;
         pos = 0;
      } else {
         pos -= 2;
      }
   }
   index |= (pos & 0x000000000000000FULL);
   return index;
}

uint64_t convert_to_type(const eosio::name &n, const string &desc) {
   return n.to_uint64_t();
}

template<>
uint64_t convert_to_type(const string& str, const string& desc) {

   try {
      return boost::lexical_cast<uint64_t>(str.c_str(), str.size());
   } catch( ... ) { }

   try {
      auto trimmed_str = str;
      boost::trim(trimmed_str);
      name s(trimmed_str);
      return s.to_uint64_t();
   } catch( ... ) { }

   if (str.find(',') != string::npos) { // fix #6274 only match formats like 4,EOS
      try {
         auto symb = eosio::chain::symbol::from_string(str);
         return symb.value();
      } catch( ... ) { }
   }

   try {
      return ( eosio::chain::string_to_symbol( 0, str.c_str() ) >> 8 );
   } catch( ... ) {
      EOS_ASSERT( false, chain_type_exception, "Could not convert ${desc} string '${str}' to any of the following: "
                        "uint64_t, valid name, or valid symbol (with or without the precision)",
                  ("desc", desc)("str", str));
   }
}

template<>
double convert_to_type(const string& str, const string& desc) {
   double val{};
   try {
      val = fc::variant(str).as<double>();
   } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} string '${str}' to key type.", ("desc", desc)("str",str) )

   EOS_ASSERT( !std::isnan(val), chain::contract_table_query_exception,
               "Converted ${desc} string '${str}' to NaN which is not a permitted value for the key type", ("desc", desc)("str",str) );

   return val;
}

template<typename Type>
string convert_to_string(const Type& source, const string& key_type, const string& encode_type, const string& desc) {
   try {
      return fc::variant(source).as<string>();
   } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} from '${source}' to string.", ("desc", desc)("source",source) )
}

template<>
string convert_to_string(const chain::key256_t& source, const string& key_type, const string& encode_type, const string& desc) {
   try {
      if (key_type == chain_apis::sha256 || (key_type == chain_apis::i256 && encode_type == chain_apis::hex)) {
         auto byte_array = fixed_bytes<32>(source).extract_as_byte_array();
         fc::sha256 val(reinterpret_cast<char *>(byte_array.data()), byte_array.size());
         return std::string(val);
      } else if (key_type == chain_apis::i256) {
         auto byte_array = fixed_bytes<32>(source).extract_as_byte_array();
         fc::sha256 val(reinterpret_cast<char *>(byte_array.data()), byte_array.size());
         return std::string("0x") + std::string(val);
      } else if (key_type == chain_apis::ripemd160) {
         auto byte_array = fixed_bytes<20>(source).extract_as_byte_array();
         fc::ripemd160 val;
         memcpy(val._hash, byte_array.data(), byte_array.size() );
         return std::string(val);
      }
      EOS_ASSERT( false, chain_type_exception, "Incompatible key_type and encode_type for key256_t next_key" );

   } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} source '${source}' to string.", ("desc", desc)("source",source) )
}

template<>
string convert_to_string(const float128_t& source, const string& key_type, const string& encode_type, const string& desc) {
   try {
      float64_t f = f128_to_f64(source);
      return fc::variant(f).as<string>();
   } FC_RETHROW_EXCEPTIONS(warn, "Could not convert ${desc} from '${source}' to string.", ("desc", desc)("source",source) )
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
   const abi_def abi = eosio::chain_apis::get_abi( db, p.code );
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
   bool primary = false;
   auto table_with_index = get_table_index_name( p, primary );
   if( primary ) {
      EOS_ASSERT( p.table == table_with_index, chain::contract_table_query_exception, "Invalid table name ${t}", ( "t", p.table ));
      auto table_type = get_table_type( abi, p.table );
      if( table_type == KEYi64 || p.key_type == "i64" || p.key_type == "name" ) {
         return get_table_rows_ex<key_value_index>(p,abi);
      }
      EOS_ASSERT( false, chain::contract_table_query_exception,  "Invalid table type ${type}", ("type",table_type)("abi",abi));
   } else {
      EOS_ASSERT( !p.key_type.empty(), chain::contract_table_query_exception, "key type required for non-primary index" );

      if (p.key_type == chain_apis::i64 || p.key_type == "name") {
         return get_table_rows_by_seckey<index64_index, uint64_t>(p, abi, [](uint64_t v)->uint64_t {
            return v;
         });
      }
      else if (p.key_type == chain_apis::i128) {
         return get_table_rows_by_seckey<index128_index, uint128_t>(p, abi, [](uint128_t v)->uint128_t {
            return v;
         });
      }
      else if (p.key_type == chain_apis::i256) {
         if ( p.encode_type == chain_apis::hex) {
            using  conv = keytype_converter<chain_apis::sha256,chain_apis::hex>;
            return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
         }
         using  conv = keytype_converter<chain_apis::i256>;
         return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
      }
      else if (p.key_type == chain_apis::float64) {
         return get_table_rows_by_seckey<index_double_index, double>(p, abi, [](double v)->float64_t {
            float64_t f = *(float64_t *)&v;
            return f;
         });
      }
      else if (p.key_type == chain_apis::float128) {
         if ( p.encode_type == chain_apis::hex) {
            return get_table_rows_by_seckey<index_long_double_index, uint128_t>(p, abi, [](uint128_t v)->float128_t{
               return *reinterpret_cast<float128_t *>(&v);
            });
         }
         return get_table_rows_by_seckey<index_long_double_index, double>(p, abi, [](double v)->float128_t{
            float64_t f = *(float64_t *)&v;
            float128_t f128;
            f64_to_f128M(f, &f128);
            return f128;
         });
      }
      else if (p.key_type == chain_apis::sha256) {
         using  conv = keytype_converter<chain_apis::sha256,chain_apis::hex>;
         return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
      }
      else if(p.key_type == chain_apis::ripemd160) {
         using  conv = keytype_converter<chain_apis::ripemd160,chain_apis::hex>;
         return get_table_rows_by_seckey<conv::index_type, conv::input_type>(p, abi, conv::function());
      }
      EOS_ASSERT(false, chain::contract_table_query_exception,  "Unsupported secondary index type: ${t}", ("t", p.key_type));
   }
#pragma GCC diagnostic pop
}

/// short_string is intended to optimize the string equality comparison where one of the operand is
/// no greater than 8 bytes long.
struct short_string {
   uint64_t data = 0;

   template <size_t SIZE>
   short_string(const char (&str)[SIZE]) {
      static_assert(SIZE <= 8, "data has to be 8 bytes or less");
      memcpy(&data, str, SIZE);
   }

   short_string(std::string str) { memcpy(&data, str.c_str(), std::min(sizeof(data), str.size())); }

   bool empty() const { return data == 0; }

   friend bool operator==(short_string lhs, short_string rhs) { return lhs.data == rhs.data; }
   friend bool operator!=(short_string lhs, short_string rhs) { return lhs.data != rhs.data; }
};

template <typename Type, typename Enable = void>
struct key_converter;

inline void key_convert_assert(bool condition) {
   // EOS_ASSERT is avoided intentionally here because EOS_ASSERT would create the fc::log_message object which is
   // relatively expensive. The throw statement here is only used for flow control purpose, not for error reporting
   // purpose. 
   if (!condition)
      throw std::invalid_argument("");
}

// convert unsigned integer in hex representation back to its integer representation
template <typename UnsignedInt>
UnsignedInt unhex(const std::string& bytes_in_hex) {
   assert(bytes_in_hex.size() == 2 * sizeof(UnsignedInt));
   std::array<char, sizeof(UnsignedInt)> bytes;
   boost::algorithm::unhex(bytes_in_hex.begin(), bytes_in_hex.end(), bytes.rbegin());
   UnsignedInt result;
   memcpy(&result, bytes.data(), sizeof(result));
   return result;
}

template <typename IntType>
struct key_converter<IntType, std::enable_if_t<std::is_integral_v<IntType>>> {
   static void to_bytes(const string& str, short_string encode_type, fixed_buf_stream& strm) {
      int base = 10;
      if (encode_type == "hex")
         base = 16;
      else
         key_convert_assert(encode_type.empty() || encode_type == "dec");

      size_t pos = 0;
      if constexpr (std::is_unsigned_v<IntType>) {
         uint64_t value = std::stoul(str, &pos, base);
         key_convert_assert(pos > 0 && value <= std::numeric_limits<IntType>::max());
         to_key(static_cast<IntType>(value), strm);
      } else {
         int64_t value = std::stol(str, &pos, base);
         key_convert_assert(pos > 0 && value <= std::numeric_limits<IntType>::max() &&
                            value >= std::numeric_limits<IntType>::min());
         to_key(static_cast<IntType>(value), strm);
      }
   }

   static IntType value_from_hex(const std::string& bytes_in_hex) {
      auto unsigned_val = unhex<std::make_unsigned_t<IntType>>(bytes_in_hex);
      if (unsigned_val > std::numeric_limits<IntType>::max()) {
         return unsigned_val + static_cast<std::make_unsigned_t<IntType>>(std::numeric_limits<IntType>::min());
      } else {
         return unsigned_val + std::numeric_limits<IntType>::min();
      }
   }

   static std::string from_hex(const std::string& bytes_in_hex, short_string encode_type) {
      IntType val = value_from_hex(bytes_in_hex);
      if (encode_type.empty() || encode_type == "dec") {
         return std::to_string(val);
      } else if (encode_type == "hex") {
         std::array<char, sizeof(IntType)> v;
         memcpy(v.data(), &val, sizeof(val));
         char result[2 * sizeof(IntType) + 1] = {'\0'};
         boost::algorithm::hex(v.rbegin(), v.rend(), result);
         return std::find_if_not(result, result + 2 * sizeof(IntType), [](char v) { return v == '0'; });
      }
      throw std::invalid_argument("");
   }
};

template <typename Float>
struct key_converter<Float, std::enable_if_t<std::is_floating_point_v<Float>>> {
   static void to_bytes(const string& str, short_string encode_type, fixed_buf_stream& strm) {
      key_convert_assert(encode_type.empty() || encode_type == "dec");
      if constexpr (sizeof(Float) == 4) {
         to_key(std::stof(str), strm);
      } else {
         to_key(std::stod(str), strm);
      }
   }

   static Float value_from_hex(const std::string& bytes_in_hex) {
      using UInt = std::conditional_t<sizeof(Float) == 4, uint32_t, uint64_t>;
      UInt val   = unhex<UInt>(bytes_in_hex);

      UInt mask    = 0;
      UInt signbit = (static_cast<UInt>(1) << (std::numeric_limits<UInt>::digits - 1));
      if (!(val & signbit)) // flip mask if val is positive
         mask = ~mask;
      val ^= (mask | signbit);
      Float result;
      memcpy(&result, &val, sizeof(val));
      return result;
   }

   static std::string from_hex(const std::string& bytes_in_hex, short_string encode_type) {
      return std::to_string(value_from_hex(bytes_in_hex));
   }
};

template <>
struct key_converter<checksum256_type, void> {
   static void to_bytes(const string& str, short_string encode_type, fixed_buf_stream& strm) {
      key_convert_assert(encode_type.empty() || encode_type == "hex");
      checksum256_type sha{str};
      strm.write(sha.data(), sha.data_size());
   }
   static std::string from_hex(const std::string& bytes_in_hex, short_string encode_type) { return bytes_in_hex; }
};

template <>
struct key_converter<name, void> {
   static void to_bytes(const string& str, short_string encode_type, fixed_buf_stream& strm) {
      key_convert_assert(encode_type.empty() || encode_type == "name");
      to_key(name(str).to_uint64_t(), strm);
   }

   static std::string from_hex(const std::string& bytes_in_hex, short_string encode_type) {
      return name(key_converter<uint64_t>::value_from_hex(bytes_in_hex)).to_string();
   }
};

template <>
struct key_converter<std::string, void> {
   static void to_bytes(const string& str, short_string encode_type, fixed_buf_stream& strm) {
      key_convert_assert(encode_type.empty() || encode_type == "string");
      to_key(str, strm);
   }

   static std::string from_hex(const std::string& bytes_in_hex, short_string encode_type) {
      std::string result = boost::algorithm::unhex(bytes_in_hex);
      /// restore the string following the encoding rule from `template <typename S> to_key(std::string, S&)` in abieos
      /// to_key.hpp
      boost::replace_all(result, "\0\1", "\0");
      // remove trailing '\0\0'
      auto sz = result.size();
      if (sz >= 2 && result[sz - 1] == '\0' && result[sz - 2] == '\0')
         result.resize(sz - 2);
      return result;
   }
};

namespace key_helper {
/// Caution: the order of `key_type` and `key_type_ids` should match exactly.
using key_types = std::tuple<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, float, double,
                             name, checksum256_type, checksum256_type, std::string>;
static const short_string key_type_ids[] = {"int8",   "int16",   "int32",   "int64", "uint8",  "uint16", "uint32",
                                            "uint64", "float32", "float64", "name",  "sha256", "i256",   "string"};

static_assert(sizeof(key_type_ids) / sizeof(short_string) == std::tuple_size<key_types>::value,
              "key_type_ids and key_types must be of the same size and the order of their elements has to match");

uint64_t type_string_to_function_index(short_string name) {
   unsigned index = std::find(std::begin(key_type_ids), std::end(key_type_ids), name) - std::begin(key_type_ids);
   key_convert_assert(index < std::tuple_size<key_types>::value);
   return index;
}

void write_key(string index_type, string encode_type, const string& index_value, fixed_buf_stream& strm) {
   try {
      // converts arbitrary hex strings to bytes ex) "FFFEFD" to {255, 254, 253}
      if (encode_type == "bytes") {
         strm.pos = boost::algorithm::unhex(index_value.begin(), index_value.end(), strm.pos);
         return;
      }

      if (index_type == "ripemd160") {
         key_convert_assert(encode_type.empty() || encode_type == "hex");
         checksum160_type ripem160{index_value};
         strm.write(ripem160.data(), ripem160.data_size());
         return;
      }

      std::apply(
          [index_type, &index_value, encode_type, &strm](auto... t) {
             using to_byte_fun_t         = void (*)(const string&, short_string, fixed_buf_stream&);
             static to_byte_fun_t funs[] = {&key_converter<decltype(t)>::to_bytes...};
             auto                 index  = type_string_to_function_index(index_type);
             funs[index](index_value, encode_type, strm);
          },
          key_types{});
   } catch (...) {
      FC_THROW_EXCEPTION(chain::contract_table_query_exception,
                         "Incompatible index type/encode_type/Index_value: ${t}/${e}/{$v} ",
                         ("t", index_type)("e", encode_type)("v", index_value));
   }
}

std::string read_key(string index_type, string encode_type, const string& bytes_in_hex) {
   try {
      if (encode_type == "bytes" || index_type == "ripemd160")
         return bytes_in_hex;

      return std::apply(
          [index_type, bytes_in_hex, &encode_type](auto... t) {
             using from_hex_fun_t         = std::string (*)(const string&, short_string);
             static from_hex_fun_t funs[] = {&key_converter<decltype(t)>::from_hex...};
             auto                    index  = type_string_to_function_index(index_type);
             return funs[index](bytes_in_hex, encode_type);
          },
          key_types{});
   } catch (...) {
      FC_THROW_EXCEPTION(chain::contract_table_query_exception, "Unsupported index type/encode_type: ${t}/${e} ",
                         ("t", index_type)("e", encode_type));
   }
}
} // namespace key_helper

constexpr uint32_t prefix_size = 17; // prefix 17bytes: status(1 byte) + table_name(8bytes) + index_name(8 bytes)
struct kv_table_rows_context {
   std::unique_ptr<eosio::chain::kv_context>  kv_context;
   const read_only::get_kv_table_rows_params& p;
   abi_serializer::yield_function_t           yield_function;                            
   abi_def                                    abi;
   abi_serializer                             abis;
   std::string                                index_type;
   bool                                       shorten_abi_errors;
   bool                                       is_primary_idx;

   kv_table_rows_context(const controller& db, const read_only::get_kv_table_rows_params& param,
                         const fc::microseconds abi_serializer_max_time, bool shorten_error)
       : kv_context(db.kv_db().create_kv_context(
             param.code, {},
             db.get_global_properties().kv_configuration)) // To do: provide kv_resource_manmager to create_kv_context
       , p(param)
       , yield_function(abi_serializer::create_yield_function(abi_serializer_max_time))
       , abi(eosio::chain_apis::get_abi(db, param.code))
       , shorten_abi_errors(shorten_error) {

      EOS_ASSERT(p.limit > 0, chain::contract_table_query_exception, "invalid limit : ${n}", ("n", p.limit));
      string tbl_name = p.table.to_string();

      // Check valid table name
      const auto table_it = abi.kv_tables.value.find(p.table);
      if (table_it == abi.kv_tables.value.end()) {
         EOS_ASSERT(false, chain::contract_table_query_exception, "Unknown kv_table: ${t}", ("t", tbl_name));
      }
      const auto& kv_tbl_def = table_it->second;
      // Check valid index_name
      is_primary_idx  = (p.index_name == kv_tbl_def.primary_index.name);
      bool is_sec_idx = (kv_tbl_def.secondary_indices.find(p.index_name) != kv_tbl_def.secondary_indices.end());
      EOS_ASSERT(is_primary_idx || is_sec_idx, chain::contract_table_query_exception, "Unknown kv index: ${t} ${i}",
                 ("t", p.table)("i", p.index_name));

      index_type = kv_tbl_def.get_index_type(p.index_name.to_string());
      abis.set_abi(abi, yield_function);
   }

   bool point_query() const { return p.index_value.size(); }

   void write_prefix(fixed_buf_stream& strm) const {
      strm.write('\1');
      to_key(p.table.to_uint64_t(), strm);
      to_key(p.index_name.to_uint64_t(), strm);
   }

   std::vector<char> get_full_key(string key) const {
      // the max possible encoded_key_byte_count occurs when the encoded type is string and when all characters 
      // in the string is '\0'
      const size_t max_encoded_key_byte_count = std::max(sizeof(uint64_t), 2 * key.size() + 1);
      std::vector<char> full_key(prefix_size + max_encoded_key_byte_count);
      fixed_buf_stream  strm(full_key.data(), full_key.size());
      write_prefix(strm);
      if (key.size())
         key_helper::write_key(index_type, p.encode_type, key, strm);
      full_key.resize(strm.pos - full_key.data());
      return full_key;
   }
};

struct kv_iterator_ex {
   uint32_t                     key_size   = 0;
   uint32_t                     value_size = 0;
   const kv_table_rows_context& context;
   std::unique_ptr<kv_iterator> base;
   kv_it_stat                   status;

   kv_iterator_ex(const kv_table_rows_context& ctx, const std::vector<char>& full_key)
       : context(ctx) {
      base   = context.kv_context->kv_it_create(context.p.code.to_uint64_t(), full_key.data(), prefix_size);
      status = base->kv_it_lower_bound(full_key.data(), full_key.size(), &key_size, &value_size);
      EOS_ASSERT(status != chain::kv_it_stat::iterator_erased, chain::contract_table_query_exception,
                 "Invalid iterator in ${t} ${i}", ("t", context.p.table)("i", context.p.index_name));
   }

   bool is_end() const { return status == kv_it_stat::iterator_end; }

   /// @pre ! is_end()
   std::vector<char> get_key() const {
      std::vector<char> result(key_size);
      uint32_t          actual_size;
      base->kv_it_key(0, result.data(), key_size, actual_size);
      return result;
   }

   /// @pre ! is_end()
   std::vector<char> get_value() const {
      std::vector<char> result(value_size);
      uint32_t          actual_size;
      base->kv_it_value(0, result.data(), value_size, actual_size);
      if (!context.is_primary_idx) {
         auto success =
             context.kv_context->kv_get(context.p.code.to_uint64_t(), result.data(), result.size(), actual_size);
         EOS_ASSERT(success, chain::contract_table_query_exception, "invalid secondary index in ${t} ${i}",
                    ("t", context.p.table)("i", context.p.index_name));
         result.resize(actual_size);
         context.kv_context->kv_get_data(0, result.data(), actual_size);
      }

      return result;
   }

   /// @pre ! is_end()
   fc::variant get_value_var() const {
      std::vector<char> row_value = get_value();
      if (context.p.json) {
         try {
            return context.abis.binary_to_variant(context.p.table.to_string(), row_value,
                                                  context.yield_function,
                                                  context.shorten_abi_errors);
         } catch (fc::exception& e) {
         }
      }
      return fc::variant(row_value);
   }

   /// @pre ! is_end()
   fc::variant get_value_and_maybe_payer_var() const {
      fc::variant result = get_value_var();
      if (context.p.show_payer) {
         auto maybe_payer = base->kv_it_payer();
         std::string payer = maybe_payer.has_value() ? maybe_payer.value().to_string() : "";
         return fc::mutable_variant_object("data", std::move(result))("payer", payer);
      }
      return result;
   }

   /// @pre ! is_end()
   std::string get_key_hex_string() const {
      auto        row_key = get_key();
      std::string result;
      boost::algorithm::hex(row_key.begin() + prefix_size, row_key.end(), std::back_inserter(result));
      return result;
   }

   /// @pre ! is_end()
   kv_iterator_ex& operator++() {
      status = base->kv_it_next(&key_size, &value_size);
      return *this;
   }

   /// @pre ! is_end()
   kv_iterator_ex& operator--() {
      status = base->kv_it_prev(&key_size, &value_size);
      return *this;
   }

   int key_compare(const std::vector<char>& key) const {
      return base->kv_it_key_compare(key.data(), key.size());
   }
};

struct kv_forward_range {
   kv_iterator_ex           current;
   const std::vector<char>& last_key;

   kv_forward_range(const kv_table_rows_context& ctx, const std::vector<char>& first_key,
                    const std::vector<char>& last_key)
       : current(ctx, first_key)
       , last_key(last_key) {}

   bool is_done() const {
      return current.is_end() ||
             (last_key.size() > prefix_size && current.key_compare(last_key) > 0);
   }

   void next() { ++current; }
};

struct kv_reverse_range {
   kv_iterator_ex           current;
   const std::vector<char>& last_key;

   kv_reverse_range(const kv_table_rows_context& ctx, const std::vector<char>& first_key,
                    const std::vector<char>& last_key)
       : current(ctx, first_key)
       , last_key(last_key) {
      if (first_key.size() == prefix_size) {
         current.status = current.base->kv_it_move_to_end();
      }
      if (current.is_end() || current.key_compare(first_key) != 0)
         --current;
   }

   bool is_done() const {
      return current.is_end() ||
             (last_key.size() > prefix_size && current.key_compare(last_key) < 0);
   }

   void next() { --current; }
};

template <typename Range>
read_only::get_table_rows_result kv_get_rows(Range&& range) {

   keep_processing kp {};
   read_only::get_table_rows_result result;
   auto&                            ctx      = range.current.context;
   for (unsigned count = 0; count < ctx.p.limit && !range.is_done() && kp() ;
        ++count) {
      result.rows.emplace_back(range.current.get_value_and_maybe_payer_var());
      range.next();
   }

   if (!range.is_done()) {
      result.more           = true;
      result.next_key_bytes = range.current.get_key_hex_string();
      result.next_key = key_helper::read_key(ctx.index_type, ctx.p.encode_type, result.next_key_bytes);
   }
   return result;
}

read_only::get_table_rows_result read_only::get_kv_table_rows(const read_only::get_kv_table_rows_params& p) const {

   kv_table_rows_context context{db, p, abi_serializer_max_time, shorten_abi_errors};

   if (context.point_query()) {
      EOS_ASSERT(p.lower_bound.empty() && p.upper_bound.empty(), chain::contract_table_query_exception,
                 "specify both index_value and ranges (i.e. lower_bound/upper_bound) is not allowed");
      read_only::get_table_rows_result result;
      auto full_key = context.get_full_key(p.index_value);
      kv_iterator_ex                   itr(context, full_key);
      if (!itr.is_end() && itr.key_compare(full_key) == 0) {
         result.rows.emplace_back(itr.get_value_and_maybe_payer_var());
      }
      return result;
   }

   auto lower_bound = context.get_full_key(p.lower_bound);
   auto upper_bound = context.get_full_key(p.upper_bound);

   if (context.p.reverse == false)
      return kv_get_rows(kv_forward_range(context, lower_bound, upper_bound));
   else
      return kv_get_rows(kv_reverse_range(context, upper_bound, lower_bound));
}

struct table_receiver
  : chain::backing_store::table_only_error_receiver<table_receiver, chain::contract_table_query_exception> {
   table_receiver(read_only::get_table_by_scope_result& result, const read_only::get_table_by_scope_params& params)
   : result_(result), params_(params) {
      check_limit();
   }

   void add_table_row(const backing_store::table_id_object_view& row) {
      if( params_.table && row.table != params_.table ) {
         return;
      }
      result_.rows.push_back( {row.code, row.scope, row.table, row.payer, row.count} );
      check_limit();
   }

   auto keep_processing_entries() {
      keep_processing kp {};
      return [kp{std::move(kp)},&reached_limit=reached_limit_]() {
         return !reached_limit && kp();
      };
   };

   void check_limit() {
      if (result_.rows.size() >= params_.limit)
         reached_limit_ = true;
   }

   read_only::get_table_by_scope_result& result_;
   const read_only::get_table_by_scope_params& params_;
   bool reached_limit_ = false;
};

read_only::get_table_by_scope_result read_only::get_table_by_scope( const read_only::get_table_by_scope_params& p )const {
   read_only::get_table_by_scope_result result;
   auto lower_bound_lookup_tuple = std::make_tuple( p.code, name(std::numeric_limits<uint64_t>::lowest()), p.table );
   auto upper_bound_lookup_tuple = std::make_tuple( p.code, name(std::numeric_limits<uint64_t>::max()),
                                                    (p.table.empty() ? name(std::numeric_limits<uint64_t>::max()) : p.table) );

   if( p.lower_bound.size() ) {
      uint64_t scope = convert_to_type<uint64_t>(p.lower_bound, "lower_bound scope");
      std::get<1>(lower_bound_lookup_tuple) = name(scope);
   }

   if( p.upper_bound.size() ) {
      uint64_t scope = convert_to_type<uint64_t>(p.upper_bound, "upper_bound scope");
      std::get<1>(upper_bound_lookup_tuple) = name(scope);
   }

   if( upper_bound_lookup_tuple < lower_bound_lookup_tuple )
      return result;

   const bool reverse = p.reverse && *p.reverse;
   const auto db_backing_store = get_backing_store();
   if (db_backing_store == eosio::chain::backing_store_type::CHAINBASE) {
      auto walk_table_range = [&result,&p]( auto itr, auto end_itr ) {
         keep_processing kp;
         for( unsigned int count = 0; kp() && count < p.limit && itr != end_itr; ++itr ) {
            if( p.table && itr->table != p.table ) continue;

            result.rows.push_back( {itr->code, itr->scope, itr->table, itr->payer, itr->count} );

            ++count;
         }
         if( itr != end_itr ) {
            result.more = itr->scope.to_string();
         }
      };

      const auto& d = db.db();
      const auto& idx = d.get_index<chain::table_id_multi_index, chain::by_code_scope_table>();
      auto lower = idx.lower_bound( lower_bound_lookup_tuple );
      auto upper = idx.upper_bound( upper_bound_lookup_tuple );
      if( reverse ) {
         walk_table_range( boost::make_reverse_iterator(upper), boost::make_reverse_iterator(lower) );
      } else {
         walk_table_range( lower, upper );
      }
   }
   else {
      using namespace eosio::chain;
      EOS_ASSERT(db_backing_store == backing_store_type::ROCKSDB,
                 chain::contract_table_query_exception,
                 "Support for configured backing_store has not been added to get_table_by_scope");
      const auto& kv_database = db.kv_db();
      table_receiver receiver(result, p);
      auto kp = receiver.keep_processing_entries();
      auto lower = chain::backing_store::db_key_value_format::create_full_prefix_key(std::get<0>(lower_bound_lookup_tuple),
                                                                                     std::get<1>(lower_bound_lookup_tuple),
                                                                                     std::get<2>(lower_bound_lookup_tuple));
      auto upper = chain::backing_store::db_key_value_format::create_full_prefix_key(std::get<0>(upper_bound_lookup_tuple),
                                                                                     std::get<1>(upper_bound_lookup_tuple),
                                                                                     std::get<2>(upper_bound_lookup_tuple));
      if (reverse) {
         lower = eosio::session::shared_bytes::truncate_key(lower);
      }
      // since upper is either the upper_bound of a forward search, or the reverse iterator <= for the beginning of the end of
      // the table, we need to move it to just before the beginning of the next table
      upper = upper.next();
      const auto context = (reverse) ? backing_store::key_context::table_only_reverse : backing_store::key_context::table_only;
      backing_store::rocksdb_contract_db_table_writer<table_receiver, std::decay_t < decltype(kp)>> writer(receiver, context, kp);
      eosio::chain::backing_store::walk_rocksdb_entries_with_prefix(kv_database.get_kv_undo_stack(), lower, upper, writer);
      const auto stopped_at = writer.stopped_at();
      if (stopped_at) {
         result.more = stopped_at->scope.to_string();
      }
   }

   return result;
}

vector<asset> read_only::get_currency_balance( const read_only::get_currency_balance_params& p )const {

   const abi_def abi = eosio::chain_apis::get_abi( db, p.code );
   (void)get_table_type( abi, name("accounts") );

   vector<asset> results;
   walk_key_value_table(p.code, p.account, "accounts"_n, [&](const auto& obj){
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

   const abi_def abi = eosio::chain_apis::get_abi( db, p.code );
   (void)get_table_type( abi, name("stat") );

   uint64_t scope = ( eosio::chain::string_to_symbol( 0, boost::algorithm::to_upper_copy(p.symbol).c_str() ) >> 8 );

   walk_key_value_table(p.code, name(scope), "stat"_n, [&](const auto& obj){
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

read_only::get_producers_result read_only::get_producers( const read_only::get_producers_params& p ) const try {
   const auto producers_table = "producers"_n;
   const abi_def abi = eosio::chain_apis::get_abi(db, config::system_account_name);
   const auto table_type = get_table_type(abi, producers_table);
   const abi_serializer abis{ abi, abi_serializer::create_yield_function( abi_serializer_max_time ) };
   EOS_ASSERT(table_type == KEYi64, chain::contract_table_query_exception, "Invalid table type ${type} for table producers", ("type",table_type));

   const auto& d = db.db();
   const auto lower = name{p.lower_bound};

   keep_processing kp;
   read_only::get_producers_result result;
   auto done = [&kp,&result,&limit=p.limit](const auto& row) {
      if (result.rows.size() >= limit || !kp()) {
         result.more = name{row.primary_key}.to_string();
         return true;
      }
      return false;
   };
   auto type = abis.get_table_type(producers_table);
   auto get_val = get_primary_key_value(type, abis, p.json);
   auto add_val = [&result,get_val{std::move(get_val)}](const auto& row) {
      fc::variant data_var;
      get_val(data_var, row);
      result.rows.emplace_back(std::move(data_var));
   };

   const auto code = config::system_account_name;
   const auto scope = config::system_account_name;
   static const uint8_t secondary_index_num = 0;
   const name sec_producers_table {producers_table.to_uint64_t() | secondary_index_num};

   const auto db_backing_store = get_backing_store();
   if (db_backing_store == eosio::chain::backing_store_type::CHAINBASE) {
      const auto* const table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
            boost::make_tuple(code, scope, producers_table));
      const auto* const secondary_table_id = d.find<chain::table_id_object, chain::by_code_scope_table>(
            boost::make_tuple(code, scope, sec_producers_table));
      EOS_ASSERT(table_id && secondary_table_id, chain::contract_table_query_exception, "Missing producers table");

      const auto& kv_index = d.get_index<key_value_index, by_scope_primary>();
      const auto& secondary_index = d.get_index<index_double_index>().indices();
      const auto& secondary_index_by_primary = secondary_index.get<by_primary>();
      const auto& secondary_index_by_secondary = secondary_index.get<by_secondary>();

      vector<char> data;

      auto it = lower.to_uint64_t() == 0
         ? secondary_index_by_secondary.lower_bound(
               boost::make_tuple(secondary_table_id->id, to_softfloat64(std::numeric_limits<double>::lowest()), 0))
         : secondary_index.project<by_secondary>(
               secondary_index_by_primary.lower_bound(
                  boost::make_tuple(secondary_table_id->id, lower.to_uint64_t())));
      for( ; it != secondary_index_by_secondary.end() && it->t_id == secondary_table_id->id; ++it ) {
         if (done(*it)) {
            break;
         }
         auto itr = kv_index.find(boost::make_tuple(table_id->id, it->primary_key));
         add_val(*itr);
      }
   }
   else {
      using namespace eosio::chain;
      EOS_ASSERT(db_backing_store == backing_store_type::ROCKSDB,
                 contract_table_query_exception,
                 "Support for configured backing_store has not been added to get_table_by_scope");
      const auto& kv_database = db.kv_db();
      using key_type = backing_store::db_key_value_format::key_type;

      // derive the "root" of the float64 secondary key space to determine where it ends (upper), then may need to recalculate
      auto lower_key = backing_store::db_key_value_format::create_full_prefix_key(code, scope, sec_producers_table, key_type::sec_double);
      auto upper_key = lower_key.next();
      if (lower.to_uint64_t() == 0) {
         lower_key = backing_store::db_key_value_format::create_full_prefix_secondary_key(code, scope, sec_producers_table, float64_t{lower.to_uint64_t()});
      }

      auto session = kv_database.get_kv_undo_stack()->top();
      auto retrieve_primary_key = [&code,&scope,&producers_table,&session](uint64_t primary_key) {
         auto full_primary_key = backing_store::db_key_value_format::create_full_primary_key(code, scope, producers_table, primary_key);
         auto value = session.read(full_primary_key);
         return *value;
      };

      using done_func = decltype(done);
      using add_val_func = decltype(add_val);
      using retrieve_prim_func = decltype(retrieve_primary_key);

      struct f64_secondary_key_receiver
      : backing_store::single_type_error_receiver<f64_secondary_key_receiver,
                                                  backing_store::secondary_index_view<float64_t>,
                                                  contract_table_query_exception> {
         f64_secondary_key_receiver(read_only::get_producers_result& result, done_func&& done,
                                    add_val_func&& add_val, retrieve_prim_func&& retrieve_prim)
         : result_(result), done_(done), add_val_(add_val), retrieve_primary_key_(retrieve_prim) {}

         void add_only_row(const backing_store::secondary_index_view<float64_t>& row) {
            // needs to allow a second pass after limit is reached or time has passed, to allow "more" processing
            if (done_(row)) {
               finished_ = true;
            }
            else {
               auto value = retrieve_primary_key_(row.primary_key);
               add_val_(backing_store::primary_index_view::create(row.primary_key, value.data(), value.size()));
            }
         }

         void add_table_row(const backing_store::table_id_object_view& ) {
            // used for only one table, so we already know the context of the table
         }

         auto keep_processing_entries() {
            return [&finished=finished_]() {
               return !finished;
            };
         };

         read_only::get_producers_result& result_;
         done_func done_;
         add_val_func add_val_;
         retrieve_prim_func retrieve_primary_key_;
         bool finished_ = false;
      };
      f64_secondary_key_receiver receiver(result, std::move(done), std::move(add_val), std::move(retrieve_primary_key));
      auto kpe = receiver.keep_processing_entries();
      backing_store::rocksdb_contract_db_table_writer<f64_secondary_key_receiver, std::decay_t < decltype(kpe)>>
         writer(receiver, backing_store::key_context::standalone, kpe);
      eosio::chain::backing_store::walk_rocksdb_entries_with_prefix(kv_database.get_kv_undo_stack(), lower_key, upper_key, writer);
   };

   constexpr name global = "global"_n;
   const auto global_table_type = get_table_type(abi, global);
   EOS_ASSERT(global_table_type == read_only::KEYi64, chain::contract_table_query_exception, "Invalid table type ${type} for table global", ("type",global_table_type));
   auto var = get_primary_key(config::system_account_name, config::system_account_name, global, global.to_uint64_t(), row_requirements::required, row_requirements::required, abis.get_table_type(global));
   result.total_producer_vote_weight = var["total_producer_vote_weight"].as_double();
   return result;
} catch (...) {
   read_only::get_producers_result result;

   for (auto p : db.active_producers().producers) {
      auto row = fc::mutable_variant_object()
         ("owner", p.producer_name)
         ("producer_authority", p.authority)
         ("url", "")
         ("total_votes", 0.0f);

      // detect a legacy key and maintain API compatibility for those entries
      if (std::holds_alternative<block_signing_authority_v0>(p.authority)) {
         const auto& auth = std::get<block_signing_authority_v0>(p.authority);
         if (auth.keys.size() == 1 && auth.keys.back().weight == auth.threshold) {
            row("producer_key", auth.keys.back().key);
         }
      }

      result.rows.push_back(row);
   }

   return result;
}

read_only::get_producer_schedule_result read_only::get_producer_schedule( const read_only::get_producer_schedule_params& p ) const {
   read_only::get_producer_schedule_result result;
   to_variant(db.active_producers(), result.active);
   if(!db.pending_producers().producers.empty())
      to_variant(db.pending_producers(), result.pending);
   auto proposed = db.proposed_producers();
   if(proposed && !proposed->producers.empty())
      to_variant(*proposed, result.proposed);
   return result;
}

template<typename Api>
struct resolver_factory {
   static auto make(const Api* api, abi_serializer::yield_function_t yield) {
      return [api, yield{std::move(yield)}](const account_name &name) -> std::optional<abi_serializer> {
         const auto* accnt = api->db.db().template find<account_object, by_name>(name);
         if (accnt != nullptr) {
            abi_def abi;
            if (abi_serializer::to_abi(accnt->abi, abi)) {
               return abi_serializer(abi, yield);
            }
         }

         return std::optional<abi_serializer>();
      };
   }
};

template<typename Api>
auto make_resolver(const Api* api, abi_serializer::yield_function_t yield) {
   return resolver_factory<Api>::make(api, std::move( yield ));
}


read_only::get_scheduled_transactions_result
read_only::get_scheduled_transactions( const read_only::get_scheduled_transactions_params& p ) const {
   const auto& d = db.db();

   const auto& idx_by_delay = d.get_index<generated_transaction_multi_index,by_delay>();
   auto itr = ([&](){
      if (!p.lower_bound.empty()) {
         try {
            auto when = time_point::from_iso_string( p.lower_bound );
            return idx_by_delay.lower_bound(boost::make_tuple(when));
         } catch (...) {
            try {
               auto txid = transaction_id_type(p.lower_bound);
               const auto& by_txid = d.get_index<generated_transaction_multi_index,by_trx_id>();
               auto itr = by_txid.find( txid );
               if (itr == by_txid.end()) {
                  EOS_THROW(transaction_exception, "Unknown Transaction ID: ${txid}", ("txid", txid));
               }

               return d.get_index<generated_transaction_multi_index>().indices().project<by_delay>(itr);

            } catch (...) {
               return idx_by_delay.end();
            }
         }
      } else {
         return idx_by_delay.begin();
      }
   })();

   read_only::get_scheduled_transactions_result result;

   auto resolver = make_resolver(this, abi_serializer::create_yield_function( abi_serializer_max_time ));

   uint32_t remaining = p.limit;
   auto time_limit = fc::time_point::now() + fc::microseconds(1000 * 10); /// 10ms max time
   while (itr != idx_by_delay.end() && remaining > 0 && time_limit > fc::time_point::now()) {
      auto row = fc::mutable_variant_object()
              ("trx_id", itr->trx_id)
              ("sender", itr->sender)
              ("sender_id", itr->sender_id)
              ("payer", itr->payer)
              ("delay_until", itr->delay_until)
              ("expiration", itr->expiration)
              ("published", itr->published)
      ;

      if (p.json) {
         fc::variant pretty_transaction;

         transaction trx;
         fc::datastream<const char*> ds( itr->packed_trx.data(), itr->packed_trx.size() );
         fc::raw::unpack(ds,trx);

         abi_serializer::to_variant(trx, pretty_transaction, resolver, abi_serializer::create_yield_function( abi_serializer_max_time ));
         row("transaction", pretty_transaction);
      } else {
         auto packed_transaction = bytes(itr->packed_trx.begin(), itr->packed_trx.end());
         row("transaction", packed_transaction);
      }

      result.transactions.emplace_back(std::move(row));
      ++itr;
      remaining--;
   }

   if (itr != idx_by_delay.end()) {
      result.more = string(itr->trx_id);
   }

   return result;
}

fc::variant read_only::get_block(const read_only::get_block_params& params) const {
   signed_block_ptr block;
   std::optional<uint64_t> block_num;

   EOS_ASSERT( !params.block_num_or_id.empty() && params.block_num_or_id.size() <= 64,
               chain::block_id_type_exception,
               "Invalid Block number or ID, must be greater than 0 and less than 64 characters"
   );

   try {
      block_num = fc::to_uint64(params.block_num_or_id);
   } catch( ... ) {}

   if( block_num ) {
      block = db.fetch_block_by_number( *block_num );
   } else {
      try {
         block = db.fetch_block_by_id( fc::variant(params.block_num_or_id).as<block_id_type>() );
      } EOS_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))
   }

   EOS_ASSERT( block, unknown_block_exception, "Could not find block: ${block}", ("block", params.block_num_or_id));

   // serializes signed_block to variant in signed_block_v0 format
   fc::variant pretty_output;
   abi_serializer::to_variant(*block, pretty_output, make_resolver(this, abi_serializer::create_yield_function( abi_serializer_max_time )),
                              abi_serializer::create_yield_function( abi_serializer_max_time ));

   const auto id = block->calculate_id();
   const uint32_t ref_block_prefix = id._hash[1];

   return fc::mutable_variant_object(pretty_output.get_object())
           ("id", id)
           ("block_num",block->block_num())
           ("ref_block_prefix", ref_block_prefix);
}

fc::variant read_only::get_block_info(const read_only::get_block_info_params& params) const {

   signed_block_ptr block;
   try {
         block = db.fetch_block_by_number( params.block_num );
   } catch (...)   {
      // assert below will handle the invalid block num
   }

   EOS_ASSERT( block, unknown_block_exception, "Could not find block: ${block}", ("block", params.block_num));

   const auto id = block->calculate_id();
   const uint32_t ref_block_prefix = id._hash[1];

   return fc::mutable_variant_object ()
         ("block_num", block->block_num())
         ("ref_block_num", static_cast<uint16_t>(block->block_num()))
         ("id", id)
         ("timestamp", block->timestamp)
         ("producer", block->producer)
         ("confirmed", block->confirmed)
         ("previous", block->previous)
         ("transaction_mroot", block->transaction_mroot)
         ("action_mroot", block->action_mroot)
         ("schedule_version", block->schedule_version)
         ("producer_signature", block->producer_signature)
         ("ref_block_prefix", ref_block_prefix);
}

fc::variant read_only::get_block_header_state(const get_block_header_state_params& params) const {
   block_state_ptr b;
   std::optional<uint64_t> block_num;
   std::exception_ptr e;
   try {
      block_num = fc::to_uint64(params.block_num_or_id);
   } catch( ... ) {}

   if( block_num ) {
      b = db.fetch_block_state_by_number(*block_num);
   } else {
      try {
         b = db.fetch_block_state_by_id(fc::variant(params.block_num_or_id).as<block_id_type>());
      } EOS_RETHROW_EXCEPTIONS(chain::block_id_type_exception, "Invalid block ID: ${block_num_or_id}", ("block_num_or_id", params.block_num_or_id))
   }

   EOS_ASSERT( b, unknown_block_exception, "Could not find reversible block: ${block}", ("block", params.block_num_or_id));

   fc::variant vo;
   fc::to_variant( static_cast<const block_header_state&>(*b), vo );
   return vo;
}

void read_write::push_block(read_write::push_block_params&& params, next_function<read_write::push_block_results> next) {
   try {
      app().get_method<incoming::methods::block_sync>()(std::make_shared<signed_block>( std::move( params ), true), {});
      next(read_write::push_block_results{});
   } catch ( boost::interprocess::bad_alloc& ) {
      chain_plugin::handle_db_exhaustion();
   } catch ( const std::bad_alloc& ) {
      chain_plugin::handle_bad_alloc();
   } CATCH_AND_CALL(next);
}

void read_write::push_transaction(const read_write::push_transaction_params& params, next_function<read_write::push_transaction_results> next) {
   try {
      packed_transaction_v0 input_trx_v0;
      auto resolver = make_resolver(this, abi_serializer::create_yield_function( abi_serializer_max_time ));
      packed_transaction_ptr input_trx;
      try {
         abi_serializer::from_variant(params, input_trx_v0, std::move( resolver ), abi_serializer::create_yield_function( abi_serializer_max_time ));
         input_trx = std::make_shared<packed_transaction>( std::move( input_trx_v0 ), true );
      } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

      auto trx_trace = fc_create_trace_with_id("Transaction", input_trx->id());
      auto trx_span = fc_create_span(trx_trace, "HTTP Received");
      fc_add_tag(trx_span, "trx_id", input_trx->id());
      fc_add_tag(trx_span, "method", "push_transaction");

      app().get_method<incoming::methods::transaction_async>()(input_trx, true,
            [this, token=trx_trace.get_token(), input_trx, next]
            (const std::variant<fc::exception_ptr, transaction_trace_ptr>& result) -> void {

         auto trx_span = fc_create_span_from_token(token, "Processed");
         fc_add_tag(trx_span, "trx_id", input_trx->id());

         if (std::holds_alternative<fc::exception_ptr>(result)) {
            auto& eptr = std::get<fc::exception_ptr>(result);
            fc_add_tag(trx_span, "error", eptr->to_string());
            next(eptr);
         } else {
            auto& trx_trace_ptr = std::get<transaction_trace_ptr>(result);

            fc_add_tag(trx_span, "block_num", trx_trace_ptr->block_num);
            fc_add_tag(trx_span, "block_time", trx_trace_ptr->block_time.to_time_point());
            fc_add_tag(trx_span, "elapsed", trx_trace_ptr->elapsed.count());
            if( trx_trace_ptr->receipt ) {
               fc_add_tag(trx_span, "status", std::string(trx_trace_ptr->receipt->status));
            }
            if( trx_trace_ptr->except ) {
               fc_add_tag(trx_span, "error", trx_trace_ptr->except->to_string());
            }

            try {
               fc::variant output;
               try {
                  output = db.to_variant_with_abi( *trx_trace_ptr, abi_serializer::create_yield_function( abi_serializer_max_time ) );

                  // Create map of (closest_unnotified_ancestor_action_ordinal, global_sequence) with action trace
                  std::map< std::pair<uint32_t, uint64_t>, fc::mutable_variant_object > act_traces_map;
                  for( const auto& act_trace : output["action_traces"].get_array() ) {
                     if (act_trace["receipt"].is_null() && act_trace["except"].is_null()) continue;
                     auto closest_unnotified_ancestor_action_ordinal =
                           act_trace["closest_unnotified_ancestor_action_ordinal"].as<fc::unsigned_int>().value;
                     auto global_sequence = act_trace["receipt"].is_null() ?
                                                std::numeric_limits<uint64_t>::max() :
                                                act_trace["receipt"]["global_sequence"].as<uint64_t>();
                     act_traces_map.emplace( std::make_pair( closest_unnotified_ancestor_action_ordinal,
                                                             global_sequence ),
                                             act_trace.get_object() );
                  }

                  std::function<vector<fc::variant>(uint32_t)> convert_act_trace_to_tree_struct =
                  [&](uint32_t closest_unnotified_ancestor_action_ordinal) {
                     vector<fc::variant> restructured_act_traces;
                     auto it = act_traces_map.lower_bound(
                                 std::make_pair( closest_unnotified_ancestor_action_ordinal, 0)
                     );
                     for( ;
                        it != act_traces_map.end() && it->first.first == closest_unnotified_ancestor_action_ordinal; ++it )
                     {
                        auto& act_trace_mvo = it->second;

                        auto action_ordinal = act_trace_mvo["action_ordinal"].as<fc::unsigned_int>().value;
                        act_trace_mvo["inline_traces"] = convert_act_trace_to_tree_struct(action_ordinal);
                        if (act_trace_mvo["receipt"].is_null()) {
                           act_trace_mvo["receipt"] = fc::mutable_variant_object()
                              ("abi_sequence", 0)
                              ("act_digest", digest_type::hash(trx_trace_ptr->action_traces[action_ordinal-1].act))
                              ("auth_sequence", flat_map<account_name,uint64_t>())
                              ("code_sequence", 0)
                              ("global_sequence", 0)
                              ("receiver", act_trace_mvo["receiver"])
                              ("recv_sequence", 0);
                        }
                        restructured_act_traces.push_back( std::move(act_trace_mvo) );
                     }
                     return restructured_act_traces;
                  };

                  fc::mutable_variant_object output_mvo(output);
                  output_mvo["action_traces"] = convert_act_trace_to_tree_struct(0);

                  output = output_mvo;
               } catch( chain::abi_exception& ) {
                  output = *trx_trace_ptr;
               }

               const chain::transaction_id_type& id = trx_trace_ptr->id;
               next(read_write::push_transaction_results{id, output});
            } CATCH_AND_CALL(next);
         }
      });
   } catch ( boost::interprocess::bad_alloc& ) {
      chain_plugin::handle_db_exhaustion();
   } catch ( const std::bad_alloc& ) {
      chain_plugin::handle_bad_alloc();
   } CATCH_AND_CALL(next);
}

static void push_recurse(read_write* rw, int index, const std::shared_ptr<read_write::push_transactions_params>& params, const std::shared_ptr<read_write::push_transactions_results>& results, const next_function<read_write::push_transactions_results>& next) {
   auto wrapped_next = [=](const std::variant<fc::exception_ptr, read_write::push_transaction_results>& result) {
      if (std::holds_alternative<fc::exception_ptr>(result)) {
         const auto& e = std::get<fc::exception_ptr>(result);
         results->emplace_back( read_write::push_transaction_results{ transaction_id_type(), fc::mutable_variant_object( "error", e->to_detail_string() ) } );
      } else {
         const auto& r = std::get<read_write::push_transaction_results>(result);
         results->emplace_back( r );
      }

      size_t next_index = index + 1;
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
      EOS_ASSERT( params.size() <= 1000, too_many_tx_at_once, "Attempt to push too many transactions at once" );
      auto params_copy = std::make_shared<read_write::push_transactions_params>(params.begin(), params.end());
      auto result = std::make_shared<read_write::push_transactions_results>();
      result->reserve(params.size());

      push_recurse(this, 0, params_copy, result, next);
   } catch ( boost::interprocess::bad_alloc& ) {
      chain_plugin::handle_db_exhaustion();
   } catch ( const std::bad_alloc& ) {
      chain_plugin::handle_bad_alloc();
   } CATCH_AND_CALL(next);
}

void read_write::send_transaction(const read_write::send_transaction_params& params, next_function<read_write::send_transaction_results> next) {

   try {
      packed_transaction_v0 input_trx_v0;
      auto resolver = make_resolver(this, abi_serializer::create_yield_function( abi_serializer_max_time ));
      packed_transaction_ptr input_trx;
      try {
         abi_serializer::from_variant(params, input_trx_v0, std::move( resolver ), abi_serializer::create_yield_function( abi_serializer_max_time ));
         input_trx = std::make_shared<packed_transaction>( std::move( input_trx_v0 ), true );
      } EOS_RETHROW_EXCEPTIONS(chain::packed_transaction_type_exception, "Invalid packed transaction")

      auto trx_trace = fc_create_trace_with_id("Transaction", input_trx->id());
      auto trx_span = fc_create_span(trx_trace, "HTTP Received");
      fc_add_tag(trx_span, "trx_id", input_trx->id());
      fc_add_tag(trx_span, "method", "send_transaction");

      app().get_method<incoming::methods::transaction_async>()(input_trx, true,
            [this, token=trx_trace.get_token(), input_trx, next]
            (const std::variant<fc::exception_ptr, transaction_trace_ptr>& result) -> void {
         auto trx_span = fc_create_span_from_token(token, "Processed");
         fc_add_tag(trx_span, "trx_id", input_trx->id());

         if (std::holds_alternative<fc::exception_ptr>(result)) {
            auto& eptr = std::get<fc::exception_ptr>(result);
            fc_add_tag(trx_span, "error", eptr->to_string());
            next(eptr);
         } else {
            auto& trx_trace_ptr = std::get<transaction_trace_ptr>(result);

            fc_add_tag(trx_span, "block_num", trx_trace_ptr->block_num);
            fc_add_tag(trx_span, "block_time", trx_trace_ptr->block_time.to_time_point());
            fc_add_tag(trx_span, "elapsed", trx_trace_ptr->elapsed.count());
            if( trx_trace_ptr->receipt ) {
               fc_add_tag(trx_span, "status", std::string(trx_trace_ptr->receipt->status));
            }
            if( trx_trace_ptr->except ) {
               fc_add_tag(trx_span, "error", trx_trace_ptr->except->to_string());
            }

            try {
               fc::variant output;
               try {
                  output = db.to_variant_with_abi( *trx_trace_ptr, abi_serializer::create_yield_function( abi_serializer_max_time ) );
               } catch( chain::abi_exception& ) {
                  output = *trx_trace_ptr;
               }

               const chain::transaction_id_type& id = trx_trace_ptr->id;
               next(read_write::send_transaction_results{id, output});
            } CATCH_AND_CALL(next);
         }
      });
   } catch ( boost::interprocess::bad_alloc& ) {
      chain_plugin::handle_db_exhaustion();
   } catch ( const std::bad_alloc& ) {
      chain_plugin::handle_bad_alloc();
   } CATCH_AND_CALL(next);
}

read_only::get_abi_results read_only::get_abi( const get_abi_params& params )const {
   get_abi_results result;
   result.account_name = params.account_name;
   const auto& d = db.db();
   const auto& accnt  = d.get<account_object,by_name>( params.account_name );

   abi_def abi;
   if( abi_serializer::to_abi(accnt.abi, abi) ) {
      result.abi = std::move(abi);
   }

   return result;
}

read_only::get_code_results read_only::get_code( const get_code_params& params )const {
   get_code_results result;
   result.account_name = params.account_name;
   const auto& d = db.db();
   const auto& accnt_obj          = d.get<account_object,by_name>( params.account_name );
   const auto& accnt_metadata_obj = d.get<account_metadata_object,by_name>( params.account_name );

   EOS_ASSERT( params.code_as_wasm, unsupported_feature, "Returning WAST from get_code is no longer supported" );

   if( accnt_metadata_obj.code_hash != digest_type() ) {
      const auto& code_obj = d.get<code_object, by_code_hash>(accnt_metadata_obj.code_hash);
      result.wasm = string(code_obj.code.begin(), code_obj.code.end());
      result.code_hash = code_obj.code_hash;
   }

   abi_def abi;
   if( abi_serializer::to_abi(accnt_obj.abi, abi) ) {
      result.abi = std::move(abi);
   }

   return result;
}

read_only::get_code_hash_results read_only::get_code_hash( const get_code_hash_params& params )const {
   get_code_hash_results result;
   result.account_name = params.account_name;
   const auto& d = db.db();
   const auto& accnt  = d.get<account_metadata_object,by_name>( params.account_name );

   if( accnt.code_hash != digest_type() )
      result.code_hash = accnt.code_hash;

   return result;
}

read_only::get_raw_code_and_abi_results read_only::get_raw_code_and_abi( const get_raw_code_and_abi_params& params)const {
   get_raw_code_and_abi_results result;
   result.account_name = params.account_name;

   const auto& d = db.db();
   const auto& accnt_obj          = d.get<account_object,by_name>(params.account_name);
   const auto& accnt_metadata_obj = d.get<account_metadata_object,by_name>(params.account_name);
   if( accnt_metadata_obj.code_hash != digest_type() ) {
      const auto& code_obj = d.get<code_object, by_code_hash>(accnt_metadata_obj.code_hash);
      result.wasm = blob{{code_obj.code.begin(), code_obj.code.end()}};
   }
   result.abi = blob{{accnt_obj.abi.begin(), accnt_obj.abi.end()}};

   return result;
}

read_only::get_raw_abi_results read_only::get_raw_abi( const get_raw_abi_params& params )const {
   get_raw_abi_results result;
   result.account_name = params.account_name;

   const auto& d = db.db();
   const auto& accnt_obj          = d.get<account_object,by_name>(params.account_name);
   const auto& accnt_metadata_obj = d.get<account_metadata_object,by_name>(params.account_name);
   result.abi_hash = fc::sha256::hash( accnt_obj.abi.data(), accnt_obj.abi.size() );
   if( accnt_metadata_obj.code_hash != digest_type() )
      result.code_hash = accnt_metadata_obj.code_hash;
   if( !params.abi_hash || *params.abi_hash != result.abi_hash )
      result.abi = blob{{accnt_obj.abi.begin(), accnt_obj.abi.end()}};

   return result;
}

read_only::get_account_results read_only::get_account( const get_account_params& params )const {
   get_account_results result;
   result.account_name = params.account_name;

   const auto& d = db.db();
   const auto& rm = db.get_resource_limits_manager();

   result.head_block_num  = db.head_block_num();
   result.head_block_time = db.head_block_time();

   rm.get_account_limits( result.account_name, result.ram_quota, result.net_weight, result.cpu_weight );

   const auto& accnt_obj = db.get_account( result.account_name );
   const auto& accnt_metadata_obj = db.db().get<account_metadata_object,by_name>( result.account_name );

   result.privileged       = accnt_metadata_obj.is_privileged();
   result.last_code_update = accnt_metadata_obj.last_code_update;
   result.created          = accnt_obj.creation_date;

   uint32_t greylist_limit = db.is_resource_greylisted(result.account_name) ? 1 : config::maximum_elastic_resource_multiplier;
   const block_timestamp_type current_usage_time (db.head_block_time());
   result.net_limit.set( rm.get_account_net_limit_ex( result.account_name, greylist_limit, current_usage_time).first );
   if ( result.net_limit.last_usage_update_time && (result.net_limit.last_usage_update_time->slot == 0) ) {   // account has no action yet
      result.net_limit.last_usage_update_time = accnt_obj.creation_date;
   }
   result.cpu_limit.set( rm.get_account_cpu_limit_ex( result.account_name, greylist_limit, current_usage_time).first );
   if ( result.cpu_limit.last_usage_update_time && (result.cpu_limit.last_usage_update_time->slot == 0) ) {   // account has no action yet
      result.cpu_limit.last_usage_update_time = accnt_obj.creation_date;
   }
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
            EOS_ASSERT(perm->owner == p->owner, invalid_parent_permission, "Invalid parent permission");
            parent = p->name;
         }
      }

      result.permissions.push_back( permission{ perm->name, parent, perm->auth.to_authority() } );
      ++perm;
   }

   const auto& code_account = db.db().get<account_object,by_name>( config::system_account_name );

   abi_def abi;
   if( abi_serializer::to_abi(code_account.abi, abi) ) {
      abi_serializer abis( abi, abi_serializer::create_yield_function( abi_serializer_max_time ) );

      const auto token_code = "eosio.token"_n;

      auto core_symbol = extract_core_symbol();

      if (params.expected_core_symbol)
         core_symbol = *(params.expected_core_symbol);

      get_primary_key<asset>(token_code, params.account_name, "accounts"_n, core_symbol.to_symbol_code(),
		      row_requirements::optional, row_requirements::optional, [&core_symbol,&result](const asset& bal) {
         if( bal.get_symbol().valid() && bal.get_symbol() == core_symbol ) {
            result.core_liquid_balance = bal;
         }
      });

      result.total_resources = get_primary_key(config::system_account_name, params.account_name, "userres"_n, params.account_name.to_uint64_t(),
		      row_requirements::optional, row_requirements::optional, "user_resources", abis); 

      result.self_delegated_bandwidth = get_primary_key(config::system_account_name, params.account_name, "delband"_n, params.account_name.to_uint64_t(),
		      row_requirements::optional, row_requirements::optional, "delegated_bandwidth", abis); 

      result.refund_request = get_primary_key(config::system_account_name, params.account_name, "refunds"_n, params.account_name.to_uint64_t(),
		      row_requirements::optional, row_requirements::optional, "refund_request", abis); 

      result.voter_info = get_primary_key(config::system_account_name, config::system_account_name, "voters"_n, params.account_name.to_uint64_t(),
		      row_requirements::optional, row_requirements::optional, "voter_info", abis); 

      result.rex_info = get_primary_key(config::system_account_name, config::system_account_name, "rexbal"_n, params.account_name.to_uint64_t(),
		      row_requirements::optional, row_requirements::optional, "rex_balance", abis); 
   }
   return result;
}

static fc::variant action_abi_to_variant( const abi_def& abi, type_name action_type ) {
   fc::variant v;
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
      abi_serializer abis( abi, abi_serializer::create_yield_function( abi_serializer_max_time ) );
      auto action_type = abis.get_action_type(params.action);
      EOS_ASSERT(!action_type.empty(), action_validate_exception, "Unknown action ${action} in contract ${contract}", ("action", params.action)("contract", params.code));
      try {
         result.binargs = abis.variant_to_binary( action_type, params.args, abi_serializer::create_yield_function( abi_serializer_max_time ), shorten_abi_errors );
      } EOS_RETHROW_EXCEPTIONS(chain::invalid_action_args_exception,
                                "'${args}' is invalid args for action '${action}' code '${code}'. expected '${proto}'",
                                ("args", params.args)("action", params.action)("code", params.code)("proto", action_abi_to_variant(abi, action_type)))
   } else {
      EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
   }
   return result;
} FC_RETHROW_EXCEPTIONS( warn, "code: ${code}, action: ${action}, args: ${args}",
                         ("code", params.code)( "action", params.action )( "args", params.args ))

read_only::abi_bin_to_json_result read_only::abi_bin_to_json( const read_only::abi_bin_to_json_params& params )const {
   abi_bin_to_json_result result;
   const auto& code_account = db.db().get<account_object,by_name>( params.code );
   abi_def abi;
   if( abi_serializer::to_abi(code_account.abi, abi) ) {
      abi_serializer abis( abi, abi_serializer::create_yield_function( abi_serializer_max_time ) );
      result.args = abis.binary_to_variant( abis.get_action_type( params.action ), params.binargs, abi_serializer::create_yield_function( abi_serializer_max_time ), shorten_abi_errors );
   } else {
      EOS_ASSERT(false, abi_not_found_exception, "No ABI found for ${contract}", ("contract", params.code));
   }
   return result;
}

read_only::get_required_keys_result read_only::get_required_keys( const get_required_keys_params& params )const {
   transaction pretty_input;
   auto resolver = make_resolver(this, abi_serializer::create_yield_function( abi_serializer_max_time ));
   try {
      abi_serializer::from_variant(params.transaction, pretty_input, resolver, abi_serializer::create_yield_function( abi_serializer_max_time ));
   } EOS_RETHROW_EXCEPTIONS(chain::transaction_type_exception, "Invalid transaction")

   auto required_keys_set = db.get_authorization_manager().get_required_keys( pretty_input, params.available_keys, fc::seconds( pretty_input.delay_sec ));
   get_required_keys_result result;
   result.required_keys = required_keys_set;
   return result;
}

read_only::get_transaction_id_result read_only::get_transaction_id( const read_only::get_transaction_id_params& params)const {
   return params.id();
}

account_query_db::get_accounts_by_authorizers_result read_only::get_accounts_by_authorizers( const account_query_db::get_accounts_by_authorizers_params& args) const
{
   EOS_ASSERT(aqdb.has_value(), plugin_config_exception, "Account Queries being accessed when not enabled");
   return aqdb->get_accounts_by_authorizers(args);
}  
  
namespace detail {
   struct ram_market_exchange_state_t {
      asset  ignore1;
      asset  ignore2;
      double ignore3;
      asset  core_symbol;
      double ignore4;
   };
}

chain::symbol read_only::extract_core_symbol()const {
   symbol core_symbol(0);

   // The following code makes assumptions about the contract deployed on eosio account (i.e. the system contract) and how it stores its data.
   get_primary_key<detail::ram_market_exchange_state_t>("eosio"_n, "eosio"_n, "rammarket"_n, eosio::chain::string_to_symbol_c(4,"RAMCORE"),
		      row_requirements::optional, row_requirements::optional, [&core_symbol](const detail::ram_market_exchange_state_t& ram_market_exchange_state) {
         if( ram_market_exchange_state.core_symbol.get_symbol().valid() ) {
            core_symbol = ram_market_exchange_state.core_symbol.get_symbol();
         }
   });

   return core_symbol;
}

fc::variant read_only::get_primary_key(name code, name scope, name table, uint64_t primary_key, row_requirements require_table,
                                       row_requirements require_primary, const std::string_view& type, bool as_json) const {
   const abi_def abi = eosio::chain_apis::get_abi(db, code);
   abi_serializer abis;
   abis.set_abi(abi, abi_serializer::create_yield_function(abi_serializer_max_time));
   return get_primary_key(code, scope, table, primary_key, require_table, require_primary, type, abis, as_json);
}

fc::variant read_only::get_primary_key(name code, name scope, name table, uint64_t primary_key, row_requirements require_table,
                                       row_requirements require_primary, const std::string_view& type, const abi_serializer& abis,
                                       bool as_json) const {
   fc::variant val;
   const auto valid = get_primary_key_internal(code, scope, table, primary_key, require_table, require_primary, get_primary_key_value(val, type, abis, as_json));
   return val;
}

eosio::chain::backing_store_type read_only::get_backing_store() const {
   const auto& kv_database = db.kv_db();
   return kv_database.get_backing_store();
}

} // namespace chain_apis
} // namespace eosio

FC_REFLECT( eosio::chain_apis::detail::ram_market_exchange_state_t, (ignore1)(ignore2)(ignore3)(core_symbol)(ignore4) )
