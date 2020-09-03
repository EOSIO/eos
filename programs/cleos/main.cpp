/**
  @defgroup eosclienttool

  @section intro Introduction to cleos

  `cleos` is a command line tool that interfaces with the REST api exposed by @ref nodeos. In order to use `cleos` you will need to
  have a local copy of `nodeos` running and configured to load the 'eosio::chain_api_plugin'.

   cleos contains documentation for all of its commands. For a list of all commands known to cleos, simply run it with no arguments:
```
$ ./cleos
Command Line Interface to EOSIO Client
Usage: programs/cleos/cleos [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help message and exit
  -u,--url TEXT=http://localhost:8888/
                              the http/https URL where nodeos is running
  --wallet-url TEXT=http://localhost:8888/
                              the http/https URL where keosd is running
  -r,--header                 pass specific HTTP header, repeat this option to pass multiple headers
  -n,--no-verify              don't verify peer certificate when using HTTPS
  -v,--verbose                output verbose errors and action output

Subcommands:
  version                     Retrieve version information
  create                      Create various items, on and off the blockchain
  get                         Retrieve various items and information from the blockchain
  set                         Set or update blockchain state
  transfer                    Transfer tokens from account to account
  net                         Interact with local p2p network connections
  wallet                      Interact with local wallet
  sign                        Sign a transaction
  push                        Push arbitrary transactions to the blockchain
  multisig                    Multisig contract commands

```
To get help with any particular subcommand, run it with no arguments as well:
```
$ ./cleos create
Create various items, on and off the blockchain
Usage: ./cleos create SUBCOMMAND

Subcommands:
  key                         Create a new keypair and print the public and private keys
  account                     Create a new account on the blockchain (assumes system contract does not restrict RAM usage)

$ ./cleos create account
Create a new account on the blockchain (assumes system contract does not restrict RAM usage)
Usage: ./cleos create account [OPTIONS] creator name OwnerKey ActiveKey

Positionals:
  creator TEXT                The name of the account creating the new account
  name TEXT                   The name of the new account
  OwnerKey TEXT               The owner public key for the new account
  ActiveKey TEXT              The active public key for the new account

Options:
  -x,--expiration             set the time in seconds before a transaction expires, defaults to 30s
  -f,--force-unique           force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times
  -s,--skip-sign              Specify if unlocked wallet keys should be used to sign transaction
  -d,--dont-broadcast         don't broadcast transaction to the network (just print to stdout)
  -p,--permission TEXT ...    An account and permission level to authorize, as in 'account@permission' (defaults to 'creator@active')
```
*/

#include <pwd.h>
#include <string>
#include <vector>
#include <regex>
#include <iostream>
#include <fc/crypto/hex.hpp>
#include <fc/variant.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/exception/exception.hpp>
#include <fc/variant_object.hpp>
#include <fc/static_variant.hpp>

#include <eosio/chain/name.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/thread_utils.hpp>

#include <eosio/amqp_trx_plugin/amqp_trx_plugin.hpp>
#include <eosio/amqp/amqp_handler.hpp>

#include <eosio/version/version.hpp>

#pragma push_macro("N")
#undef N

#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/process/spawn.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/algorithm/copy.hpp>

#pragma pop_macro("N")

#include <fc/io/fstream.hpp>

#define CLI11_HAS_FILESYSTEM 0
#include "CLI11.hpp"
#include "help_text.hpp"
#include "localize.hpp"
#include "config.hpp"
#include "httpc.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::client::help;
using namespace eosio::client::http;
using namespace eosio::client::localize;
using namespace eosio::client::config;
using namespace boost::filesystem;

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );
FC_DECLARE_EXCEPTION( localized_exception, 10000000, "an error occured" );
#define EOSC_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                   \
        std::cerr << localized( __VA_ARGS__ ) << std::endl;  \
        FC_THROW_EXCEPTION( explained_exception, #TEST ); \
      }                                                   \
    FC_MULTILINE_MACRO_END \
  )

//copy pasta from keosd's main.cpp
bfs::path determine_home_directory()
{
   bfs::path home;
   struct passwd* pwd = getpwuid(getuid());
   if(pwd) {
      home = pwd->pw_dir;
   }
   else {
      home = getenv("HOME");
   }
   if(home.empty())
      home = "./";
   return home;
}

string default_url = "http://127.0.0.1:8888/";
string default_wallet_url = "unix://" + (determine_home_directory() / "eosio-wallet" / (string(key_store_executable_name) + ".sock")).string();
string wallet_url; //to be set to default_wallet_url in main
string amqp_address;
bool no_verify = false;
vector<string> headers;

auto   tx_expiration = fc::seconds(30);
const fc::microseconds abi_serializer_max_time = fc::seconds(10); // No risk to client side serialization taking a long time
string tx_ref_block_num_or_id;
bool   tx_force_unique = false;
bool   tx_dont_broadcast = false;
bool   tx_return_packed = false;
bool   tx_skip_sign = false;
bool   tx_print_json = false;
bool   tx_use_old_rpc = false;
string tx_json_save_file;
bool   print_request = false;
bool   print_response = false;
bool   no_auto_keosd = false;
bool   verbose = false;

uint8_t  tx_max_cpu_usage = 0;
uint32_t tx_max_net_usage = 0;

uint32_t delaysec = 0;

vector<string> tx_permission;

eosio::client::http::http_context context;

enum class tx_compression_type {
   none,
   zlib,
   default_compression
};
static std::map<std::string, tx_compression_type> compression_type_map{
   {"none", tx_compression_type::none },
   {"zlib", tx_compression_type::zlib }
};
tx_compression_type tx_compression = tx_compression_type::default_compression;
packed_transaction::compression_type to_compression_type( tx_compression_type t ) {
   switch( t ) {
      case tx_compression_type::none: return packed_transaction::compression_type::none;
      case tx_compression_type::zlib: return packed_transaction::compression_type::zlib;
      case tx_compression_type::default_compression: return packed_transaction::compression_type::none;
   }
}

void add_standard_transaction_options(CLI::App* cmd, string default_permission = "") {
   CLI::callback_t parse_expiration = [](CLI::results_t res) -> bool {
      double value_s;
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_s)) {
         return false;
      }

      tx_expiration = fc::seconds(static_cast<uint64_t>(value_s));
      return true;
   };

   cmd->add_option("-x,--expiration", parse_expiration, localized("Set the time in seconds before a transaction expires, defaults to 30s"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localized("Force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times"));
   cmd->add_flag("-s,--skip-sign", tx_skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   cmd->add_flag("-j,--json", tx_print_json, localized("Print result as JSON"));
   cmd->add_option("--json-file", tx_json_save_file, localized("Save result in JSON format into a file"));
   cmd->add_flag("-d,--dont-broadcast", tx_dont_broadcast, localized("Don't broadcast transaction to the network (just print to stdout)"));
   cmd->add_flag("--return-packed", tx_return_packed, localized("Used in conjunction with --dont-broadcast to get the packed transaction"));
   cmd->add_option("-r,--ref-block", tx_ref_block_num_or_id, (localized("Set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)")));
   cmd->add_flag("--use-old-rpc", tx_use_old_rpc, localized("Use old RPC push_transaction, rather than new RPC send_transaction"));
   cmd->add_option("--compression", tx_compression, localized("Compression for transaction 'none' or 'zlib'"))->transform(
         CLI::CheckedTransformer(compression_type_map, CLI::ignore_case));

   string msg = "An account and permission level to authorize, as in 'account@permission'";
   if(!default_permission.empty())
      msg += " (defaults to '" + default_permission + "')";
   cmd->add_option("-p,--permission", tx_permission, localized(msg.c_str()));

   cmd->add_option("--max-cpu-usage-ms", tx_max_cpu_usage, localized("Set an upper limit on the milliseconds of cpu usage budget, for the execution of the transaction (defaults to 0 which means no limit)"));
   cmd->add_option("--max-net-usage", tx_max_net_usage, localized("Set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)"));

   cmd->add_option("--delay-sec", delaysec, localized("Set the delay_sec seconds, defaults to 0s"));
}

bool is_public_key_str(const std::string& potential_key_str) {
   return boost::istarts_with(potential_key_str, "EOS") || boost::istarts_with(potential_key_str, "PUB_R1") ||  boost::istarts_with(potential_key_str, "PUB_K1") ||  boost::istarts_with(potential_key_str, "PUB_WA");
}

class signing_keys_option {
public:
   signing_keys_option() {}
   void add_option(CLI::App* cmd) {
      cmd->add_option("--sign-with", public_key_json, localized("The public key or json array of public keys to sign with"));
   }

   std::vector<public_key_type> get_keys() {
      std::vector<public_key_type> signing_keys;
      if (!public_key_json.empty()) {
         if (is_public_key_str(public_key_json)) {
            try {
               signing_keys.push_back(public_key_type(public_key_json));
            } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", public_key_json))
         } else {
            fc::variant json_keys;
            try {
               json_keys = fc::json::from_string(public_key_json, fc::json::parse_type::relaxed_parser);
            } EOS_RETHROW_EXCEPTIONS(json_parse_exception, "Fail to parse JSON from string: ${string}", ("string", public_key_json));
            try {
               std::vector<public_key_type> keys = json_keys.template as<std::vector<public_key_type>>();
               signing_keys = std::move(keys);
            } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key array format '${data}'",
                                     ("data", fc::json::to_string(json_keys, fc::time_point::maximum())))
         }
      }
      return signing_keys;
   }
private:
   string public_key_json;
};

signing_keys_option signing_keys_opt;


void add_standard_transaction_options_plus_signing(CLI::App* cmd, string default_permission = "") {
   add_standard_transaction_options(cmd, default_permission);
   signing_keys_opt.add_option(cmd);
}

vector<chain::permission_level> get_account_permissions(const vector<string>& permissions) {
   auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
      vector<string> pieces;
      split(pieces, p, boost::algorithm::is_any_of("@"));
      if( pieces.size() == 1 ) pieces.push_back( "active" );
      return chain::permission_level{ .actor = name(pieces[0]), .permission = name(pieces[1]) };
   });
   vector<chain::permission_level> accountPermissions;
   boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
   return accountPermissions;
}

vector<chain::permission_level> get_account_permissions(const vector<string>& permissions, const chain::permission_level& default_permission) {
   if (permissions.empty())
      return vector<chain::permission_level>{default_permission};
   else
      return get_account_permissions(tx_permission);
}

template<typename T>
fc::variant call( const std::string& url,
                  const std::string& path,
                  const T& v ) {
   try {
      auto sp = std::make_unique<eosio::client::http::connection_param>(context, parse_url(url) + path, no_verify ? false : true, headers);
      return eosio::client::http::do_http_call(*sp, fc::variant(v), print_request, print_response );
   }
   catch(boost::system::system_error& e) {
      if(url == ::default_url)
         std::cerr << localized("Failed to connect to ${n} at ${u}; is ${n} running?", ("n", node_executable_name)("u", url)) << std::endl;
      else if(url == ::wallet_url)
         std::cerr << localized("Failed to connect to ${k} at ${u}; is ${k} running?", ("k", key_store_executable_name)("u", url)) << std::endl;
      throw connection_exception(fc::log_messages{FC_LOG_MESSAGE(error, e.what())});
   }
}

template<typename T>
fc::variant call( const std::string& path,
                  const T& v ) { return call( default_url, path, fc::variant( v) ); }

template<>
fc::variant call( const std::string& url,
                  const std::string& path) { return call( url, path, fc::variant() ); }

eosio::chain_apis::read_only::get_info_results get_info() {
   return call(default_url, get_info_func).as<eosio::chain_apis::read_only::get_info_results>();
}

string generate_nonce_string() {
   return fc::to_string(fc::time_point::now().time_since_epoch().count());
}

chain::action generate_nonce_action() {
   return chain::action( {}, config::null_account_name, name("nonce"), fc::raw::pack(fc::time_point::now().time_since_epoch().count()));
}

//resolver for ABI serializer to decode actions in proposed transaction in multisig contract
auto abi_serializer_resolver = [](const name& account) -> fc::optional<abi_serializer> {
  static unordered_map<account_name, fc::optional<abi_serializer> > abi_cache;
  auto it = abi_cache.find( account );
  if ( it == abi_cache.end() ) {
    const auto raw_abi_result = call(get_raw_abi_func, fc::mutable_variant_object("account_name", account));
    const auto raw_abi_blob = raw_abi_result["abi"].as_blob().data;

    fc::optional<abi_serializer> abis;
    if (raw_abi_blob.size() != 0) {
      abis.emplace(fc::raw::unpack<abi_def>(raw_abi_blob), abi_serializer::create_yield_function( abi_serializer_max_time ));
    } else {
      std::cerr << "ABI for contract " << account.to_string() << " not found. Action data will be shown in hex only." << std::endl;
    }
    abi_cache.emplace( account, abis );

    return abis;
  }

  return it->second;
};

auto abi_serializer_resolver_empty = [](const name& account) -> fc::optional<abi_serializer> {
   return fc::optional<abi_serializer>();
};

void prompt_for_wallet_password(string& pw, const string& name) {
   if(pw.size() == 0 && name != "SecureEnclave") {
      std::cout << localized("password: ");
      fc::set_console_echo(false);
      std::getline( std::cin, pw, '\n' );
      fc::set_console_echo(true);
   }
}

fc::variant determine_required_keys(const signed_transaction& trx) {
   // TODO better error checking
   //wdump((trx));
   const auto& public_keys = call(wallet_url, wallet_public_keys);
   auto get_arg = fc::mutable_variant_object
           ("transaction", (transaction)trx)
           ("available_keys", public_keys);
   const auto& required_keys = call(get_required_keys, get_arg);
   return required_keys["required_keys"];
}

void sign_transaction(signed_transaction& trx, fc::variant& required_keys, const chain_id_type& chain_id) {
   fc::variants sign_args = {fc::variant(trx), required_keys, fc::variant(chain_id)};
   const auto& signed_trx = call(wallet_url, wallet_sign_trx, sign_args);
   trx = signed_trx.as<signed_transaction>();
}

fc::variant push_transaction( signed_transaction& trx, const std::vector<public_key_type>& signing_keys = std::vector<public_key_type>() )
{
   auto info = get_info();

   if (trx.signatures.size() == 0) { // #5445 can't change txn content if already signed
      trx.expiration = info.head_block_time + tx_expiration;

      // Set tapos, default to last irreversible block if it's not specified by the user
      block_id_type ref_block_id = info.last_irreversible_block_id;
      try {
         fc::variant ref_block;
         if (!tx_ref_block_num_or_id.empty()) {
            ref_block = call(get_block_func, fc::mutable_variant_object("block_num_or_id", tx_ref_block_num_or_id));
            ref_block_id = ref_block["id"].as<block_id_type>();
         }
      } EOS_RETHROW_EXCEPTIONS(invalid_ref_block_exception, "Invalid reference block num or id: ${block_num_or_id}", ("block_num_or_id", tx_ref_block_num_or_id));
      trx.set_reference_block(ref_block_id);

      if (tx_force_unique) {
         trx.context_free_actions.emplace_back( generate_nonce_action() );
      }

      trx.max_cpu_usage_ms = tx_max_cpu_usage;
      trx.max_net_usage_words = (tx_max_net_usage + 7)/8;
      trx.delay_sec = delaysec;
   }

   if (!tx_skip_sign) {
      fc::variant required_keys;
      if (signing_keys.size() > 0) {
         required_keys = fc::variant(signing_keys);
      }
      else {
         required_keys = determine_required_keys(trx);
      }
      sign_transaction(trx, required_keys, info.chain_id);
   }

   packed_transaction::compression_type compression = to_compression_type( tx_compression );
   if (!tx_dont_broadcast) {
      if (tx_use_old_rpc) {
         return call(push_txn_func, packed_transaction_v0(trx, compression));
      } else {
         if( !amqp_address.empty() ) {
            fc::variant result;
            eosio::transaction_msg msg{packed_transaction( std::move( trx ), true, compression )};
            auto buf = fc::raw::pack( msg );
            const auto& tid = msg.get<packed_transaction>().id();
            string id = tid.str();
            eosio::amqp qp_trx( amqp_address, "trx", []( const std::string& err ) {
               std::cerr << "AMQP trx error: " << err << std::endl;
               exit( 1 );
            } );
            result = fc::mutable_variant_object()
                  ( "transaction_id", id )
                  ( "status", "submitted" );
            qp_trx.publish( "", std::move( id ), std::move( buf ) );
            return result;
         } else {
            try {
               return call( send_txn_func, packed_transaction_v0( trx, compression ) );
            } catch( chain::missing_chain_api_plugin_exception& ) {
               std::cerr << "New RPC send_transaction may not be supported. "
                            "Add flag --use-old-rpc to use old RPC push_transaction instead." << std::endl;
               throw;
            }
         }
      }
   } else {
      if (!tx_return_packed) {
         try {
            fc::variant unpacked_data_trx;
            abi_serializer::to_variant(trx, unpacked_data_trx, abi_serializer_resolver, abi_serializer::create_yield_function( abi_serializer_max_time ));
            return unpacked_data_trx;
         } catch (...) {
            return fc::variant(trx);
         }
      } else {
        return fc::variant(packed_transaction_v0(trx, compression));
      }
   }
}

fc::variant push_actions(std::vector<chain::action>&& actions, const std::vector<public_key_type>& signing_keys = std::vector<public_key_type>() ) {
   signed_transaction trx;
   trx.actions = std::forward<decltype(actions)>(actions);

   return push_transaction(trx, signing_keys);
}

void print_action( const fc::variant& at ) {
   auto receiver = at["receiver"].as_string();
   const auto& act = at["act"].get_object();
   auto code = act["account"].as_string();
   auto func = act["name"].as_string();
   auto args = fc::json::to_string( act["data"], fc::time_point::maximum() );
   auto console = at["console"].as_string();

   /*
   if( code == "eosio" && func == "setcode" )
      args = args.substr(40)+"...";
   if( name(code) == config::system_account_name && func == "setabi" )
      args = args.substr(40)+"...";
   */
   if( args.size() > 100 ) args = args.substr(0,100) + "...";
   cout << "#" << std::setw(14) << right << receiver << " <= " << std::setw(28) << std::left << (code +"::" + func) << " " << args << "\n";
   if( console.size() ) {
      std::stringstream ss(console);
      string line;
      while( std::getline( ss, line ) ) {
         cout << ">> " << line << "\n";
         if( !verbose ) break;
      }
   }
}

bytes variant_to_bin( const account_name& account, const action_name& action, const fc::variant& action_args_var ) {
   auto abis = abi_serializer_resolver( account );
   FC_ASSERT( abis.valid(), "No ABI found for ${contract}", ("contract", account));

   auto action_type = abis->get_action_type( action );
   FC_ASSERT( !action_type.empty(), "Unknown action ${action} in contract ${contract}", ("action", action)( "contract", account ));
   return abis->variant_to_binary( action_type, action_args_var, abi_serializer::create_yield_function( abi_serializer_max_time ) );
}

fc::variant bin_to_variant( const account_name& account, const action_name& action, const bytes& action_args) {
   auto abis = abi_serializer_resolver( account );
   FC_ASSERT( abis.valid(), "No ABI found for ${contract}", ("contract", account));

   auto action_type = abis->get_action_type( action );
   FC_ASSERT( !action_type.empty(), "Unknown action ${action} in contract ${contract}", ("action", action)( "contract", account ));
   return abis->binary_to_variant( action_type, action_args, abi_serializer::create_yield_function( abi_serializer_max_time ) );
}

fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::parse_type::legacy_parser)
{
   regex r("^[ \t]*[\{\[]");
   if ( !regex_search(file_or_str, r) && fc::is_regular_file(file_or_str) ) {
      try {
         return fc::json::from_file(file_or_str, ptype);
      } EOS_RETHROW_EXCEPTIONS(json_parse_exception, "Fail to parse JSON from file: ${file}", ("file", file_or_str));

   } else {
      try {
         return fc::json::from_string(file_or_str, ptype);
      } EOS_RETHROW_EXCEPTIONS(json_parse_exception, "Fail to parse JSON from string: ${string}", ("string", file_or_str));
   }
}

bytes json_or_file_to_bin( const account_name& account, const action_name& action, const string& data_or_filename ) {
   fc::variant action_args_var;
   if( !data_or_filename.empty() ) {
      action_args_var = json_from_file_or_string(data_or_filename, fc::json::parse_type::relaxed_parser);
   }
   return variant_to_bin( account, action, action_args_var );
}

void print_action_tree( const fc::variant& action ) {
   print_action( action );
   if( action.get_object().contains( "inline_traces" ) ) {
      const auto& inline_traces = action["inline_traces"].get_array();
      for( const auto& t : inline_traces ) {
         print_action_tree( t );
      }
   }
}

void print_result( const fc::variant& result ) { try {
      if (result.is_object() && result.get_object().contains("processed")) {
         const auto& processed = result["processed"];
         const auto& transaction_id = processed["id"].as_string();
         string status = "failed";
         int64_t net = -1;
         int64_t cpu = -1;
         if( processed.get_object().contains( "receipt" )) {
            const auto& receipt = processed["receipt"];
            if( receipt.is_object()) {
               status = receipt["status"].as_string();
               net = receipt["net_usage_words"].as_int64() * 8;
               cpu = receipt["cpu_usage_us"].as_int64();
            }
         }

         cerr << status << " transaction: " << transaction_id << "  ";
         if( net < 0 ) {
            cerr << "<unknown>";
         } else {
            cerr << net;
         }
         cerr << " bytes  ";
         if( cpu < 0 ) {
            cerr << "<unknown>";
         } else {
            cerr << cpu;
         }

         cerr << " us\n";

         if( status == "failed" ) {
            auto soft_except = processed["except"].as<fc::optional<fc::exception>>();
            if( soft_except ) {
               edump((soft_except->to_detail_string()));
            }
         } else {
            const auto& actions = processed["action_traces"].get_array();
            for( const auto& a : actions ) {
               print_action_tree( a );
            }
            wlog( "\rwarning: transaction executed locally, but may not be confirmed by the network yet" );
         }
      } else {
         cerr << fc::json::to_pretty_string( result ) << endl;
      }
} FC_CAPTURE_AND_RETHROW( (result) ) }

using std::cout;
void send_actions(std::vector<chain::action>&& actions, const std::vector<public_key_type>& signing_keys = std::vector<public_key_type>() ) {
   std::ofstream out;
   if (tx_json_save_file.length()) {
      out.open(tx_json_save_file);
      EOSC_ASSERT(!out.fail(), "ERROR: Failed to create file \"${p}\"", ("p", tx_json_save_file));
   }
   auto result = push_actions( move(actions), signing_keys);

   string jsonstr;
   if (tx_json_save_file.length()) {
      jsonstr = fc::json::to_pretty_string( result );
      out << jsonstr;
      out.close();
   }
   if( tx_print_json ) {
      if (jsonstr.length() == 0) {
         jsonstr = fc::json::to_pretty_string( result );
      }
      cout << jsonstr << endl;
   } else {
      print_result( result );
   }
}

chain::permission_level to_permission_level(const std::string& s) {
   auto at_pos = s.find('@');
   return permission_level { name(s.substr(0, at_pos)), name(s.substr(at_pos + 1)) };
}

chain::action create_newaccount(const name& creator, const name& newaccount, authority owner, authority active) {
   return action {
      get_account_permissions(tx_permission, {creator,config::active_name}),
      eosio::chain::newaccount{
         .creator      = creator,
         .name         = newaccount,
         .owner        = owner,
         .active       = active
      }
   };
}

chain::action create_action(const vector<permission_level>& authorization, const account_name& code, const action_name& act, const fc::variant& args) {
   return chain::action{authorization, code, act, variant_to_bin(code, act, args)};
}

chain::action create_buyram(const name& creator, const name& newaccount, const asset& quantity) {
   fc::variant act_payload = fc::mutable_variant_object()
         ("payer", creator.to_string())
         ("receiver", newaccount.to_string())
         ("quant", quantity.to_string());
   return create_action(get_account_permissions(tx_permission, {creator,config::active_name}),
                        config::system_account_name, N(buyram), act_payload);
}

chain::action create_buyrambytes(const name& creator, const name& newaccount, uint32_t numbytes) {
   fc::variant act_payload = fc::mutable_variant_object()
         ("payer", creator.to_string())
         ("receiver", newaccount.to_string())
         ("bytes", numbytes);
   return create_action(get_account_permissions(tx_permission, {creator,config::active_name}),
                        config::system_account_name, N(buyrambytes), act_payload);
}

chain::action create_delegate(const name& from, const name& receiver, const asset& net, const asset& cpu, bool transfer) {
   fc::variant act_payload = fc::mutable_variant_object()
         ("from", from.to_string())
         ("receiver", receiver.to_string())
         ("stake_net_quantity", net.to_string())
         ("stake_cpu_quantity", cpu.to_string())
         ("transfer", transfer);
   return create_action(get_account_permissions(tx_permission, {from,config::active_name}),
                        config::system_account_name, N(delegatebw), act_payload);
}

fc::variant regproducer_variant(const account_name& producer, const public_key_type& key, const string& url, uint16_t location) {
   return fc::mutable_variant_object()
            ("producer", producer)
            ("producer_key", key)
            ("url", url)
            ("location", location)
            ;
}

chain::action create_open(const string& contract, const name& owner, symbol sym, const name& ram_payer) {
   auto open_ = fc::mutable_variant_object
      ("owner", owner)
      ("symbol", sym)
      ("ram_payer", ram_payer);
    return action {
      get_account_permissions(tx_permission, {ram_payer, config::active_name}),
      name(contract), N(open), variant_to_bin( name(contract), N(open), open_ )
   };
}

chain::action create_transfer(const string& contract, const name& sender, const name& recipient, asset amount, const string& memo ) {

   auto transfer = fc::mutable_variant_object
      ("from", sender)
      ("to", recipient)
      ("quantity", amount)
      ("memo", memo);

   return action {
      get_account_permissions(tx_permission, {sender,config::active_name}),
      name(contract), N(transfer), variant_to_bin( name(contract), N(transfer), transfer )
   };
}

chain::action create_setabi(const name& account, const bytes& abi) {
   return action {
      get_account_permissions(tx_permission, {account,config::active_name}),
      setabi{
         .account   = account,
         .abi       = abi
      }
   };
}

chain::action create_setcode(const name& account, const bytes& code) {
   return action {
      get_account_permissions(tx_permission, {account,config::active_name}),
      setcode{
         .account   = account,
         .vmtype    = 0,
         .vmversion = 0,
         .code      = code
      }
   };
}

chain::action create_updateauth(const name& account, const name& permission, const name& parent, const authority& auth) {
   return action { get_account_permissions(tx_permission, {account,config::active_name}),
                   updateauth{account, permission, parent, auth}};
}

chain::action create_deleteauth(const name& account, const name& permission) {
   return action { get_account_permissions(tx_permission, {account,config::active_name}),
                   deleteauth{account, permission}};
}

chain::action create_linkauth(const name& account, const name& code, const name& type, const name& requirement) {
   return action { get_account_permissions(tx_permission, {account,config::active_name}),
                   linkauth{account, code, type, requirement}};
}

chain::action create_unlinkauth(const name& account, const name& code, const name& type) {
   return action { get_account_permissions(tx_permission, {account,config::active_name}),
                   unlinkauth{account, code, type}};
}

authority parse_json_authority(const std::string& authorityJsonOrFile) {
   fc::variant authority_var = json_from_file_or_string(authorityJsonOrFile);
   try {
      return authority_var.as<authority>();
   } EOS_RETHROW_EXCEPTIONS(authority_type_exception, "Invalid authority format '${data}'",
                            ("data", fc::json::to_string(authority_var, fc::time_point::maximum())))
}

authority parse_json_authority_or_key(const std::string& authorityJsonOrFile) {
   if (is_public_key_str(authorityJsonOrFile)) {
      try {
         return authority(public_key_type(authorityJsonOrFile));
      } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", authorityJsonOrFile))
   } else {
      auto result = parse_json_authority(authorityJsonOrFile);
      EOS_ASSERT( eosio::chain::validate(result), authority_type_exception, "Authority failed validation! ensure that keys, accounts, and waits are sorted and that the threshold is valid and satisfiable!");
      return result;
   }
}

asset to_asset( account_name code, const string& s ) {
   static map< pair<account_name, eosio::chain::symbol_code>, eosio::chain::symbol> cache;
   auto a = asset::from_string( s );
   eosio::chain::symbol_code sym = a.get_symbol().to_symbol_code();
   auto it = cache.find( make_pair(code, sym) );
   auto sym_str = a.symbol_name();
   if ( it == cache.end() ) {
      auto json = call(get_currency_stats_func, fc::mutable_variant_object("json", false)
                       ("code", code)
                       ("symbol", sym_str)
      );
      auto obj = json.get_object();
      auto obj_it = obj.find( sym_str );
      if (obj_it != obj.end()) {
         auto result = obj_it->value().as<eosio::chain_apis::read_only::get_currency_stats_result>();
         auto p = cache.emplace( make_pair( code, sym ), result.max_supply.get_symbol() );
         it = p.first;
      } else {
         EOS_THROW(symbol_type_exception, "Symbol ${s} is not supported by token contract ${c}", ("s", sym_str)("c", code));
      }
   }
   auto expected_symbol = it->second;
   if ( a.decimals() < expected_symbol.decimals() ) {
      auto factor = expected_symbol.precision() / a.precision();
      a = asset( a.get_amount() * factor, expected_symbol );
   } else if ( a.decimals() > expected_symbol.decimals() ) {
      EOS_THROW(symbol_type_exception, "Too many decimal digits in ${a}, only ${d} supported", ("a", a)("d", expected_symbol.decimals()));
   } // else precision matches
   return a;
}

inline asset to_asset( const string& s ) {
   return to_asset( N(eosio.token), s );
}

struct set_account_permission_subcommand {
   string account;
   string permission;
   string authority_json_or_file;
   string parent;
   bool add_code = false;
   bool remove_code = false;

   set_account_permission_subcommand(CLI::App* accountCmd) {
      auto permissions = accountCmd->add_subcommand("permission", localized("Set parameters dealing with account permissions"));
      permissions->add_option("account", account, localized("The account to set/delete a permission authority for"))->required();
      permissions->add_option("permission", permission, localized("The permission name to set/delete an authority for"))->required();
      permissions->add_option("authority", authority_json_or_file, localized("[delete] NULL, [create/update] public key, JSON string or filename defining the authority, [code] contract name"));
      permissions->add_option("parent", parent, localized("[create] The permission name of this parents permission, defaults to 'active'"));
      permissions->add_flag("--add-code", add_code, localized("[code] add '${code}' permission to specified permission authority", ("code", name(config::eosio_code_name))));
      permissions->add_flag("--remove-code", remove_code, localized("[code] remove '${code}' permission from specified permission authority", ("code", name(config::eosio_code_name))));

      add_standard_transaction_options(permissions, "account@active");

      permissions->callback([this] {
         EOSC_ASSERT( !(add_code && remove_code), "ERROR: Either --add-code or --remove-code can be set" );
         EOSC_ASSERT( (add_code ^ remove_code) || !authority_json_or_file.empty(), "ERROR: authority should be specified unless add or remove code permission" );

         authority auth;

         bool need_parent = parent.empty() && (name(permission) != name("owner"));
         bool need_auth = add_code || remove_code;

         if ( !need_auth && boost::iequals(authority_json_or_file, "null") ) {
            send_actions( { create_deleteauth(name(account), name(permission)) } );
            return;
         }

         if ( need_parent || need_auth ) {
            fc::variant json = call(get_account_func, fc::mutable_variant_object("account_name", account));
            auto res = json.as<eosio::chain_apis::read_only::get_account_results>();
            auto itr = std::find_if(res.permissions.begin(), res.permissions.end(), [&](const auto& perm) {
               return perm.perm_name == name(permission);
            });

            if ( need_parent ) {
               // see if we can auto-determine the proper parent
               if ( itr != res.permissions.end() ) {
                  parent = (*itr).parent.to_string();
               } else {
                  // if this is a new permission and there is no parent we default to "active"
                  parent = config::active_name.to_string();
               }
            }

            if ( need_auth ) {
               auto actor = (authority_json_or_file.empty()) ? name(account) : name(authority_json_or_file);
               auto code_name = config::eosio_code_name;

               if ( itr != res.permissions.end() ) {
                  // fetch existing authority
                  auth = std::move((*itr).required_auth);

                  auto code_perm = permission_level { actor, code_name };
                  auto itr2 = std::lower_bound(auth.accounts.begin(), auth.accounts.end(), code_perm, [&](const auto& perm_level, const auto& value) {
                     return perm_level.permission < value; // Safe since valid authorities must order the permissions in accounts in ascending order
                  });

                  if ( add_code ) {
                     if ( itr2 != auth.accounts.end() && itr2->permission == code_perm ) {
                        // authority already contains code permission, promote its weight to satisfy threshold
                        if ( (*itr2).weight < auth.threshold ) {
                           if ( auth.threshold > std::numeric_limits<weight_type>::max() ) {
                              std::cerr << "ERROR: Threshold is too high to be satisfied by sole code permission" << std::endl;
                              return;
                           }
                           std::cerr << localized("The weight of '${actor}@${code}' in '${permission}' permission authority will be increased up to threshold",
                                                  ("actor", actor)("code", code_name)("permission", permission)) << std::endl;
                           (*itr2).weight = static_cast<weight_type>(auth.threshold);
                        } else {
                           std::cerr << localized("ERROR: The permission '${permission}' already contains '${actor}@${code}'",
                                                  ("permission", permission)("actor", actor)("code", code_name)) << std::endl;
                           return ;
                        }
                     } else {
                        // add code permission to specified authority
                        if ( auth.threshold > std::numeric_limits<weight_type>::max() ) {
                           std::cerr << "ERROR: Threshold is too high to be satisfied by sole code permission" << std::endl;
                           return;
                        }
                        auth.accounts.insert( itr2, permission_level_weight {
                           .permission = { actor, code_name },
                           .weight = static_cast<weight_type>(auth.threshold)
                        });
                     }
                  } else {
                     if ( itr2 != auth.accounts.end() && itr2->permission == code_perm ) {
                        // remove code permission, if authority becomes empty by the removal of code permission, delete permission
                        auth.accounts.erase( itr2 );
                        if ( auth.keys.empty() && auth.accounts.empty() && auth.waits.empty() ) {
                           send_actions( { create_deleteauth(name(account), name(permission)) } );
                           return;
                        }
                     } else {
                        // authority doesn't contain code permission
                        std::cerr << localized("ERROR: '${actor}@${code}' does not exist in '${permission}' permission authority",
                                               ("actor", actor)("code", code_name)("permission", permission)) << std::endl;
                        return;
                     }
                  }
               } else {
                  if ( add_code ) {
                     // create new permission including code permission
                     auth.threshold = 1;
                     auth.accounts.push_back( permission_level_weight {
                        .permission = { actor, code_name },
                        .weight = 1
                     });
                  } else {
                     // specified permission doesn't exist, so failed to remove code permission from it
                     std::cerr << localized("ERROR: The permission '${permission}' does not exist", ("permission", permission)) << std::endl;
                     return;
                  }
               }
            }
         }

         if ( !need_auth ) {
            auth = parse_json_authority_or_key(authority_json_or_file);
         }

         send_actions( { create_updateauth(name(account), name(permission), name(parent), auth) } );
      });
   }
};

struct set_action_permission_subcommand {
   string accountStr;
   string codeStr;
   string typeStr;
   string requirementStr;

   set_action_permission_subcommand(CLI::App* actionRoot) {
      auto permissions = actionRoot->add_subcommand("permission", localized("Set paramaters dealing with account permissions"));
      permissions->add_option("account", accountStr, localized("The account to set/delete a permission authority for"))->required();
      permissions->add_option("code", codeStr, localized("The account that owns the code for the action"))->required();
      permissions->add_option("type", typeStr, localized("The type of the action"))->required();
      permissions->add_option("requirement", requirementStr, localized("[delete] NULL, [set/update] The permission name require for executing the given action"))->required();

      add_standard_transaction_options_plus_signing(permissions, "account@active");

      permissions->callback([this] {
         name account = name(accountStr);
         name code = name(codeStr);
         name type = name(typeStr);
         bool is_delete = boost::iequals(requirementStr, "null");

         if (is_delete) {
            send_actions({create_unlinkauth(account, code, type)}, signing_keys_opt.get_keys());
         } else {
            name requirement = name(requirementStr);
            send_actions({create_linkauth(account, code, type, requirement)}, signing_keys_opt.get_keys());
         }
      });
   }
};


bool local_port_used() {
    using namespace boost::asio;

    io_service ios;
    local::stream_protocol::endpoint endpoint(wallet_url.substr(strlen("unix://")));
    local::stream_protocol::socket socket(ios);
    boost::system::error_code ec;
    socket.connect(endpoint, ec);

    return !ec;
}

void try_local_port(uint32_t duration) {
   using namespace std::chrono;
   auto start_time = duration_cast<std::chrono::milliseconds>( system_clock::now().time_since_epoch() ).count();
   while ( !local_port_used()) {
      if (duration_cast<std::chrono::milliseconds>( system_clock::now().time_since_epoch()).count() - start_time > duration ) {
         std::cerr << "Unable to connect to " << key_store_executable_name << ", if " << key_store_executable_name << " is running please kill the process and try again.\n";
         throw connection_exception(fc::log_messages{FC_LOG_MESSAGE(error, "Unable to connect to ${k}", ("k", key_store_executable_name))});
      }
   }
}

void ensure_keosd_running(CLI::App* app) {
    if (no_auto_keosd)
        return;
    // get, version, net, convert do not require keosd
    if (tx_skip_sign || app->got_subcommand("get") || app->got_subcommand("version") || app->got_subcommand("net") || app->got_subcommand("convert"))
        return;
    if (app->get_subcommand("create")->got_subcommand("key")) // create key does not require wallet
       return;
    if (app->get_subcommand("multisig")->got_subcommand("review")) // multisig review does not require wallet
       return;
    if (auto* subapp = app->get_subcommand("system")) {
       if (subapp->got_subcommand("listproducers") || subapp->got_subcommand("listbw") || subapp->got_subcommand("bidnameinfo")) // system list* do not require wallet
         return;
    }
    if (wallet_url != default_wallet_url)
      return;

    if (local_port_used())
       return;

    boost::filesystem::path binPath = boost::dll::program_location();
    binPath.remove_filename();
    // This extra check is necessary when running cleos like this: ./cleos ...
    if (binPath.filename_is_dot())
        binPath.remove_filename();
    binPath.append(key_store_executable_name); // if cleos and keosd are in the same installation directory
    if (!boost::filesystem::exists(binPath)) {
        binPath.remove_filename().remove_filename().append("keosd").append(key_store_executable_name);
    }

    if (boost::filesystem::exists(binPath)) {
        namespace bp = boost::process;
        binPath = boost::filesystem::canonical(binPath);

        vector<std::string> pargs;
        pargs.push_back("--http-server-address");
        pargs.push_back("");
        pargs.push_back("--https-server-address");
        pargs.push_back("");
        pargs.push_back("--unix-socket-path");
        pargs.push_back(string(key_store_executable_name) + ".sock");

        ::boost::process::child keos(binPath, pargs,
                                     bp::std_in.close(),
                                     bp::std_out > bp::null,
                                     bp::std_err > bp::null);
        if (keos.running()) {
            std::cerr << binPath << " launched" << std::endl;
            keos.detach();
            try_local_port(2000);
        } else {
            std::cerr << "No wallet service listening on " << wallet_url << ". Failed to launch " << binPath << std::endl;
        }
    } else {
        std::cerr << "No wallet service listening on "
                  << ". Cannot automatically start " << key_store_executable_name << " because " << key_store_executable_name << " was not found." << std::endl;
    }
}


CLI::callback_t obsoleted_option_host_port = [](CLI::results_t) {
   std::cerr << localized("Host and port options (-H, --wallet-host, etc.) have been replaced with -u/--url and --wallet-url\n"
                          "Use for example -u http://localhost:8888 or --url https://example.invalid/\n");
   exit(1);
   return false;
};

struct register_producer_subcommand {
   string producer_str;
   string producer_key_str;
   string url;
   uint16_t loc = 0;

   register_producer_subcommand(CLI::App* actionRoot) {
      auto register_producer = actionRoot->add_subcommand("regproducer", localized("Register a new producer"));
      register_producer->add_option("account", producer_str, localized("The account to register as a producer"))->required();
      register_producer->add_option("producer_key", producer_key_str, localized("The producer's public key"))->required();
      register_producer->add_option("url", url, localized("The URL where info about producer can be found"), true);
      register_producer->add_option("location", loc, localized("Relative location for purpose of nearest neighbor scheduling"), true);
      add_standard_transaction_options_plus_signing(register_producer, "account@active");


      register_producer->callback([this] {
         public_key_type producer_key;
         try {
            producer_key = public_key_type(producer_key_str);
         } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid producer public key: ${public_key}", ("public_key", producer_key_str))

         auto regprod_var = regproducer_variant(name(producer_str), producer_key, url, loc );
         auto accountPermissions = get_account_permissions(tx_permission, {name(producer_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(regproducer), regprod_var)}, signing_keys_opt.get_keys());
      });
   }
};

struct create_account_subcommand {
   string creator;
   string account_name;
   string owner_key_str;
   string active_key_str;
   string stake_net;
   string stake_cpu;
   uint32_t buy_ram_bytes_in_kbytes = 0;
   uint32_t buy_ram_bytes = 0;
   string buy_ram_eos;
   bool transfer = false;
   bool simple = false;

   create_account_subcommand(CLI::App* actionRoot, bool s) : simple(s) {
      auto createAccount = actionRoot->add_subcommand(
                              (simple ? "account" : "newaccount"),
                              (simple ? localized("Create a new account on the blockchain (assumes system contract does not restrict RAM usage)")
                                      : localized("Create a new account on the blockchain with initial resources") )
      );
      createAccount->add_option("creator", creator, localized("The name of the account creating the new account"))->required();
      createAccount->add_option("name", account_name, localized("The name of the new account"))->required();
      createAccount->add_option("OwnerKey", owner_key_str, localized("The owner public key, permission level, or authority for the new account"))->required();
      createAccount->add_option("ActiveKey", active_key_str, localized("The active public key, permission level, or authority for the new account"));

      if (!simple) {
         createAccount->add_option("--stake-net", stake_net,
                                   (localized("The amount of tokens delegated for net bandwidth")))->required();
         createAccount->add_option("--stake-cpu", stake_cpu,
                                   (localized("The amount of tokens delegated for CPU bandwidth")))->required();
         createAccount->add_option("--buy-ram-kbytes", buy_ram_bytes_in_kbytes,
                                   (localized("The amount of RAM bytes to purchase for the new account in kibibytes (KiB)")));
         createAccount->add_option("--buy-ram-bytes", buy_ram_bytes,
                                   (localized("The amount of RAM bytes to purchase for the new account in bytes")));
         createAccount->add_option("--buy-ram", buy_ram_eos,
                                   (localized("The amount of RAM bytes to purchase for the new account in tokens")));
         createAccount->add_flag("--transfer", transfer,
                                 (localized("Transfer voting power and right to unstake tokens to receiver")));
      }

      add_standard_transaction_options_plus_signing(createAccount, "creator@active");

      createAccount->callback([this] {
            authority owner, active;
            if( owner_key_str.find('{') != string::npos ) {
               try{
                  owner = parse_json_authority_or_key(owner_key_str);
               } EOS_RETHROW_EXCEPTIONS( explained_exception, "Invalid owner authority: ${authority}", ("authority", owner_key_str) )
            } else if( owner_key_str.find('@') != string::npos ) {
               try {
                  owner = authority(to_permission_level(owner_key_str));
               } EOS_RETHROW_EXCEPTIONS( explained_exception, "Invalid owner permission level: ${permission}", ("permission", owner_key_str) )
            } else {
               try {
                  owner = authority(public_key_type(owner_key_str));
               } EOS_RETHROW_EXCEPTIONS( public_key_type_exception, "Invalid owner public key: ${public_key}", ("public_key", owner_key_str) );
            }

            if( active_key_str.empty() ) {
               active = owner;
            } else if ( active_key_str.find('{') != string::npos ) { 
               try{
                  active = parse_json_authority_or_key(active_key_str);
               } EOS_RETHROW_EXCEPTIONS( explained_exception, "Invalid active authority: ${authority}", ("authority", owner_key_str) )
            }else if( active_key_str.find('@') != string::npos ) {
               try {
                  active = authority(to_permission_level(active_key_str));
               } EOS_RETHROW_EXCEPTIONS( explained_exception, "Invalid active permission level: ${permission}", ("permission", active_key_str) )
            } else {
               try {
                  active = authority(public_key_type(active_key_str));
               } EOS_RETHROW_EXCEPTIONS( public_key_type_exception, "Invalid active public key: ${public_key}", ("public_key", active_key_str) );
            }

            auto create = create_newaccount(name(creator), name(account_name), owner, active);
            if (!simple) {
               EOSC_ASSERT( buy_ram_eos.size() || buy_ram_bytes_in_kbytes || buy_ram_bytes, "ERROR: One of --buy-ram, --buy-ram-kbytes or --buy-ram-bytes should have non-zero value" );
               EOSC_ASSERT( !buy_ram_bytes_in_kbytes || !buy_ram_bytes, "ERROR: --buy-ram-kbytes and --buy-ram-bytes cannot be set at the same time" );
               action buyram = !buy_ram_eos.empty() ? create_buyram(name(creator), name(account_name), to_asset(buy_ram_eos))
                  : create_buyrambytes(name(creator), name(account_name), (buy_ram_bytes_in_kbytes) ? (buy_ram_bytes_in_kbytes * 1024) : buy_ram_bytes);
               auto net = to_asset(stake_net);
               auto cpu = to_asset(stake_cpu);
               if ( net.get_amount() != 0 || cpu.get_amount() != 0 ) {
                  action delegate = create_delegate( name(creator), name(account_name), net, cpu, transfer);
                  send_actions( { create, buyram, delegate } );
               } else {
                  send_actions( { create, buyram } );
               }
            } else {
               send_actions( { create } );
            }
      });
   }
};

struct unregister_producer_subcommand {
   string producer_str;

   unregister_producer_subcommand(CLI::App* actionRoot) {
      auto unregister_producer = actionRoot->add_subcommand("unregprod", localized("Unregister an existing producer"));
      unregister_producer->add_option("account", producer_str, localized("The account to unregister as a producer"))->required();
      add_standard_transaction_options_plus_signing(unregister_producer, "account@active");

      unregister_producer->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("producer", producer_str);

         auto accountPermissions = get_account_permissions(tx_permission, {name(producer_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(unregprod), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct vote_producer_proxy_subcommand {
   string voter_str;
   string proxy_str;

   vote_producer_proxy_subcommand(CLI::App* actionRoot) {
      auto vote_proxy = actionRoot->add_subcommand("proxy", localized("Vote your stake through a proxy"));
      vote_proxy->add_option("voter", voter_str, localized("The voting account"))->required();
      vote_proxy->add_option("proxy", proxy_str, localized("The proxy account"))->required();
      add_standard_transaction_options_plus_signing(vote_proxy, "voter@active");

      vote_proxy->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("voter", voter_str)
                  ("proxy", proxy_str)
                  ("producers", std::vector<account_name>{});
         auto accountPermissions = get_account_permissions(tx_permission, {name(voter_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(voteproducer), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct vote_producers_subcommand {
   string voter_str;
   vector<std::string> producer_names;

   vote_producers_subcommand(CLI::App* actionRoot) {
      auto vote_producers = actionRoot->add_subcommand("prods", localized("Vote for one or more producers"));
      vote_producers->add_option("voter", voter_str, localized("The voting account"))->required();
      vote_producers->add_option("producers", producer_names, localized("The account(s) to vote for. All options from this position and following will be treated as the producer list."))->required();
      add_standard_transaction_options_plus_signing(vote_producers, "voter@active");

      vote_producers->callback([this] {

         std::sort( producer_names.begin(), producer_names.end() );

         fc::variant act_payload = fc::mutable_variant_object()
                  ("voter", voter_str)
                  ("proxy", "")
                  ("producers", producer_names);
         auto accountPermissions = get_account_permissions(tx_permission, {name(voter_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(voteproducer), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct approve_producer_subcommand {
   string voter;
   string producer_name;

   approve_producer_subcommand(CLI::App* actionRoot) {
      auto approve_producer = actionRoot->add_subcommand("approve", localized("Add one producer to list of voted producers"));
      approve_producer->add_option("voter", voter, localized("The voting account"))->required();
      approve_producer->add_option("producer", producer_name, localized("The account to vote for"))->required();
      add_standard_transaction_options_plus_signing(approve_producer, "voter@active");

      approve_producer->callback([this] {
            auto result = call(get_table_func, fc::mutable_variant_object("json", true)
                               ("code", name(config::system_account_name).to_string())
                               ("scope", name(config::system_account_name).to_string())
                               ("table", "voters")
                               ("table_key", "owner")
                               ("lower_bound", name(voter).to_uint64_t())
                               ("upper_bound", name(voter).to_uint64_t() + 1)
                               // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                               // Change to voter.value when cleos no longer needs to support nodeos versions older than 1.5.0
                               ("limit", 1)
            );
            auto res = result.as<eosio::chain_apis::read_only::get_table_rows_result>();
            // Condition in if statement below can simply be res.rows.empty() when cleos no longer needs to support nodeos versions older than 1.5.0
            // Although since this subcommand will actually change the voter's vote, it is probably better to just keep this check to protect
            //  against future potential chain_plugin bugs.
            if( res.rows.empty() || res.rows[0].get_object()["owner"].as_string() != name(voter).to_string() ) {
               std::cerr << "Voter info not found for account " << voter << std::endl;
               return;
            }
            EOS_ASSERT( 1 == res.rows.size(), multiple_voter_info, "More than one voter_info for account" );
            auto prod_vars = res.rows[0]["producers"].get_array();
            vector<eosio::name> prods;
            for ( auto& x : prod_vars ) {
               prods.push_back( name(x.as_string()) );
            }
            prods.push_back( name(producer_name) );
            std::sort( prods.begin(), prods.end() );
            auto it = std::unique( prods.begin(), prods.end() );
            if (it != prods.end() ) {
               std::cerr << "Producer \"" << producer_name << "\" is already on the list." << std::endl;
               return;
            }
            fc::variant act_payload = fc::mutable_variant_object()
               ("voter", voter)
               ("proxy", "")
               ("producers", prods);
            auto accountPermissions = get_account_permissions(tx_permission, {name(voter), config::active_name});
            send_actions({create_action(accountPermissions, config::system_account_name, N(voteproducer), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct unapprove_producer_subcommand {
   string voter;
   string producer_name;

   unapprove_producer_subcommand(CLI::App* actionRoot) {
      auto approve_producer = actionRoot->add_subcommand("unapprove", localized("Remove one producer from list of voted producers"));
      approve_producer->add_option("voter", voter, localized("The voting account"))->required();
      approve_producer->add_option("producer", producer_name, localized("The account to remove from voted producers"))->required();
      add_standard_transaction_options_plus_signing(approve_producer, "voter@active");

      approve_producer->callback([this] {
            auto result = call(get_table_func, fc::mutable_variant_object("json", true)
                               ("code", name(config::system_account_name).to_string())
                               ("scope", name(config::system_account_name).to_string())
                               ("table", "voters")
                               ("table_key", "owner")
                               ("lower_bound", name(voter).to_uint64_t())
                               ("upper_bound", name(voter).to_uint64_t() + 1)
                               // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                               // Change to voter.value when cleos no longer needs to support nodeos versions older than 1.5.0
                               ("limit", 1)
            );
            auto res = result.as<eosio::chain_apis::read_only::get_table_rows_result>();
            // Condition in if statement below can simply be res.rows.empty() when cleos no longer needs to support nodeos versions older than 1.5.0
            // Although since this subcommand will actually change the voter's vote, it is probably better to just keep this check to protect
            //  against future potential chain_plugin bugs.
            if( res.rows.empty() || res.rows[0].get_object()["owner"].as_string() != name(voter).to_string() ) {
               std::cerr << "Voter info not found for account " << voter << std::endl;
               return;
            }
            EOS_ASSERT( 1 == res.rows.size(), multiple_voter_info, "More than one voter_info for account" );
            auto prod_vars = res.rows[0]["producers"].get_array();
            vector<eosio::name> prods;
            for ( auto& x : prod_vars ) {
               prods.push_back( name(x.as_string()) );
            }
            auto it = std::remove( prods.begin(), prods.end(), name(producer_name) );
            if (it == prods.end() ) {
               std::cerr << "Cannot remove: producer \"" << producer_name << "\" is not on the list." << std::endl;
               return;
            }
            prods.erase( it, prods.end() ); //should always delete only one element
            fc::variant act_payload = fc::mutable_variant_object()
               ("voter", voter)
               ("proxy", "")
               ("producers", prods);
            auto accountPermissions = get_account_permissions(tx_permission, {name(voter), config::active_name});
            send_actions({create_action(accountPermissions, config::system_account_name, N(voteproducer), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct list_producers_subcommand {
   bool print_json = false;
   uint32_t limit = 50;
   std::string lower;

   list_producers_subcommand(CLI::App* actionRoot) {
      auto list_producers = actionRoot->add_subcommand("listproducers", localized("List producers"));
      list_producers->add_flag("--json,-j", print_json, localized("Output in JSON format"));
      list_producers->add_option("-l,--limit", limit, localized("The maximum number of rows to return"));
      list_producers->add_option("-L,--lower", lower, localized("Lower bound value of key, defaults to first"));
      list_producers->callback([this] {
         auto rawResult = call(get_producers_func, fc::mutable_variant_object
            ("json", true)("lower_bound", lower)("limit", limit));
         if ( print_json ) {
            std::cout << fc::json::to_pretty_string(rawResult) << std::endl;
            return;
         }
         auto result = rawResult.as<eosio::chain_apis::read_only::get_producers_result>();
         if ( result.rows.empty() ) {
            std::cout << "No producers found" << std::endl;
            return;
         }
         auto weight = result.total_producer_vote_weight;
         if ( !weight )
            weight = 1;
         printf("%-13s %-57s %-59s %s\n", "Producer", "Producer key", "Url", "Scaled votes");
         for ( auto& row : result.rows )
            printf("%-13.13s %-57.57s %-59.59s %1.4f\n",
                   row["owner"].as_string().c_str(),
                   row["producer_key"].as_string().c_str(),
                   row["url"].as_string().c_str(),
                   row["total_votes"].as_double() / weight);
         if ( !result.more.empty() )
            std::cout << "-L " << result.more << " for more" << std::endl;
      });
   }
};

struct get_schedule_subcommand {
   bool print_json = false;

   void print(const char* name, const fc::variant& schedule) {
      if (schedule.is_null()) {
         printf("%s schedule empty\n\n", name);
         return;
      }
      printf("%s schedule version %s\n", name, schedule["version"].as_string().c_str());
      printf("    %-13s %s\n", "Producer", "Producer Authority");
      printf("    %-13s %s\n", "=============", "==================");
      for( auto& row: schedule["producers"].get_array() ) {
         if( row.get_object().contains("block_signing_key") ) {
            // pre 2.0
            printf( "    %-13s %s\n", row["producer_name"].as_string().c_str(), row["block_signing_key"].as_string().c_str() );
         } else {
            printf( "    %-13s ", row["producer_name"].as_string().c_str() );
            auto a = row["authority"].as<block_signing_authority>();
            static_assert( std::is_same<decltype(a), static_variant<block_signing_authority_v0>>::value,
                           "Updates maybe needed if block_signing_authority changes" );
            block_signing_authority_v0 auth = a.get<block_signing_authority_v0>();
            printf( "%s\n", fc::json::to_string( auth, fc::time_point::maximum() ).c_str() );
         }
      }
      printf("\n");
   }

   get_schedule_subcommand(CLI::App* actionRoot) {
      auto get_schedule = actionRoot->add_subcommand("schedule", localized("Retrieve the producer schedule"));
      get_schedule->add_flag("--json,-j", print_json, localized("Output in JSON format"));
      get_schedule->callback([this] {
         auto result = call(get_schedule_func, fc::mutable_variant_object());
         if ( print_json ) {
            std::cout << fc::json::to_pretty_string(result) << std::endl;
            return;
         }
         print("active", result["active"]);
         print("pending", result["pending"]);
         print("proposed", result["proposed"]);
      });
   }
};

struct get_transaction_id_subcommand {
   string trx_to_check;

   get_transaction_id_subcommand(CLI::App* actionRoot) {
      auto get_transaction_id = actionRoot->add_subcommand("transaction_id", localized("Get transaction id given transaction object"));
      get_transaction_id->add_option("transaction", trx_to_check, localized("The JSON string or filename defining the transaction which transaction id we want to retrieve"))->required();

      get_transaction_id->callback([&] {
         try {
            fc::variant trx_var = json_from_file_or_string(trx_to_check);
            if( trx_var.is_object() ) {
               fc::variant_object& vo = trx_var.get_object();
               // if actions.data & actions.hex_data provided, use the hex_data since only currently support unexploded data
               if( vo.contains("actions") ) {
                  if( vo["actions"].is_array() ) {
                     fc::mutable_variant_object mvo = vo;
                     fc::variants& action_variants = mvo["actions"].get_array();
                     for( auto& action_v : action_variants ) {
                        if( !action_v.is_object() ) {
                           std::cerr << "Empty 'action' in transaction" << endl;
                           return;
                        }
                        fc::variant_object& action_vo = action_v.get_object();
                        if( action_vo.contains( "data" ) && action_vo.contains( "hex_data" ) ) {
                           fc::mutable_variant_object maction_vo = action_vo;
                           maction_vo["data"] = maction_vo["hex_data"];
                           action_vo = maction_vo;
                           vo = mvo;
                        } else if( action_vo.contains( "data" ) ) {
                           if( !action_vo["data"].is_string() ) {
                              std::cerr << "get transaction_id only supports un-exploded 'data' (hex form)" << std::endl;
                              return;
                           }
                        }
                     }
                  } else {
                     std::cerr << "transaction json 'actions' is not an array" << std::endl;
                     return;
                  }
               } else {
                  std::cerr << "transaction json does not include 'actions'" << std::endl;
                  return;
               }
               auto trx = trx_var.as<transaction>();
               transaction_id_type id = trx.id();
               if( id == transaction().id() ) {
                  std::cerr << "file/string does not represent a transaction" << std::endl;
               } else {
                  std::cout << string( id ) << std::endl;
               }
            } else {
               std::cerr << "file/string does not represent a transaction" << std::endl;
            }
         } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON '${data}'", ("data",trx_to_check))
      });
   }
};

struct delegate_bandwidth_subcommand {
   string from_str;
   string receiver_str;
   string stake_net_amount;
   string stake_cpu_amount;
   string stake_storage_amount;
   string buy_ram_amount;
   uint32_t buy_ram_bytes = 0;
   bool transfer = false;

   delegate_bandwidth_subcommand(CLI::App* actionRoot) {
      auto delegate_bandwidth = actionRoot->add_subcommand("delegatebw", localized("Delegate bandwidth"));
      delegate_bandwidth->add_option("from", from_str, localized("The account to delegate bandwidth from"))->required();
      delegate_bandwidth->add_option("receiver", receiver_str, localized("The account to receive the delegated bandwidth"))->required();
      delegate_bandwidth->add_option("stake_net_quantity", stake_net_amount, localized("The amount of tokens to stake for network bandwidth"))->required();
      delegate_bandwidth->add_option("stake_cpu_quantity", stake_cpu_amount, localized("The amount of tokens to stake for CPU bandwidth"))->required();
      delegate_bandwidth->add_option("--buyram", buy_ram_amount, localized("The amount of tokens to buy RAM with"));
      delegate_bandwidth->add_option("--buy-ram-bytes", buy_ram_bytes, localized("The amount of RAM to buy in bytes"));
      delegate_bandwidth->add_flag("--transfer", transfer, localized("Transfer voting power and right to unstake tokens to receiver"));
      add_standard_transaction_options_plus_signing(delegate_bandwidth, "from@active");

      delegate_bandwidth->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("from", from_str)
                  ("receiver", receiver_str)
                  ("stake_net_quantity", to_asset(stake_net_amount))
                  ("stake_cpu_quantity", to_asset(stake_cpu_amount))
                  ("transfer", transfer);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         std::vector<chain::action> acts{create_action(accountPermissions, config::system_account_name, N(delegatebw), act_payload)};
         EOSC_ASSERT( !(buy_ram_amount.size()) || !buy_ram_bytes, "ERROR: --buyram and --buy-ram-bytes cannot be set at the same time" );
         if (buy_ram_amount.size()) {
            acts.push_back( create_buyram(name(from_str), name(receiver_str), to_asset(buy_ram_amount)) );
         } else if (buy_ram_bytes) {
            acts.push_back( create_buyrambytes(name(from_str), name(receiver_str), buy_ram_bytes) );
         }
         send_actions(std::move(acts), signing_keys_opt.get_keys());
      });
   }
};

struct undelegate_bandwidth_subcommand {
   string from_str;
   string receiver_str;
   string unstake_net_amount;
   string unstake_cpu_amount;
   uint64_t unstake_storage_bytes;

   undelegate_bandwidth_subcommand(CLI::App* actionRoot) {
      auto undelegate_bandwidth = actionRoot->add_subcommand("undelegatebw", localized("Undelegate bandwidth"));
      undelegate_bandwidth->add_option("from", from_str, localized("The account undelegating bandwidth"))->required();
      undelegate_bandwidth->add_option("receiver", receiver_str, localized("The account to undelegate bandwidth from"))->required();
      undelegate_bandwidth->add_option("unstake_net_quantity", unstake_net_amount, localized("The amount of tokens to undelegate for network bandwidth"))->required();
      undelegate_bandwidth->add_option("unstake_cpu_quantity", unstake_cpu_amount, localized("The amount of tokens to undelegate for CPU bandwidth"))->required();
      add_standard_transaction_options_plus_signing(undelegate_bandwidth, "from@active");

      undelegate_bandwidth->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("from", from_str)
                  ("receiver", receiver_str)
                  ("unstake_net_quantity", to_asset(unstake_net_amount))
                  ("unstake_cpu_quantity", to_asset(unstake_cpu_amount));
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(undelegatebw), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct bidname_subcommand {
   string bidder_str;
   string newname_str;
   string bid_amount;
   bidname_subcommand(CLI::App *actionRoot) {
      auto bidname = actionRoot->add_subcommand("bidname", localized("Name bidding"));
      bidname->add_option("bidder", bidder_str, localized("The bidding account"))->required();
      bidname->add_option("newname", newname_str, localized("The bidding name"))->required();
      bidname->add_option("bid", bid_amount, localized("The amount of tokens to bid"))->required();
      add_standard_transaction_options_plus_signing(bidname, "bidder@active");

      bidname->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("bidder", bidder_str)
                  ("newname", newname_str)
                  ("bid", to_asset(bid_amount));
         auto accountPermissions = get_account_permissions(tx_permission, {name(bidder_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(bidname), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct bidname_info_subcommand {
   bool print_json = false;
   string newname;
   bidname_info_subcommand(CLI::App* actionRoot) {
      auto list_producers = actionRoot->add_subcommand("bidnameinfo", localized("Get bidname info"));
      list_producers->add_flag("--json,-j", print_json, localized("Output in JSON format"));
      list_producers->add_option("newname", newname, localized("The bidding name"))->required();
      list_producers->callback([this] {
         auto rawResult = call(get_table_func, fc::mutable_variant_object("json", true)
                               ("code", name(config::system_account_name).to_string())
                               ("scope", name(config::system_account_name).to_string())
                               ("table", "namebids")
                               ("lower_bound", name(newname).to_uint64_t())
                               ("upper_bound", name(newname).to_uint64_t() + 1)
                               // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                               // Change to newname.value when cleos no longer needs to support nodeos versions older than 1.5.0
                               ("limit", 1));
         if ( print_json ) {
            std::cout << fc::json::to_pretty_string(rawResult) << std::endl;
            return;
         }
         auto result = rawResult.as<eosio::chain_apis::read_only::get_table_rows_result>();
         // Condition in if statement below can simply be res.rows.empty() when cleos no longer needs to support nodeos versions older than 1.5.0
         if( result.rows.empty() || result.rows[0].get_object()["newname"].as_string() != name(newname).to_string() ) {
            std::cout << "No bidname record found" << std::endl;
            return;
         }
         const auto& row = result.rows[0];
         string time = row["last_bid_time"].as_string();
         try {
             time = (string)fc::time_point(fc::microseconds(to_uint64(time)));
         } catch (fc::parse_error_exception&) {
         }
         int64_t bid = row["high_bid"].as_int64();
         std::cout << std::left << std::setw(18) << "bidname:" << std::right << std::setw(24) << row["newname"].as_string() << "\n"
                   << std::left << std::setw(18) << "highest bidder:" << std::right << std::setw(24) << row["high_bidder"].as_string() << "\n"
                   << std::left << std::setw(18) << "highest bid:" << std::right << std::setw(24) << (bid > 0 ? bid : -bid) << "\n"
                   << std::left << std::setw(18) << "last bid time:" << std::right << std::setw(24) << time << std::endl;
         if (bid < 0) std::cout << "This auction has already closed" << std::endl;
      });
   }
};

struct list_bw_subcommand {
   string account;
   bool print_json = false;

   list_bw_subcommand(CLI::App* actionRoot) {
      auto list_bw = actionRoot->add_subcommand("listbw", localized("List delegated bandwidth"));
      list_bw->add_option("account", account, localized("The account delegated bandwidth"))->required();
      list_bw->add_flag("--json,-j", print_json, localized("Output in JSON format") );

      list_bw->callback([this] {
            //get entire table in scope of user account
            auto result = call(get_table_func, fc::mutable_variant_object("json", true)
                               ("code", name(config::system_account_name).to_string())
                               ("scope", name(account).to_string())
                               ("table", "delband")
            );
            if (!print_json) {
               auto res = result.as<eosio::chain_apis::read_only::get_table_rows_result>();
               if ( !res.rows.empty() ) {
                  std::cout << std::setw(13) << std::left << "Receiver" << std::setw(21) << std::left << "Net bandwidth"
                            << std::setw(21) << std::left << "CPU bandwidth" << std::endl;
                  for ( auto& r : res.rows ){
                     std::cout << std::setw(13) << std::left << r["to"].as_string()
                               << std::setw(21) << std::left << r["net_weight"].as_string()
                               << std::setw(21) << std::left << r["cpu_weight"].as_string()
                               << std::endl;
                  }
               } else {
                  std::cerr << "Delegated bandwidth not found" << std::endl;
               }
            } else {
               std::cout << fc::json::to_pretty_string(result) << std::endl;
            }
      });
   }
};

struct buyram_subcommand {
   string from_str;
   string receiver_str;
   string amount;
   bool kbytes = false;
   bool bytes = false;

   buyram_subcommand(CLI::App* actionRoot) {
      auto buyram = actionRoot->add_subcommand("buyram", localized("Buy RAM"));
      buyram->add_option("payer", from_str, localized("The account paying for RAM"))->required();
      buyram->add_option("receiver", receiver_str, localized("The account receiving bought RAM"))->required();
      buyram->add_option("amount", amount, localized("The amount of tokens to pay for RAM, or number of bytes/kibibytes of RAM if --bytes/--kbytes is set"))->required();
      buyram->add_flag("--kbytes,-k", kbytes, localized("The amount to buy in kibibytes (KiB)"));
      buyram->add_flag("--bytes,-b", bytes, localized("The amount to buy in bytes"));
      add_standard_transaction_options_plus_signing(buyram, "payer@active");
      buyram->callback([this] {
         EOSC_ASSERT( !kbytes || !bytes, "ERROR: --kbytes and --bytes cannot be set at the same time" );
         if (kbytes || bytes) {
            send_actions( { create_buyrambytes(name(from_str), name(receiver_str), fc::to_uint64(amount) * ((kbytes) ? 1024ull : 1ull)) }, signing_keys_opt.get_keys());
         } else {
            send_actions( { create_buyram(name(from_str), name(receiver_str), to_asset(amount)) }, signing_keys_opt.get_keys());
         }
      });
   }
};

struct sellram_subcommand {
   string from_str;
   string receiver_str;
   uint64_t amount;

   sellram_subcommand(CLI::App* actionRoot) {
      auto sellram = actionRoot->add_subcommand("sellram", localized("Sell RAM"));
      sellram->add_option("account", receiver_str, localized("The account to receive tokens for sold RAM"))->required();
      sellram->add_option("bytes", amount, localized("The amount of RAM bytes to sell"))->required();
      add_standard_transaction_options_plus_signing(sellram, "account@active");

      sellram->callback([this] {
            fc::variant act_payload = fc::mutable_variant_object()
               ("account", receiver_str)
               ("bytes", amount);
            auto accountPermissions = get_account_permissions(tx_permission, {name(receiver_str), config::active_name});
            send_actions({create_action(accountPermissions, config::system_account_name, N(sellram), act_payload)}, signing_keys_opt.get_keys());
         });
   }
};

struct claimrewards_subcommand {
   string owner;

   claimrewards_subcommand(CLI::App* actionRoot) {
      auto claim_rewards = actionRoot->add_subcommand("claimrewards", localized("Claim producer rewards"));
      claim_rewards->add_option("owner", owner, localized("The account to claim rewards for"))->required();
      add_standard_transaction_options_plus_signing(claim_rewards, "owner@active");

      claim_rewards->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("owner", owner);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(claimrewards), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct regproxy_subcommand {
   string proxy;

   regproxy_subcommand(CLI::App* actionRoot) {
      auto register_proxy = actionRoot->add_subcommand("regproxy", localized("Register an account as a proxy (for voting)"));
      register_proxy->add_option("proxy", proxy, localized("The proxy account to register"))->required();
      add_standard_transaction_options_plus_signing(register_proxy, "proxy@active");

      register_proxy->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("proxy", proxy)
                  ("isproxy", true);
         auto accountPermissions = get_account_permissions(tx_permission, {name(proxy), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(regproxy), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct unregproxy_subcommand {
   string proxy;

   unregproxy_subcommand(CLI::App* actionRoot) {
      auto unregister_proxy = actionRoot->add_subcommand("unregproxy", localized("Unregister an account as a proxy (for voting)"));
      unregister_proxy->add_option("proxy", proxy, localized("The proxy account to unregister"))->required();
      add_standard_transaction_options_plus_signing(unregister_proxy, "proxy@active");

      unregister_proxy->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
                  ("proxy", proxy)
                  ("isproxy", false);
         auto accountPermissions = get_account_permissions(tx_permission, {name(proxy), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, N(regproxy), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct canceldelay_subcommand {
   string canceling_account;
   string canceling_permission;
   string trx_id;

   canceldelay_subcommand(CLI::App* actionRoot) {
      auto cancel_delay = actionRoot->add_subcommand("canceldelay", localized("Cancel a delayed transaction"));
      cancel_delay->add_option("canceling_account", canceling_account, localized("Account from authorization on the original delayed transaction"))->required();
      cancel_delay->add_option("canceling_permission", canceling_permission, localized("Permission from authorization on the original delayed transaction"))->required();
      cancel_delay->add_option("trx_id", trx_id, localized("The transaction id of the original delayed transaction"))->required();
      add_standard_transaction_options_plus_signing(cancel_delay, "canceling_account@canceling_permission");

      cancel_delay->callback([this] {
         auto canceling_auth = permission_level{name(canceling_account), name(canceling_permission)};
         fc::variant act_payload = fc::mutable_variant_object()
                  ("canceling_auth", canceling_auth)
                  ("trx_id", trx_id);
         auto accountPermissions = get_account_permissions(tx_permission, canceling_auth);
         send_actions({create_action(accountPermissions, config::system_account_name, N(canceldelay), act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct deposit_subcommand {
   string owner_str;
   string amount_str;
   const name act_name{ N(deposit) };

   deposit_subcommand(CLI::App* actionRoot) {
      auto deposit = actionRoot->add_subcommand("deposit", localized("Deposit into owner's REX fund by transfering from owner's liquid token balance"));
      deposit->add_option("owner",  owner_str,  localized("Account which owns the REX fund"))->required();
      deposit->add_option("amount", amount_str, localized("Amount to be deposited into REX fund"))->required();
      add_standard_transaction_options_plus_signing(deposit, "owner@active");
      deposit->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("owner",  owner_str)
            ("amount", amount_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct withdraw_subcommand {
   string owner_str;
   string amount_str;
   const name act_name{ N(withdraw) };

   withdraw_subcommand(CLI::App* actionRoot) {
      auto withdraw = actionRoot->add_subcommand("withdraw", localized("Withdraw from owner's REX fund by transfering to owner's liquid token balance"));
      withdraw->add_option("owner",  owner_str,  localized("Account which owns the REX fund"))->required();
      withdraw->add_option("amount", amount_str, localized("Amount to be withdrawn from REX fund"))->required();
      add_standard_transaction_options_plus_signing(withdraw, "owner@active");
      withdraw->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("owner",  owner_str)
            ("amount", amount_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct buyrex_subcommand {
   string from_str;
   string amount_str;
   const name act_name{ N(buyrex) };

   buyrex_subcommand(CLI::App* actionRoot) {
      auto buyrex = actionRoot->add_subcommand("buyrex", localized("Buy REX using tokens in owner's REX fund"));
      buyrex->add_option("from",   from_str,   localized("Account buying REX tokens"))->required();
      buyrex->add_option("amount", amount_str, localized("Amount to be taken from REX fund and used in buying REX"))->required();
      add_standard_transaction_options_plus_signing(buyrex, "from@active");
      buyrex->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",   from_str)
            ("amount", amount_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct lendrex_subcommand {
   string from_str;
   string amount_str;
   const name act_name1{ N(deposit) };
   const name act_name2{ N(buyrex) };

   lendrex_subcommand(CLI::App* actionRoot) {
      auto lendrex = actionRoot->add_subcommand("lendrex", localized("Deposit tokens to REX fund and use the tokens to buy REX"));
      lendrex->add_option("from",   from_str,   localized("Account buying REX tokens"))->required();
      lendrex->add_option("amount", amount_str, localized("Amount of liquid tokens to be used in buying REX"))->required();
      add_standard_transaction_options_plus_signing(lendrex, "from@active");
      lendrex->callback([this] {
         fc::variant act_payload1 = fc::mutable_variant_object()
            ("owner",  from_str)
            ("amount", amount_str);
         fc::variant act_payload2 = fc::mutable_variant_object()
            ("from",   from_str)
            ("amount", amount_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name1, act_payload1),
                       create_action(accountPermissions, config::system_account_name, act_name2, act_payload2)}, signing_keys_opt.get_keys());
      });
   }
};

struct unstaketorex_subcommand {
   string owner_str;
   string receiver_str;
   string from_net_str;
   string from_cpu_str;
   const name act_name{ N(unstaketorex) };

   unstaketorex_subcommand(CLI::App* actionRoot) {
      auto unstaketorex = actionRoot->add_subcommand("unstaketorex", localized("Buy REX using staked tokens"));
      unstaketorex->add_option("owner",    owner_str,    localized("Account buying REX tokens"))->required();
      unstaketorex->add_option("receiver", receiver_str, localized("Account that tokens have been staked to"))->required();
      unstaketorex->add_option("from_net", from_net_str, localized("Amount to be unstaked from Net resources and used in REX purchase"))->required();
      unstaketorex->add_option("from_cpu", from_cpu_str, localized("Amount to be unstaked from CPU resources and used in REX purchase"))->required();
      add_standard_transaction_options_plus_signing(unstaketorex, "owner@active");
      unstaketorex->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("owner",    owner_str)
            ("receiver", receiver_str)
            ("from_net", from_net_str)
            ("from_cpu", from_cpu_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct sellrex_subcommand {
   string from_str;
   string rex_str;
   const name act_name{ N(sellrex) };

   sellrex_subcommand(CLI::App* actionRoot) {
      auto sellrex = actionRoot->add_subcommand("sellrex", localized("Sell REX tokens"));
      sellrex->add_option("from", from_str, localized("Account selling REX tokens"))->required();
      sellrex->add_option("rex",  rex_str,  localized("Amount of REX tokens to be sold"))->required();
      add_standard_transaction_options_plus_signing(sellrex, "from@active");
      sellrex->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from", from_str)
            ("rex",  rex_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct cancelrexorder_subcommand {
   string owner_str;
   const name act_name{ N(cnclrexorder) };

   cancelrexorder_subcommand(CLI::App* actionRoot) {
      auto cancelrexorder = actionRoot->add_subcommand("cancelrexorder", localized("Cancel queued REX sell order if one exists"));
      cancelrexorder->add_option("owner", owner_str, localized("Owner account of sell order"))->required();
      add_standard_transaction_options_plus_signing(cancelrexorder, "owner@active");
      cancelrexorder->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()("owner", owner_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct rentcpu_subcommand {
   string from_str;
   string receiver_str;
   string loan_payment_str;
   string loan_fund_str;
   const name act_name{ N(rentcpu) };

   rentcpu_subcommand(CLI::App* actionRoot) {
      auto rentcpu = actionRoot->add_subcommand("rentcpu", localized("Rent CPU bandwidth for 30 days"));
      rentcpu->add_option("from",         from_str,         localized("Account paying rent fees"))->required();
      rentcpu->add_option("receiver",     receiver_str,     localized("Account to whom rented CPU bandwidth is staked"))->required();
      rentcpu->add_option("loan_payment", loan_payment_str, localized("Loan fee to be paid, used to calculate amount of rented bandwidth"))->required();
      rentcpu->add_option("loan_fund",    loan_fund_str,    localized("Loan fund to be used in automatic renewal, can be 0 tokens"))->required();
      add_standard_transaction_options_plus_signing(rentcpu, "from@active");
      rentcpu->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",         from_str)
            ("receiver",     receiver_str)
            ("loan_payment", loan_payment_str)
            ("loan_fund",    loan_fund_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct rentnet_subcommand {
   string from_str;
   string receiver_str;
   string loan_payment_str;
   string loan_fund_str;
   const name act_name{ N(rentnet) };

   rentnet_subcommand(CLI::App* actionRoot) {
      auto rentnet = actionRoot->add_subcommand("rentnet", localized("Rent Network bandwidth for 30 days"));
      rentnet->add_option("from",         from_str,         localized("Account paying rent fees"))->required();
      rentnet->add_option("receiver",     receiver_str,     localized("Account to whom rented Network bandwidth is staked"))->required();
      rentnet->add_option("loan_payment", loan_payment_str, localized("Loan fee to be paid, used to calculate amount of rented bandwidth"))->required();
      rentnet->add_option("loan_fund",    loan_fund_str,    localized("Loan fund to be used in automatic renewal, can be 0 tokens"))->required();
      add_standard_transaction_options_plus_signing(rentnet, "from@active");
      rentnet->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",         from_str)
            ("receiver",     receiver_str)
            ("loan_payment", loan_payment_str)
            ("loan_fund",    loan_fund_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct fundcpuloan_subcommand {
   string from_str;
   string loan_num_str;
   string payment_str;
   const name act_name{ N(fundcpuloan) };

   fundcpuloan_subcommand(CLI::App* actionRoot) {
      auto fundcpuloan = actionRoot->add_subcommand("fundcpuloan", localized("Deposit into a CPU loan fund"));
      fundcpuloan->add_option("from",     from_str,     localized("Loan owner"))->required();
      fundcpuloan->add_option("loan_num", loan_num_str, localized("Loan ID"))->required();
      fundcpuloan->add_option("payment",  payment_str,  localized("Amount to be deposited"))->required();
      add_standard_transaction_options_plus_signing(fundcpuloan, "from@active");
      fundcpuloan->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",     from_str)
            ("loan_num", loan_num_str)
            ("payment",  payment_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct fundnetloan_subcommand {
   string from_str;
   string loan_num_str;
   string payment_str;
   const name act_name{ N(fundnetloan) };

   fundnetloan_subcommand(CLI::App* actionRoot) {
      auto fundnetloan = actionRoot->add_subcommand("fundnetloan", localized("Deposit into a Network loan fund"));
      fundnetloan->add_option("from",     from_str,     localized("Loan owner"))->required();
      fundnetloan->add_option("loan_num", loan_num_str, localized("Loan ID"))->required();
      fundnetloan->add_option("payment",  payment_str,  localized("Amount to be deposited"))->required();
      add_standard_transaction_options_plus_signing(fundnetloan, "from@active");
      fundnetloan->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",     from_str)
            ("loan_num", loan_num_str)
            ("payment",  payment_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct defcpuloan_subcommand {
   string from_str;
   string loan_num_str;
   string amount_str;
   const name act_name{ N(defcpuloan) };

   defcpuloan_subcommand(CLI::App* actionRoot) {
      auto defcpuloan = actionRoot->add_subcommand("defundcpuloan", localized("Withdraw from a CPU loan fund"));
      defcpuloan->add_option("from",     from_str,     localized("Loan owner"))->required();
      defcpuloan->add_option("loan_num", loan_num_str, localized("Loan ID"))->required();
      defcpuloan->add_option("amount",   amount_str,  localized("Amount to be withdrawn"))->required();
      add_standard_transaction_options_plus_signing(defcpuloan, "from@active");
      defcpuloan->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",     from_str)
            ("loan_num", loan_num_str)
            ("amount",   amount_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct defnetloan_subcommand {
   string from_str;
   string loan_num_str;
   string amount_str;
   const name act_name{ N(defnetloan) };

   defnetloan_subcommand(CLI::App* actionRoot) {
      auto defnetloan = actionRoot->add_subcommand("defundnetloan", localized("Withdraw from a Network loan fund"));
      defnetloan->add_option("from",     from_str,     localized("Loan owner"))->required();
      defnetloan->add_option("loan_num", loan_num_str, localized("Loan ID"))->required();
      defnetloan->add_option("amount",   amount_str,  localized("Amount to be withdrawn"))->required();
      add_standard_transaction_options_plus_signing(defnetloan, "from@active");
      defnetloan->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("from",     from_str)
            ("loan_num", loan_num_str)
            ("amount",   amount_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(from_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct mvtosavings_subcommand {
   string owner_str;
   string rex_str;
   const name act_name{ N(mvtosavings) };

   mvtosavings_subcommand(CLI::App* actionRoot) {
      auto mvtosavings = actionRoot->add_subcommand("mvtosavings", localized("Move REX tokens to savings bucket"));
      mvtosavings->add_option("owner", owner_str, localized("REX owner"))->required();
      mvtosavings->add_option("rex",   rex_str,   localized("Amount of REX to be moved to savings bucket"))->required();
      add_standard_transaction_options_plus_signing(mvtosavings, "owner@active");
      mvtosavings->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("owner", owner_str)
            ("rex",   rex_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct mvfrsavings_subcommand {
   string owner_str;
   string rex_str;
   const name act_name{ N(mvfrsavings) };

   mvfrsavings_subcommand(CLI::App* actionRoot) {
      auto mvfrsavings = actionRoot->add_subcommand("mvfromsavings", localized("Move REX tokens out of savings bucket"));
      mvfrsavings->add_option("owner", owner_str, localized("REX owner"))->required();
      mvfrsavings->add_option("rex",   rex_str,   localized("Amount of REX to be moved out of savings bucket"))->required();
      add_standard_transaction_options_plus_signing(mvfrsavings, "owner@active");
      mvfrsavings->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()
            ("owner", owner_str)
            ("rex",   rex_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct updaterex_subcommand {
   string owner_str;
   const name act_name{ N(updaterex) };

   updaterex_subcommand(CLI::App* actionRoot) {
      auto updaterex = actionRoot->add_subcommand("updaterex", localized("Update REX owner vote stake and vote weight"));
      updaterex->add_option("owner", owner_str, localized("REX owner"))->required();
      add_standard_transaction_options_plus_signing(updaterex, "owner@active");
      updaterex->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()("owner", owner_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct consolidate_subcommand {
   string owner_str;
   const name act_name{ N(consolidate) };

   consolidate_subcommand(CLI::App* actionRoot) {
      auto consolidate = actionRoot->add_subcommand("consolidate", localized("Consolidate REX maturity buckets into one that matures in 4 days"));
      consolidate->add_option("owner", owner_str, localized("REX owner"))->required();
      add_standard_transaction_options_plus_signing(consolidate, "owner@active");
      consolidate->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()("owner", owner_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct rexexec_subcommand {
   string user_str;
   string max_str;
   const name act_name{ N(rexexec) };

   rexexec_subcommand(CLI::App* actionRoot) {
      auto rexexec = actionRoot->add_subcommand("rexexec", localized("Perform REX maintenance by processing expired loans and unfilled sell orders"));
      rexexec->add_option("user", user_str, localized("User executing the action"))->required();
      rexexec->add_option("max",  max_str,  localized("Maximum number of CPU loans, Network loans, and sell orders to be processed"))->required();
      add_standard_transaction_options_plus_signing(rexexec, "user@active");
      rexexec->callback([this] {
            fc::variant act_payload = fc::mutable_variant_object()
               ("user", user_str)
               ("max",  max_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(user_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

struct closerex_subcommand {
   string owner_str;
   const name act_name{ N(closerex) };

   closerex_subcommand(CLI::App* actionRoot) {
      auto closerex = actionRoot->add_subcommand("closerex", localized("Delete unused REX-related user table entries"));
      closerex->add_option("owner", owner_str, localized("REX owner"))->required();
      add_standard_transaction_options_plus_signing(closerex, "owner@active");
      closerex->callback([this] {
         fc::variant act_payload = fc::mutable_variant_object()("owner", owner_str);
         auto accountPermissions = get_account_permissions(tx_permission, {name(owner_str), config::active_name});
         send_actions({create_action(accountPermissions, config::system_account_name, act_name, act_payload)}, signing_keys_opt.get_keys());
      });
   }
};

void get_account( const string& accountName, const string& coresym, bool json_format ) {
   fc::variant json;
   if (coresym.empty()) {
      json = call(get_account_func, fc::mutable_variant_object("account_name", accountName));
   }
   else {
      json = call(get_account_func, fc::mutable_variant_object("account_name", accountName)("expected_core_symbol", symbol::from_string(coresym)));
   }

   auto res = json.as<eosio::chain_apis::read_only::get_account_results>();
   if (!json_format) {
      asset staked;
      asset unstaking;

      if( res.core_liquid_balance.valid() ) {
         unstaking = asset( 0, res.core_liquid_balance->get_symbol() ); // Correct core symbol for unstaking asset.
         staked = asset( 0, res.core_liquid_balance->get_symbol() );    // Correct core symbol for staked asset.
      }

      std::cout << "created: " << string(res.created) << std::endl;

      if(res.privileged) std::cout << "privileged: true" << std::endl;

      constexpr size_t indent_size = 5;
      const string indent(indent_size, ' ');

      std::cout << "permissions: " << std::endl;
      unordered_map<name, vector<name>/*children*/> tree;
      vector<name> roots; //we don't have multiple roots, but we can easily handle them here, so let's do it just in case
      unordered_map<name, eosio::chain_apis::permission> cache;
      for ( auto& perm : res.permissions ) {
         if ( perm.parent ) {
            tree[perm.parent].push_back( perm.perm_name );
         } else {
            roots.push_back( perm.perm_name );
         }
         auto name = perm.perm_name; //keep copy before moving `perm`, since thirst argument of emplace can be evaluated first
         // looks a little crazy, but should be efficient
         cache.insert( std::make_pair(name, std::move(perm)) );
      }
      std::function<void (account_name, int)> dfs_print = [&]( account_name name, int depth ) -> void {
         auto& p = cache.at(name);
         std::cout << indent << std::string(depth*3, ' ') << name << ' ' << std::setw(5) << p.required_auth.threshold << ":    ";
         const char *sep = "";
         for ( auto it = p.required_auth.keys.begin(); it != p.required_auth.keys.end(); ++it ) {
            std::cout << sep << it->weight << ' ' << it->key.to_string();
            sep = ", ";
         }
         for ( auto& acc : p.required_auth.accounts ) {
            std::cout << sep << acc.weight << ' ' << acc.permission.actor.to_string() << '@' << acc.permission.permission.to_string();
            sep = ", ";
         }
         std::cout << std::endl;
         auto it = tree.find( name );
         if (it != tree.end()) {
            auto& children = it->second;
            sort( children.begin(), children.end() );
            for ( auto& n : children ) {
               // we have a tree, not a graph, so no need to check for already visited nodes
               dfs_print( n, depth+1 );
            }
         } // else it's a leaf node
      };
      std::sort(roots.begin(), roots.end());
      for ( auto r : roots ) {
         dfs_print( r, 0 );
      }

      auto to_pretty_net = []( int64_t nbytes, uint8_t width_for_units = 5 ) {
         if(nbytes == -1) {
             // special case. Treat it as unlimited
             return std::string("unlimited");
         }

         string unit = "bytes";
         double bytes = static_cast<double> (nbytes);
         if (bytes >= 1024 * 1024 * 1024 * 1024ll) {
             unit = "TiB";
             bytes /= 1024 * 1024 * 1024 * 1024ll;
         } else if (bytes >= 1024 * 1024 * 1024) {
             unit = "GiB";
             bytes /= 1024 * 1024 * 1024;
         } else if (bytes >= 1024 * 1024) {
             unit = "MiB";
             bytes /= 1024 * 1024;
         } else if (bytes >= 1024) {
             unit = "KiB";
             bytes /= 1024;
         }
         std::stringstream ss;
         ss << setprecision(4);
         ss << bytes << " ";
         if( width_for_units > 0 )
            ss << std::left << setw( width_for_units );
         ss << unit;
         return ss.str();
      };



      std::cout << "memory: " << std::endl
                << indent << "quota: " << std::setw(15) << to_pretty_net(res.ram_quota) << "  used: " << std::setw(15) << to_pretty_net(res.ram_usage) << std::endl << std::endl;

      std::cout << "net bandwidth: " << std::endl;
      if ( res.total_resources.is_object() ) {
         auto net_total = to_asset(res.total_resources.get_object()["net_weight"].as_string());

         if( net_total.get_symbol() != unstaking.get_symbol() ) {
            // Core symbol of nodeos responding to the request is different than core symbol built into cleos
            unstaking = asset( 0, net_total.get_symbol() ); // Correct core symbol for unstaking asset.
            staked = asset( 0, net_total.get_symbol() ); // Correct core symbol for staked asset.
         }

         if( res.self_delegated_bandwidth.is_object() ) {
            asset net_own =  asset::from_string( res.self_delegated_bandwidth.get_object()["net_weight"].as_string() );
            staked = net_own;

            auto net_others = net_total - net_own;

            std::cout << indent << "staked:" << std::setw(20) << net_own
                      << std::string(11, ' ') << "(total stake delegated from account to self)" << std::endl
                      << indent << "delegated:" << std::setw(17) << net_others
                      << std::string(11, ' ') << "(total staked delegated to account from others)" << std::endl;
         }
         else {
            auto net_others = net_total;
            std::cout << indent << "delegated:" << std::setw(17) << net_others
                      << std::string(11, ' ') << "(total staked delegated to account from others)" << std::endl;
         }
      }


      auto to_pretty_time = []( int64_t nmicro, uint8_t width_for_units = 5 ) {
         if(nmicro == -1) {
             // special case. Treat it as unlimited
             return std::string("unlimited");
         }
         string unit = "us";
         double micro = static_cast<double>(nmicro);

         if( micro > 1000000*60*60ll ) {
            micro /= 1000000*60*60ll;
            unit = "hr";
         }
         else if( micro > 1000000*60 ) {
            micro /= 1000000*60;
            unit = "min";
         }
         else if( micro > 1000000 ) {
            micro /= 1000000;
            unit = "sec";
         }
         else if( micro > 1000 ) {
            micro /= 1000;
            unit = "ms";
         }
         std::stringstream ss;
         ss << setprecision(4);
         ss << micro << " ";
         if( width_for_units > 0 )
            ss << std::left << setw( width_for_units );
         ss << unit;
         return ss.str();
      };

      std::cout << std::fixed << setprecision(3);
      std::cout << indent << std::left << std::setw(11) << "used:" << std::right << std::setw(18);
      if( res.net_limit.current_used ) {
         std::cout << to_pretty_net(*res.net_limit.current_used) << "\n";
      } else {
         std::cout << to_pretty_net(res.net_limit.used) << "    ( out of date )\n";
      }
      std::cout << indent << std::left << std::setw(11) << "available:" << std::right << std::setw(18) << to_pretty_net( res.net_limit.available ) << "\n";
      std::cout << indent << std::left << std::setw(11) << "limit:"     << std::right << std::setw(18) << to_pretty_net( res.net_limit.max ) << "\n";
      std::cout << std::endl;

      std::cout << "cpu bandwidth:" << std::endl;

      if ( res.total_resources.is_object() ) {
         auto cpu_total = to_asset(res.total_resources.get_object()["cpu_weight"].as_string());

         if( res.self_delegated_bandwidth.is_object() ) {
            asset cpu_own = asset::from_string( res.self_delegated_bandwidth.get_object()["cpu_weight"].as_string() );
            staked += cpu_own;

            auto cpu_others = cpu_total - cpu_own;

            std::cout << indent << "staked:" << std::setw(20) << cpu_own
                      << std::string(11, ' ') << "(total stake delegated from account to self)" << std::endl
                      << indent << "delegated:" << std::setw(17) << cpu_others
                      << std::string(11, ' ') << "(total staked delegated to account from others)" << std::endl;
         } else {
            auto cpu_others = cpu_total;
            std::cout << indent << "delegated:" << std::setw(17) << cpu_others
                      << std::string(11, ' ') << "(total staked delegated to account from others)" << std::endl;
         }
      }

      std::cout << std::fixed << setprecision(3);
      std::cout << indent << std::left << std::setw(11) << "used:" << std::right << std::setw(18);
      if( res.cpu_limit.current_used ) {
         std::cout << to_pretty_time(*res.cpu_limit.current_used) << "\n";
      } else {
         std::cout << to_pretty_time(res.cpu_limit.used) << "    ( out of date )\n";
      }
      std::cout << indent << std::left << std::setw(11) << "available:" << std::right << std::setw(18) << to_pretty_time( res.cpu_limit.available ) << "\n";
      std::cout << indent << std::left << std::setw(11) << "limit:"     << std::right << std::setw(18) << to_pretty_time( res.cpu_limit.max ) << "\n";
      std::cout << std::endl;

      if( res.refund_request.is_object() ) {
         auto obj = res.refund_request.get_object();
         auto request_time = fc::time_point_sec::from_iso_string( obj["request_time"].as_string() );
         fc::time_point refund_time = request_time + fc::days(3);
         auto now = res.head_block_time;
         asset net = asset::from_string( obj["net_amount"].as_string() );
         asset cpu = asset::from_string( obj["cpu_amount"].as_string() );
         unstaking = net + cpu;

         if( unstaking > asset( 0, unstaking.get_symbol() ) ) {
            std::cout << std::fixed << setprecision(3);
            std::cout << "unstaking tokens:" << std::endl;
            std::cout << indent << std::left << std::setw(25) << "time of unstake request:" << std::right << std::setw(20) << string(request_time);
            if( now >= refund_time ) {
               std::cout << " (available to claim now with 'eosio::refund' action)\n";
            } else {
               std::cout << " (funds will be available in " << to_pretty_time( (refund_time - now).count(), 0 ) << ")\n";
            }
            std::cout << indent << std::left << std::setw(25) << "from net bandwidth:" << std::right << std::setw(18) << net << std::endl;
            std::cout << indent << std::left << std::setw(25) << "from cpu bandwidth:" << std::right << std::setw(18) << cpu << std::endl;
            std::cout << indent << std::left << std::setw(25) << "total:" << std::right << std::setw(18) << unstaking << std::endl;
            std::cout << std::endl;
         }
      }

      if( res.core_liquid_balance.valid() ) {
         std::cout << res.core_liquid_balance->get_symbol().name() << " balances: " << std::endl;
         std::cout << indent << std::left << std::setw(11)
                   << "liquid:" << std::right << std::setw(18) << *res.core_liquid_balance << std::endl;
         std::cout << indent << std::left << std::setw(11)
                   << "staked:" << std::right << std::setw(18) << staked << std::endl;
         std::cout << indent << std::left << std::setw(11)
                   << "unstaking:" << std::right << std::setw(18) << unstaking << std::endl;
         std::cout << indent << std::left << std::setw(11) << "total:" << std::right << std::setw(18) << (*res.core_liquid_balance + staked + unstaking) << std::endl;
         std::cout << std::endl;
      }

      if( res.rex_info.is_object() ) {
         auto& obj = res.rex_info.get_object();
         asset vote_stake = asset::from_string( obj["vote_stake"].as_string() );
         asset rex_balance = asset::from_string( obj["rex_balance"].as_string() );
         std::cout << rex_balance.get_symbol().name() << " balances: " << std::endl;
         std::cout << indent << std::left << std::setw(11)
                   << "balance:" << std::right << std::setw(18) << rex_balance << std::endl;
         std::cout << indent << std::left << std::setw(11)
                   << "staked:" << std::right << std::setw(18) << vote_stake << std::endl;
         std::cout << std::endl;
      }

      if ( res.voter_info.is_object() ) {
         auto& obj = res.voter_info.get_object();
         string proxy = obj["proxy"].as_string();
         if ( proxy.empty() ) {
            auto& prods = obj["producers"].get_array();
            std::cout << "producers:";
            if ( !prods.empty() ) {
               for ( size_t i = 0; i < prods.size(); ++i ) {
                  if ( i%3 == 0 ) {
                     std::cout << std::endl << indent;
                  }
                  std::cout << std::setw(16) << std::left << prods[i].as_string();
               }
               std::cout << std::endl;
            } else {
               std::cout << indent << "<not voted>" << std::endl;
            }
         } else {
            std::cout << "proxy:" << indent << proxy << std::endl;
         }
      }
      std::cout << std::endl;
   } else {
      std::cout << fc::json::to_pretty_string(json) << std::endl;
   }
}

CLI::callback_t header_opt_callback = [](CLI::results_t res) {
   vector<string>::iterator itr;

   for (itr = res.begin(); itr != res.end(); itr++) {
       headers.push_back(*itr);
   }

   return true;
};


int main( int argc, char** argv ) {

   fc::logger::get(DEFAULT_LOGGER).set_log_level(fc::log_level::debug);
   context = eosio::client::http::create_http_context();
   wallet_url = default_wallet_url;

   CLI::App app{"Command Line Interface to EOSIO Client"};
   app.require_subcommand();
   // Hide obsolete options by putting them into a group with an empty name.
   app.add_option( "-H,--host", obsoleted_option_host_port, localized("The host where ${n} is running", ("n", node_executable_name)) )->group("");
   app.add_option( "-p,--port", obsoleted_option_host_port, localized("The port where ${n} is running", ("n", node_executable_name)) )->group("");
   app.add_option( "--wallet-host", obsoleted_option_host_port, localized("The host where ${k} is running", ("k", key_store_executable_name)) )->group("");
   app.add_option( "--wallet-port", obsoleted_option_host_port, localized("The port where ${k} is running", ("k", key_store_executable_name)) )->group("");

   app.add_option( "-u,--url", default_url, localized( "The http/https URL where ${n} is running", ("n", node_executable_name)), true );
   app.add_option( "--wallet-url", wallet_url, localized("The http/https URL where ${k} is running", ("k", key_store_executable_name)), true );
   app.add_option( "--amqp", amqp_address, localized("The ampq URL where AMQP is running amqp://USER:PASSWORD@ADDRESS:PORT"), false );

   app.add_option( "-r,--header", header_opt_callback, localized("Pass specific HTTP header; repeat this option to pass multiple headers"));
   app.add_flag( "-n,--no-verify", no_verify, localized("Don't verify peer certificate when using HTTPS"));
   app.add_flag( "--no-auto-" + string(key_store_executable_name), no_auto_keosd, localized("Don't automatically launch a ${k} if one is not currently running", ("k", key_store_executable_name)));
   app.parse_complete_callback([&app]{ ensure_keosd_running(&app);});

   app.add_flag( "-v,--verbose", verbose, localized("Output verbose errors and action console output"));
   app.add_flag("--print-request", print_request, localized("Print HTTP request to STDERR"));
   app.add_flag("--print-response", print_response, localized("Print HTTP response to STDERR"));

   auto version = app.add_subcommand("version", localized("Retrieve version information"));
   version->require_subcommand();

   version->add_subcommand("client", localized("Retrieve basic version information of the client"))->callback([] {
      std::cout << eosio::version::version_client() << '\n';
   });

   version->add_subcommand("full", localized("Retrieve full version information of the client"))->callback([] {
     std::cout << eosio::version::version_full() << '\n';
   });

   // Create subcommand
   auto create = app.add_subcommand("create", localized("Create various items, on and off the blockchain"));
   create->require_subcommand();

   bool r1 = false;
   string key_file;
   bool print_console = false;
   // create key
   auto create_key = create->add_subcommand("key", localized("Create a new keypair and print the public and private keys"))->callback( [&r1, &key_file, &print_console](){
      if (key_file.empty() && !print_console) {
         std::cerr << "ERROR: Either indicate a file using \"--file\" or pass \"--to-console\"" << std::endl;
         return;
      }

      auto pk    = r1 ? private_key_type::generate_r1() : private_key_type::generate();
      auto privs = pk.to_string();
      auto pubs  = pk.get_public_key().to_string();
      if (print_console) {
         std::cout << localized("Private key: ${key}", ("key",  privs) ) << std::endl;
         std::cout << localized("Public key: ${key}", ("key", pubs ) ) << std::endl;
      } else {
         std::cerr << localized("saving keys to ${filename}", ("filename", key_file)) << std::endl;
         std::ofstream out( key_file.c_str() );
         out << localized("Private key: ${key}", ("key",  privs) ) << std::endl;
         out << localized("Public key: ${key}", ("key", pubs ) ) << std::endl;
      }
   });
   create_key->add_flag( "--r1", r1, "Generate a key using the R1 curve (iPhone), instead of the K1 curve (Bitcoin)"  );
   create_key->add_option("-f,--file", key_file, localized("Name of file to write private/public key output to. (Must be set, unless \"--to-console\" is passed"));
   create_key->add_flag( "--to-console", print_console, localized("Print private/public keys to console."));

   // create account
   auto createAccount = create_account_subcommand( create, true /*simple*/ );

   // convert subcommand
   auto convert = app.add_subcommand("convert", localized("Pack and unpack transactions")); // TODO also add converting action args based on abi from here ?
   convert->require_subcommand();

   // pack transaction
   string plain_signed_transaction_json;
   bool pack_action_data_flag = false;
   auto pack_transaction = convert->add_subcommand("pack_transaction", localized("From plain signed JSON to packed form"));
   pack_transaction->add_option("transaction", plain_signed_transaction_json, localized("The plain signed JSON (string)"))->required();
   pack_transaction->add_flag("--pack-action-data", pack_action_data_flag, localized("Pack all action data within transaction, needs interaction with ${n}", ("n", node_executable_name)));
   pack_transaction->callback([&] {
      fc::variant trx_var = json_from_file_or_string( plain_signed_transaction_json );
      if( pack_action_data_flag ) {
         signed_transaction trx;
         try {
            abi_serializer::from_variant( trx_var, trx, abi_serializer_resolver, abi_serializer::create_yield_function( abi_serializer_max_time ) );
         } EOS_RETHROW_EXCEPTIONS( transaction_type_exception, "Invalid transaction format: '${data}'",
                                   ("data", fc::json::to_string(trx_var, fc::time_point::maximum())))
         std::cout << fc::json::to_pretty_string( packed_transaction_v0( trx, packed_transaction_v0::compression_type::none )) << std::endl;
      } else {
         try {
            signed_transaction trx = trx_var.as<signed_transaction>();
            std::cout << fc::json::to_pretty_string( fc::variant( packed_transaction_v0( trx, packed_transaction_v0::compression_type::none ))) << std::endl;
         } EOS_RETHROW_EXCEPTIONS( transaction_type_exception, "Fail to convert transaction, --pack-action-data likely needed" )
      }
   });

   // unpack transaction
   string packed_transaction_json;
   bool unpack_action_data_flag = false;
   auto unpack_transaction = convert->add_subcommand("unpack_transaction", localized("From packed to plain signed JSON form"));
   unpack_transaction->add_option("transaction", packed_transaction_json, localized("The packed transaction JSON (string containing packed_trx and optionally compression fields)"))->required();
   unpack_transaction->add_flag("--unpack-action-data", unpack_action_data_flag, localized("Unpack all action data within transaction, needs interaction with ${n}", ("n", node_executable_name)));
   unpack_transaction->callback([&] {
      fc::variant packed_trx_var = json_from_file_or_string( packed_transaction_json );
      packed_transaction_v0 packed_trx;
      try {
         fc::from_variant<packed_transaction_v0>( packed_trx_var, packed_trx );
      } EOS_RETHROW_EXCEPTIONS( transaction_type_exception, "Invalid packed transaction format: '${data}'",
                                ("data", fc::json::to_string(packed_trx_var, fc::time_point::maximum())))
      const signed_transaction& strx = packed_trx.get_signed_transaction();
      fc::variant trx_var;
      if( unpack_action_data_flag ) {
         abi_serializer::to_variant( strx, trx_var, abi_serializer_resolver, abi_serializer::create_yield_function( abi_serializer_max_time ) );
      } else {
         trx_var = strx;
      }
      std::cout << fc::json::to_pretty_string( trx_var ) << std::endl;
   });

   // pack action data
   string unpacked_action_data_account_string;
   string unpacked_action_data_name_string;
   string unpacked_action_data_string;
   auto pack_action_data = convert->add_subcommand("pack_action_data", localized("From JSON action data to packed form"));
   pack_action_data->add_option("account", unpacked_action_data_account_string, localized("The name of the account hosting the contract"))->required();
   pack_action_data->add_option("name", unpacked_action_data_name_string, localized("The name of the function called by this action"))->required();
   pack_action_data->add_option("unpacked_action_data", unpacked_action_data_string, localized("The action data expressed as JSON"))->required();
   pack_action_data->callback([&] {
      fc::variant unpacked_action_data_json = json_from_file_or_string(unpacked_action_data_string);
      bytes packed_action_data_string;
      try {
         packed_action_data_string = variant_to_bin(name(unpacked_action_data_account_string), name(unpacked_action_data_name_string), unpacked_action_data_json);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse unpacked action data JSON")
      std::cout << fc::to_hex(packed_action_data_string.data(), packed_action_data_string.size()) << std::endl;
   });

   // unpack action data
   string packed_action_data_account_string;
   string packed_action_data_name_string;
   string packed_action_data_string;
   auto unpack_action_data = convert->add_subcommand("unpack_action_data", localized("From packed to JSON action data form"));
   unpack_action_data->add_option("account", packed_action_data_account_string, localized("The name of the account that hosts the contract"))->required();
   unpack_action_data->add_option("name", packed_action_data_name_string, localized("The name of the function that's called by this action"))->required();
   unpack_action_data->add_option("packed_action_data", packed_action_data_string, localized("The action data expressed as packed hex string"))->required();
   unpack_action_data->callback([&] {
      EOS_ASSERT( packed_action_data_string.size() >= 2, transaction_type_exception, "No packed_action_data found" );
      vector<char> packed_action_data_blob(packed_action_data_string.size()/2);
      fc::from_hex(packed_action_data_string, packed_action_data_blob.data(), packed_action_data_blob.size());
      fc::variant unpacked_action_data_json = bin_to_variant(name(packed_action_data_account_string), name(packed_action_data_name_string), packed_action_data_blob);
      std::cout << fc::json::to_pretty_string(unpacked_action_data_json) << std::endl;
   });

   // validate subcommand
   auto validate = app.add_subcommand("validate", localized("Validate transactions")); 
   validate->require_subcommand();

   // validate signatures
   string trx_json_to_validate;
   string str_chain_id;
   auto validate_signatures = validate->add_subcommand("signatures", localized("Validate signatures and recover public keys"));
   validate_signatures->add_option("transaction", trx_json_to_validate,
                                   localized("The JSON string or filename defining the transaction to validate"), true)->required();
   validate_signatures->add_option("-c,--chain-id", str_chain_id, localized("The chain id that will be used in signature verification"));

   validate_signatures->callback([&] {
      fc::variant trx_var = json_from_file_or_string(trx_json_to_validate);
      signed_transaction trx;
      try {
        abi_serializer::from_variant( trx_var, trx, abi_serializer_resolver_empty, abi_serializer::create_yield_function( abi_serializer_max_time ) );
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Invalid transaction format: '${data}'",
                               ("data", fc::json::to_string(trx_var, fc::time_point::maximum())))

      fc::optional<chain_id_type> chain_id;

      if( str_chain_id.size() == 0 ) {
         ilog( "grabbing chain_id from ${n}", ("n", node_executable_name) );
         auto info = get_info();
         chain_id = info.chain_id;
      } else {
         chain_id = chain_id_type(str_chain_id);
      }

      flat_set<public_key_type> recovered_pub_keys;
      trx.get_signature_keys( *chain_id, fc::time_point::maximum(), recovered_pub_keys, false );

      std::cout << fc::json::to_pretty_string(recovered_pub_keys) << std::endl;
   });

   // Get subcommand
   auto get = app.add_subcommand("get", localized("Retrieve various items and information from the blockchain"));
   get->require_subcommand();

   // get info
   get->add_subcommand("info", localized("Get current blockchain information"))->callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   bool get_bhs = false;
   bool get_binfo = false;
   auto getBlock = get->add_subcommand("block", localized("Retrieve a full block from the blockchain"));
   getBlock->add_option("block", blockArg, localized("The number or ID of the block to retrieve"))->required();
   getBlock->add_flag("--header-state", get_bhs, localized("Get block header state from fork database instead") );
   getBlock->add_flag("--info", get_binfo, localized("Get block info from the blockchain by block num only") );
   getBlock->callback([&blockArg, &get_bhs, &get_binfo] {
      EOSC_ASSERT( !(get_bhs && get_binfo), "ERROR: Either --header-state or --info can be set" );
      if (get_binfo) {
         fc::optional<int64_t> block_num;
         try {
            block_num = fc::to_int64(blockArg);
         } catch (...) {
            // error is handled in assertion below
         }
         EOSC_ASSERT( block_num.valid() && (*block_num > 0), "Invalid block num: ${block_num}", ("block_num", blockArg) );
         const auto arg = fc::variant_object("block_num", static_cast<uint32_t>(*block_num));
         std::cout << fc::json::to_pretty_string(call(get_block_info_func, arg)) << std::endl;
      } else {
         const auto arg = fc::variant_object("block_num_or_id", blockArg);
         if (get_bhs) {
            std::cout << fc::json::to_pretty_string(call(get_block_header_state_func, arg)) << std::endl;
         } else {
            std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
         }
      }
   });

   // get account
   string accountName;
   string coresym;
   bool print_json = false;
   auto getAccount = get->add_subcommand("account", localized("Retrieve an account from the blockchain"));
   getAccount->add_option("name", accountName, localized("The name of the account to retrieve"))->required();
   getAccount->add_option("core-symbol", coresym, localized("The expected core symbol of the chain you are querying"));
   getAccount->add_flag("--json,-j", print_json, localized("Output in JSON format") );
   getAccount->callback([&]() { get_account(accountName, coresym, print_json); });

   // get code
   string codeFilename;
   string abiFilename;
   bool code_as_wasm = false;
   auto getCode = get->add_subcommand("code", localized("Retrieve the code and ABI for an account"));
   getCode->add_option("name", accountName, localized("The name of the account whose code should be retrieved"))->required();
   getCode->add_option("-c,--code",codeFilename, localized("The name of the file to save the contract .wast/wasm to") );
   getCode->add_option("-a,--abi",abiFilename, localized("The name of the file to save the contract .abi to") );
   getCode->add_flag("--wasm", code_as_wasm, localized("Save contract as wasm"));
   getCode->callback([&] {
      string code_hash, wasm, wast, abi;
      try {
         const auto result = call(get_raw_code_and_abi_func, fc::mutable_variant_object("account_name", accountName));
         const std::vector<char> wasm_v = result["wasm"].as_blob().data;
         const std::vector<char> abi_v = result["abi"].as_blob().data;

         fc::sha256 hash;
         if(wasm_v.size())
            hash = fc::sha256::hash(wasm_v.data(), wasm_v.size());
         code_hash = (string)hash;

         wasm = string(wasm_v.begin(), wasm_v.end());
         if(!code_as_wasm && wasm_v.size())
            wast = wasm_to_wast((const uint8_t*)wasm_v.data(), wasm_v.size(), false);

         abi_def abi_d;
         if(abi_serializer::to_abi(abi_v, abi_d))
            abi = fc::json::to_pretty_string(abi_d);
      }
      catch(chain::missing_chain_api_plugin_exception&) {
         //see if this is an old nodeos that doesn't support get_raw_code_and_abi
         const auto old_result = call(get_code_func, fc::mutable_variant_object("account_name", accountName)("code_as_wasm",code_as_wasm));
         code_hash = old_result["code_hash"].as_string();
         if(code_as_wasm) {
            wasm = old_result["wasm"].as_string();
            std::cout << localized("Warning: communicating to older ${n} which returns malformed binary wasm", ("n", node_executable_name)) << std::endl;
         }
         else
            wast = old_result["wast"].as_string();
         abi = fc::json::to_pretty_string(old_result["abi"]);
      }

      std::cout << localized("code hash: ${code_hash}", ("code_hash", code_hash)) << std::endl;

      if( codeFilename.size() ){
         std::cout << localized("saving ${type} to ${codeFilename}", ("type", (code_as_wasm ? "wasm" : "wast"))("codeFilename", codeFilename)) << std::endl;

         std::ofstream out( codeFilename.c_str() );
         if(code_as_wasm)
            out << wasm;
         else
            out << wast;
      }
      if( abiFilename.size() ) {
         std::cout << localized("saving abi to ${abiFilename}", ("abiFilename", abiFilename)) << std::endl;
         std::ofstream abiout( abiFilename.c_str() );
         abiout << abi;
      }
   });

   // get abi
   string filename;
   auto getAbi = get->add_subcommand("abi", localized("Retrieve the ABI for an account"));
   getAbi->add_option("name", accountName, localized("The name of the account whose abi should be retrieved"))->required();
   getAbi->add_option("-f,--file",filename, localized("The name of the file to save the contract .abi to instead of writing to console") );
   getAbi->callback([&] {
      const auto raw_abi_result = call(get_raw_abi_func, fc::mutable_variant_object("account_name", accountName));
      const auto raw_abi_blob = raw_abi_result["abi"].as_blob().data;
      if (raw_abi_blob.size() != 0) {
          const auto abi = fc::json::to_pretty_string(fc::raw::unpack<abi_def>(raw_abi_blob));
          if (filename.size()) {
              std::cerr << localized("saving abi to ${filename}", ("filename", filename)) << std::endl;
              std::ofstream abiout(filename.c_str());
              abiout << abi;
          } else {
              std::cout << abi << "\n";
          }
      } else {
        FC_THROW_EXCEPTION(key_not_found_exception, "Key ${key}", ("key", "abi"));
      }
   });

   // get table
   string scope;
   string code;
   string table;
   string lower;
   string upper;
   string table_key;
   string key_type;
   string encode_type{"dec"};
   bool binary = false;
   uint32_t limit = 10;
   string index_position;
   bool reverse = false;
   bool show_payer = false;
   auto getTable = get->add_subcommand( "table", localized("Retrieve the contents of a database table"));
   getTable->add_option( "account", code, localized("The account who owns the table") )->required();
   getTable->add_option( "scope", scope, localized("The scope within the contract in which the table is found") )->required();
   getTable->add_option( "table", table, localized("The name of the table as specified by the contract abi") )->required();
   getTable->add_option( "-l,--limit", limit, localized("The maximum number of rows to return") );
   getTable->add_option( "-k,--key", table_key, localized("Deprecated") );
   getTable->add_option( "-L,--lower", lower, localized("JSON representation of lower bound value of key, defaults to first") );
   getTable->add_option( "-U,--upper", upper, localized("JSON representation of upper bound value of key, defaults to last") );
   getTable->add_option( "--index", index_position,
                         localized("Index number, 1 - primary (first), 2 - secondary index (in order defined by multi_index), 3 - third index, etc.\n"
                                   "\t\t\t\tNumber or name of index can be specified, e.g. 'secondary' or '2'."));
   getTable->add_option( "--key-type", key_type,
                         localized("The key type of --index, primary only supports (i64), all others support (i64, i128, i256, float64, float128, ripemd160, sha256).\n"
                                   "\t\t\t\tSpecial type 'name' indicates an account name."));
   getTable->add_option( "--encode-type", encode_type,
                         localized("The encoding type of key_type (i64 , i128 , float64, float128) only support decimal encoding e.g. 'dec'"
                                    "i256 - supports both 'dec' and 'hex', ripemd160 and sha256 is 'hex' only"));
   getTable->add_flag("-b,--binary", binary, localized("Return the value as BINARY rather than using abi to interpret as JSON"));
   getTable->add_flag("-r,--reverse", reverse, localized("Iterate in reverse order"));
   getTable->add_flag("--show-payer", show_payer, localized("Show RAM payer"));


   getTable->callback([&] {
      auto result = call(get_table_func, fc::mutable_variant_object("json", !binary)
                         ("code",code)
                         ("scope",scope)
                         ("table",table)
                         ("table_key",table_key) // not used
                         ("lower_bound",lower)
                         ("upper_bound",upper)
                         ("limit",limit)
                         ("key_type",key_type)
                         ("index_position", index_position)
                         ("encode_type", encode_type)
                         ("reverse", reverse)
                         ("show_payer", show_payer)
                         );

      std::cout << fc::json::to_pretty_string(result)
                << std::endl;
   });

   auto getScope = get->add_subcommand( "scope", localized("Retrieve a list of scopes and tables owned by a contract"));
   getScope->add_option( "contract", code, localized("The contract who owns the table") )->required();
   getScope->add_option( "-t,--table", table, localized("The name of the table as filter") );
   getScope->add_option( "-l,--limit", limit, localized("The maximum number of rows to return") );
   getScope->add_option( "-L,--lower", lower, localized("Lower bound of scope") );
   getScope->add_option( "-U,--upper", upper, localized("Upper bound of scope") );
   getScope->add_flag("-r,--reverse", reverse, localized("Iterate in reverse order"));
   getScope->callback([&] {
      auto result = call(get_table_by_scope_func, fc::mutable_variant_object("code",code)
                         ("table",table)
                         ("lower_bound",lower)
                         ("upper_bound",upper)
                         ("limit",limit)
                         ("reverse", reverse)
                         );
      std::cout << fc::json::to_pretty_string(result)
                << std::endl;
   });

   // currency accessors
   // get currency balance
   string symbol;
   bool currency_balance_print_json = false;
   auto get_currency = get->add_subcommand( "currency", localized("Retrieve information related to standard currencies"));
   get_currency->require_subcommand();
   auto get_balance = get_currency->add_subcommand( "balance", localized("Retrieve the balance of an account for a given currency"));
   get_balance->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_balance->add_option( "account", accountName, localized("The account to query balances for") )->required();
   get_balance->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") );
   get_balance->add_flag("--json,-j", currency_balance_print_json, localized("Output in JSON format") );
   get_balance->callback([&] {
      auto result = call(get_currency_balance_func, fc::mutable_variant_object
         ("account", accountName)
         ("code", code)
         ("symbol", symbol.empty() ? fc::variant() : symbol)
      );
      if (!currency_balance_print_json) {
        const auto& rows = result.get_array();
        for( const auto& r : rows ) {
           std::cout << r.as_string()
                     << std::endl;
        }
      } else {
        std::cout << fc::json::to_pretty_string(result) << std::endl;
      }
   });

   auto get_currency_stats = get_currency->add_subcommand( "stats", localized("Retrieve the stats of for a given currency"));
   get_currency_stats->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_currency_stats->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") )->required();
   get_currency_stats->callback([&] {
      auto result = call(get_currency_stats_func, fc::mutable_variant_object("json", false)
         ("code", code)
         ("symbol", symbol)
      );

      std::cout << fc::json::to_pretty_string(result)
                << std::endl;
   });

   // get accounts
   string public_key_str;
   auto getAccounts = get->add_subcommand("accounts", localized("Retrieve accounts associated with a public key"));
   getAccounts->add_option("public_key", public_key_str, localized("The public key to retrieve accounts for"))->required();
   getAccounts->callback([&] {
      public_key_type public_key;
      try {
         public_key = public_key_type(public_key_str);
      } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", public_key_str))
      auto arg = fc::mutable_variant_object( "public_key", public_key);
      std::cout << fc::json::to_pretty_string(call(get_key_accounts_func, arg)) << std::endl;
   });


   // get servants
   string controllingAccount;
   auto getServants = get->add_subcommand("servants", localized("Retrieve accounts which are servants of a given account "));
   getServants->add_option("account", controllingAccount, localized("The name of the controlling account"))->required();
   getServants->callback([&] {
      auto arg = fc::mutable_variant_object( "controlling_account", controllingAccount);
      std::cout << fc::json::to_pretty_string(call(get_controlled_accounts_func, arg)) << std::endl;
   });

   // get transaction
   string transaction_id_str;
   uint32_t block_num_hint = 0;
   auto getTransaction = get->add_subcommand("transaction", localized("Retrieve a transaction from the blockchain"));
   getTransaction->add_option("id", transaction_id_str, localized("ID of the transaction to retrieve"))->required();
   getTransaction->add_option( "-b,--block-hint", block_num_hint, localized("The block number this transaction may be in") );
   getTransaction->callback([&] {
      auto arg= fc::mutable_variant_object( "id", transaction_id_str);
      if ( block_num_hint > 0 ) {
         arg = arg("block_num_hint", block_num_hint);
      }
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get actions
   string account_name;
   string skip_seq_str;
   string num_seq_str;
   bool printjson = false;
   bool fullact = false;
   bool prettyact = false;
   bool printconsole = false;

   int32_t pos_seq = -1;
   int32_t offset = -20;
   auto getActions = get->add_subcommand("actions", localized("Retrieve all actions with specific account name referenced in authorization or receiver"));
   getActions->add_option("account_name", account_name, localized("Name of account to query on"))->required();
   getActions->add_option("pos", pos_seq, localized("Sequence number of action for this account, -1 for last"));
   getActions->add_option("offset", offset, localized("Get actions [pos,pos+offset] for positive offset or [pos-offset,pos) for negative offset"));
   getActions->add_flag("--json,-j", printjson, localized("Print full JSON"));
   getActions->add_flag("--full", fullact, localized("Don't truncate action output"));
   getActions->add_flag("--pretty", prettyact, localized("Pretty print full action JSON"));
   getActions->add_flag("--console", printconsole, localized("Print console output generated by action "));
   getActions->callback([&] {
      fc::mutable_variant_object arg;
      arg( "account_name", account_name );
      arg( "pos", pos_seq );
      arg( "offset", offset);

      auto result = call(get_actions_func, arg);


      if( printjson ) {
         std::cout << fc::json::to_pretty_string(result) << std::endl;
      } else {
          auto& traces = result["actions"].get_array();
          uint32_t lib = result["last_irreversible_block"].as_uint64();


          cout  << "#" << setw(5) << "seq" << "  " << setw( 24 ) << left << "when"<< "  " << setw(24) << right << "contract::action" << " => " << setw(13) << left << "receiver" << " " << setw(11) << left << "trx id..." << " args\n";
          cout  << "================================================================================================================\n";
          for( const auto& trace: traces ) {
              std::stringstream out;
              if( trace["block_num"].as_uint64() <= lib )
                 out << "#";
              else
                 out << "?";

              out << setw(5) << trace["account_action_seq"].as_uint64() <<"  ";
              out << setw(24) << trace["block_time"].as_string() <<"  ";

              const auto& at = trace["action_trace"].get_object();

              auto id = at["trx_id"].as_string();
              const auto& receipt = at["receipt"];
              auto receiver = receipt["receiver"].as_string();
              const auto& act = at["act"].get_object();
              auto code = act["account"].as_string();
              auto func = act["name"].as_string();
              string args;
              if( prettyact ) {
                  args = fc::json::to_pretty_string( act["data"] );
              }
              else {
                 args = fc::json::to_string( act["data"], fc::time_point::maximum() );
                 if( !fullact ) {
                    args = args.substr(0,60) + "...";
                 }
              }
              out << std::setw(24) << std::right<< (code +"::" + func) << " => " << left << std::setw(13) << receiver;

              out << " " << setw(11) << (id.substr(0,8) + "...");

              if( fullact || prettyact ) out << "\n";
              else out << " ";

              out << args ;//<< "\n";

              if( trace["block_num"].as_uint64() <= lib ) {
                 dlog( "\r${m}", ("m",out.str()) );
              } else {
                 wlog( "\r${m}", ("m",out.str()) );
              }
              if( printconsole ) {
                 auto console = at["console"].as_string();
                 if( console.size() ) {
                    stringstream out;
                    std::stringstream ss(console);
                    string line;
                    while( std::getline( ss, line ) ) {
                       out << ">> " << line << "\n";
                       if( !fullact ) break;
                    }
                    cerr << out.str(); //ilog( "\r${m}                                   ", ("m",out.str()) );
                 }
              }
          }
      }
   });

   auto getSchedule = get_schedule_subcommand{get};
   auto getTransactionId = get_transaction_id_subcommand{get};

   /*
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"));
   getTransactions->add_option("account_name", account_name, localized("Name of account to query on"))->required();
   getTransactions->add_option("skip_seq", skip_seq_str, localized("Number of most recent transactions to skip (0 would start at most recent transaction)"));
   getTransactions->add_option("num_seq", num_seq_str, localized("Number of transactions to return"));
   getTransactions->add_flag("--json,-j", printjson, localized("Print full json"));
   getTransactions->callback([&] {
      fc::mutable_variant_object arg;
      if (skip_seq_str.empty()) {
         arg = fc::mutable_variant_object( "account_name", account_name);
      } else {
         uint64_t skip_seq;
         try {
            skip_seq = boost::lexical_cast<uint64_t>(skip_seq_str);
         } EOS_RETHROW_EXCEPTIONS(chain_type_exception, "Invalid Skip Seq: ${skip_seq}", ("skip_seq", skip_seq_str))
         if (num_seq_str.empty()) {
            arg = fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq);
         } else {
            uint64_t num_seq;
            try {
               num_seq = boost::lexical_cast<uint64_t>(num_seq_str);
            } EOS_RETHROW_EXCEPTIONS(chain_type_exception, "Invalid Num Seq: ${num_seq}", ("num_seq", num_seq_str))
            arg = fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq_str)("num_seq", num_seq);
         }
      }
      auto result = call(get_transactions_func, arg);
      if( printjson ) {
         std::cout << fc::json::to_pretty_string(result) << std::endl;
      }
      else {
         const auto& trxs = result.get_object()["transactions"].get_array();
         for( const auto& t : trxs ) {

            const auto& tobj = t.get_object();
            const auto& trx  = tobj["transaction"].get_object();
            const auto& data = trx["transaction"].get_object();
            const auto& msgs = data["actions"].get_array();

            for( const auto& msg : msgs ) {
               int64_t seq_num  = tobj["seq_num"].as<int64_t>();
               string  id       = tobj["transaction_id"].as_string();
               const auto& exp  = data["expiration"].as<fc::time_point_sec>();
               std::cout << tobj["seq_num"].as_string() <<"] " << id.substr(0,8) << "...  " << data["expiration"].as_string() << "  ";
               auto code = msg["account"].as_string();
               auto func = msg["name"].as_string();
               auto args = fc::json::to_string( msg["data"] );
               std::cout << setw(26) << left << (code + "::" + func) << "  " << args;
               std::cout << std::endl;
            }
         }
      }

   });
   */

   // set subcommand
   auto setSubcommand = app.add_subcommand("set", localized("Set or update blockchain state"));
   setSubcommand->require_subcommand();

   // set contract subcommand
   string account;
   string contractPath;
   string wasmPath;
   string abiPath;
   bool shouldSend = true;
   bool contract_clear = false;
   bool suppress_duplicate_check = false;
   auto codeSubcommand = setSubcommand->add_subcommand("code", localized("Create or update the code on an account"));
   codeSubcommand->add_option("account", account, localized("The account to set code for"))->required();
   codeSubcommand->add_option("code-file", wasmPath, localized("The path containing the contract WASM"));//->required();
   codeSubcommand->add_flag( "-c,--clear", contract_clear, localized("Remove code on an account"));
   codeSubcommand->add_flag( "--suppress-duplicate-check", suppress_duplicate_check, localized("Don't check for duplicate"));

   auto abiSubcommand = setSubcommand->add_subcommand("abi", localized("Create or update the abi on an account"));
   abiSubcommand->add_option("account", account, localized("The account to set the ABI for"))->required();
   abiSubcommand->add_option("abi-file", abiPath, localized("The path containing the contract ABI"));//->required();
   abiSubcommand->add_flag( "-c,--clear", contract_clear, localized("Remove abi on an account"));
   abiSubcommand->add_flag( "--suppress-duplicate-check", suppress_duplicate_check, localized("Don't check for duplicate"));

   auto contractSubcommand = setSubcommand->add_subcommand("contract", localized("Create or update the contract on an account"));
   contractSubcommand->add_option("account", account, localized("The account to publish a contract for"))
                     ->required();
   contractSubcommand->add_option("contract-dir", contractPath, localized("The path containing the .wasm and .abi"));
                     // ->required();
   contractSubcommand->add_option("wasm-file", wasmPath, localized("The file containing the contract WASM relative to contract-dir"));
//                     ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, localized("The ABI for the contract relative to contract-dir"));
//                                ->check(CLI::ExistingFile);
   contractSubcommand->add_flag( "-c,--clear", contract_clear, localized("Remove contract on an account"));
   contractSubcommand->add_flag( "--suppress-duplicate-check", suppress_duplicate_check, localized("Don't check for duplicate"));

   std::vector<chain::action> actions;
   auto set_code_callback = [&]() {

      std::vector<char> old_wasm;
      bool duplicate = false;
      fc::sha256 old_hash, new_hash;
      if (!suppress_duplicate_check) {
         try {
            const auto result = call(get_code_hash_func, fc::mutable_variant_object("account_name", account));
            old_hash = fc::sha256(result["code_hash"].as_string());
         } catch (...) {
            std::cerr << "Failed to get existing code hash, continue without duplicate check..." << std::endl;
            suppress_duplicate_check = true;
         }
      }

      bytes code_bytes;
      if(!contract_clear){
        std::string wasm;
        fc::path cpath = fc::canonical(fc::path(contractPath));

        if( wasmPath.empty() ) {
           wasmPath = (cpath / (cpath.filename().generic_string()+".wasm")).generic_string();
        } else if ( boost::filesystem::path(wasmPath).is_relative() ) {
           wasmPath = (cpath / wasmPath).generic_string();
        }

        std::cerr << localized(("Reading WASM from " + wasmPath + "...").c_str()) << std::endl;
        fc::read_file_contents(wasmPath, wasm);
        EOS_ASSERT( !wasm.empty(), wasm_file_not_found, "no wasm file found ${f}", ("f", wasmPath) );

        const string binary_wasm_header("\x00\x61\x73\x6d\x01\x00\x00\x00", 8);
        if(wasm.compare(0, 8, binary_wasm_header))
           std::cerr << localized("WARNING: ") << wasmPath << localized(" doesn't look like a binary WASM file. Is it something else, like WAST? Trying anyways...") << std::endl;
        code_bytes = bytes(wasm.begin(), wasm.end());
      } else {
        code_bytes = bytes();
      }

      if (!suppress_duplicate_check) {
         if (code_bytes.size()) {
            new_hash = fc::sha256::hash(&(code_bytes[0]), code_bytes.size());
         }
         duplicate = (old_hash == new_hash);
      }

      if (!duplicate) {
         actions.emplace_back( create_setcode(name(account), code_bytes ) );
         if ( shouldSend ) {
            std::cerr << localized("Setting Code...") << std::endl;
            if( tx_compression == tx_compression_type::default_compression )
               tx_compression = tx_compression_type::zlib;
            send_actions(std::move(actions), signing_keys_opt.get_keys());
         }
      } else {
         std::cerr << localized("Skipping set code because the new code is the same as the existing code") << std::endl;
      }
   };

   auto set_abi_callback = [&]() {

      bytes old_abi;
      bool duplicate = false;
      if (!suppress_duplicate_check) {
         try {
            const auto result = call(get_raw_abi_func, fc::mutable_variant_object("account_name", account));
            old_abi = result["abi"].as_blob().data;
         } catch (...) {
            std::cerr << "Failed to get existing raw abi, continue without duplicate check..." << std::endl;
            suppress_duplicate_check = true;
         }
      }

      bytes abi_bytes;
      if(!contract_clear){
        fc::path cpath = fc::canonical(fc::path(contractPath));

        if( abiPath.empty() ) {
           abiPath = (cpath / (cpath.filename().generic_string()+".abi")).generic_string();
        } else if ( boost::filesystem::path(abiPath).is_relative() ) {
           abiPath = (cpath / abiPath).generic_string();
        }

        EOS_ASSERT( fc::exists( abiPath ), abi_file_not_found, "no abi file found ${f}", ("f", abiPath)  );

        abi_bytes = fc::raw::pack(fc::json::from_file(abiPath).as<abi_def>());
      } else {
        abi_bytes = bytes();
      }

      if (!suppress_duplicate_check) {
         duplicate = (old_abi.size() == abi_bytes.size() && std::equal(old_abi.begin(), old_abi.end(), abi_bytes.begin()));
      }

      if (!duplicate) {
         try {
            actions.emplace_back( create_setabi(name(account), abi_bytes) );
         } EOS_RETHROW_EXCEPTIONS(abi_type_exception,  "Fail to parse ABI JSON")
         if ( shouldSend ) {
            std::cerr << localized("Setting ABI...") << std::endl;
            if( tx_compression == tx_compression_type::default_compression )
               tx_compression = tx_compression_type::zlib;
            send_actions(std::move(actions), signing_keys_opt.get_keys());
         }
      } else {
         std::cerr << localized("Skipping set abi because the new abi is the same as the existing abi") << std::endl;
      }
   };

   add_standard_transaction_options_plus_signing(contractSubcommand, "account@active");
   add_standard_transaction_options_plus_signing(codeSubcommand, "account@active");
   add_standard_transaction_options_plus_signing(abiSubcommand, "account@active");
   contractSubcommand->callback([&] {
      if(!contract_clear) EOS_ASSERT( !contractPath.empty(), contract_exception, " contract-dir is null ", ("f", contractPath) );
      shouldSend = false;
      set_code_callback();
      set_abi_callback();
      if (actions.size()) {
         std::cerr << localized("Publishing contract...") << std::endl;
         if( tx_compression == tx_compression_type::default_compression )
            tx_compression = tx_compression_type::zlib;
         send_actions(std::move(actions), signing_keys_opt.get_keys());
      } else {
         std::cout << "no transaction is sent" << std::endl;
      }
   });
   codeSubcommand->callback(set_code_callback);
   abiSubcommand->callback(set_abi_callback);

   // set account
   auto setAccount = setSubcommand->add_subcommand("account", localized("Set or update blockchain account state"))->require_subcommand();

   // set account permission
   auto setAccountPermission = set_account_permission_subcommand(setAccount);

   // set action
   auto setAction = setSubcommand->add_subcommand("action", localized("Set or update blockchain action state"))->require_subcommand();

   // set action permission
   auto setActionPermission = set_action_permission_subcommand(setAction);

   // Transfer subcommand
   string con = "eosio.token";
   string sender;
   string recipient;
   string amount;
   string memo;
   bool pay_ram = false;

   auto transfer = app.add_subcommand("transfer", localized("Transfer tokens from account to account"));
   transfer->add_option("sender", sender, localized("The account sending tokens"))->required();
   transfer->add_option("recipient", recipient, localized("The account receiving tokens"))->required();
   transfer->add_option("amount", amount, localized("The amount of tokens to send"))->required();
   transfer->add_option("memo", memo, localized("The memo for the transfer"));
   transfer->add_option("--contract,-c", con, localized("The contract that controls the token"));
   transfer->add_flag("--pay-ram-to-open", pay_ram, localized("Pay RAM to open recipient's token balance row"));

   add_standard_transaction_options_plus_signing(transfer, "sender@active");
   transfer->callback([&] {
      if (tx_force_unique && memo.size() == 0) {
         // use the memo to add a nonce
         memo = generate_nonce_string();
         tx_force_unique = false;
      }

      auto transfer_amount = to_asset(name(con), amount);
      auto transfer = create_transfer(con, name(sender), name(recipient), transfer_amount, memo);
      if (!pay_ram) {
         send_actions( { transfer }, signing_keys_opt.get_keys());
      } else {
         auto open_ = create_open(con, name(recipient), transfer_amount.get_symbol(), name(sender));
         send_actions( { open_, transfer }, signing_keys_opt.get_keys());
      }
   });

   // Net subcommand
   string new_host;
   auto net = app.add_subcommand( "net", localized("Interact with local p2p network connections"));
   net->require_subcommand();
   auto connect = net->add_subcommand("connect", localized("Start a new connection to a peer"));
   connect->add_option("host", new_host, localized("The hostname:port to connect to."))->required();
   connect->callback([&] {
      const auto& v = call(default_url, net_connect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto disconnect = net->add_subcommand("disconnect", localized("Close an existing connection"));
   disconnect->add_option("host", new_host, localized("The hostname:port to disconnect from."))->required();
   disconnect->callback([&] {
      const auto& v = call(default_url, net_disconnect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto status = net->add_subcommand("status", localized("Status of existing connection"));
   status->add_option("host", new_host, localized("The hostname:port to query status of connection"))->required();
   status->callback([&] {
      const auto& v = call(default_url, net_status, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto connections = net->add_subcommand("peers", localized("Status of all existing peers"));
   connections->callback([&] {
      const auto& v = call(default_url, net_connections, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });



   // Wallet subcommand
   auto wallet = app.add_subcommand( "wallet", localized("Interact with local wallet"));
   wallet->require_subcommand();
   // create wallet
   string wallet_name = "default";
   string password_file;
   auto createWallet = wallet->add_subcommand("create", localized("Create a new wallet locally"));
   createWallet->add_option("-n,--name", wallet_name, localized("The name of the new wallet"), true);
   createWallet->add_option("-f,--file", password_file, localized("Name of file to write wallet password output to. (Must be set, unless \"--to-console\" is passed"));
   createWallet->add_flag( "--to-console", print_console, localized("Print password to console."));
   createWallet->callback([&wallet_name, &password_file, &print_console] {
      EOSC_ASSERT( !password_file.empty() ^ print_console, "ERROR: Either indicate a file using \"--file\" or pass \"--to-console\"" );
      EOSC_ASSERT( password_file.empty() || !std::ofstream(password_file.c_str()).fail(), "ERROR: Failed to create file in specified path" );

      const auto& v = call(wallet_url, wallet_create, wallet_name);
      std::cout << localized("Creating wallet: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      std::cout << localized("Save password to use in the future to unlock this wallet.") << std::endl;
      std::cout << localized("Without password imported keys will not be retrievable.") << std::endl;
      if (print_console) {
         std::cout << fc::json::to_pretty_string(v) << std::endl;
      } else {
         std::cerr << localized("saving password to ${filename}", ("filename", password_file)) << std::endl;
         auto password_str = fc::json::to_pretty_string(v);
         boost::replace_all(password_str, "\"", "");
         std::ofstream out( password_file.c_str() );
         out << password_str;
      }
   });

   // open wallet
   auto openWallet = wallet->add_subcommand("open", localized("Open an existing wallet"));
   openWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to open"));
   openWallet->callback([&wallet_name] {
      call(wallet_url, wallet_open, wallet_name);
      std::cout << localized("Opened: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
   });

   // lock wallet
   auto lockWallet = wallet->add_subcommand("lock", localized("Lock wallet"));
   lockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to lock"));
   lockWallet->callback([&wallet_name] {
      call(wallet_url, wallet_lock, wallet_name);
      std::cout << localized("Locked: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
   });

   // lock all wallets
   auto locakAllWallets = wallet->add_subcommand("lock_all", localized("Lock all unlocked wallets"));
   locakAllWallets->callback([] {
      call(wallet_url, wallet_lock_all);
      std::cout << localized("Locked All Wallets") << std::endl;
   });

   // unlock wallet
   string wallet_pw;
   auto unlockWallet = wallet->add_subcommand("unlock", localized("Unlock wallet"));
   unlockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to unlock"));
   unlockWallet->add_option("--password", wallet_pw, localized("The password returned by wallet create"))->expected(0, 1);
   unlockWallet->callback([&wallet_name, &wallet_pw] {
      prompt_for_wallet_password(wallet_pw, wallet_name);

      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw)};
      call(wallet_url, wallet_unlock, vs);
      std::cout << localized("Unlocked: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
   });

   // import keys into wallet
   string wallet_key_str;
   auto importWallet = wallet->add_subcommand("import", localized("Import private key into wallet"));
   importWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to import key into"));
   importWallet->add_option("--private-key", wallet_key_str, localized("Private key in WIF format to import"))->expected(0, 1);
   importWallet->callback([&wallet_name, &wallet_key_str] {
      if( wallet_key_str.size() == 0 ) {
         std::cout << localized("private key: ");
         fc::set_console_echo(false);
         std::getline( std::cin, wallet_key_str, '\n' );
         fc::set_console_echo(true);
      }

      private_key_type wallet_key;
      try {
         wallet_key = private_key_type( wallet_key_str );
      } catch (...) {
         EOS_THROW(private_key_type_exception, "Invalid private key")
      }
      public_key_type pubkey = wallet_key.get_public_key();

      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_key)};
      call(wallet_url, wallet_import_key, vs);
      std::cout << localized("imported private key for: ${pubkey}", ("pubkey", pubkey.to_string())) << std::endl;
   });

   // remove keys from wallet
   string wallet_rm_key_str;
   auto removeKeyWallet = wallet->add_subcommand("remove_key", localized("Remove key from wallet"));
   removeKeyWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to remove key from"));
   removeKeyWallet->add_option("key", wallet_rm_key_str, localized("Public key in WIF format to remove"))->required();
   removeKeyWallet->add_option("--password", wallet_pw, localized("The password returned by wallet create"))->expected(0, 1);
   removeKeyWallet->callback([&wallet_name, &wallet_pw, &wallet_rm_key_str] {
      prompt_for_wallet_password(wallet_pw, wallet_name);
      public_key_type pubkey;
      try {
         pubkey = public_key_type( wallet_rm_key_str );
      } catch (...) {
         EOS_THROW(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", wallet_rm_key_str))
      }
      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw), fc::variant(wallet_rm_key_str)};
      call(wallet_url, wallet_remove_key, vs);
      std::cout << localized("removed private key for: ${pubkey}", ("pubkey", wallet_rm_key_str)) << std::endl;
   });

   // create a key within wallet
   string wallet_create_key_type;
   auto createKeyInWallet = wallet->add_subcommand("create_key", localized("Create private key within wallet"));
   createKeyInWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to create key into"), true);
   createKeyInWallet->add_option("key_type", wallet_create_key_type, localized("Key type to create (K1/R1)"), true)->type_name("K1/R1");
   createKeyInWallet->callback([&wallet_name, &wallet_create_key_type] {
      //an empty key type is allowed -- it will let the underlying wallet pick which type it prefers
      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_create_key_type)};
      const auto& v = call(wallet_url, wallet_create_key, vs);
      std::cout << localized("Created new private key with a public key of: ") << fc::json::to_pretty_string(v) << std::endl;
   });

   // list wallets
   auto listWallet = wallet->add_subcommand("list", localized("List opened wallets, * = unlocked"));
   listWallet->callback([] {
      std::cout << localized("Wallets:") << std::endl;
      const auto& v = call(wallet_url, wallet_list);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list keys
   auto listKeys = wallet->add_subcommand("keys", localized("List of public keys from all unlocked wallets."));
   listKeys->callback([] {
      const auto& v = call(wallet_url, wallet_public_keys);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list private keys
   auto listPrivKeys = wallet->add_subcommand("private_keys", localized("List of private keys from an unlocked wallet in wif or PVT_R1 format."));
   listPrivKeys->add_option("-n,--name", wallet_name, localized("The name of the wallet to list keys from"), true);
   listPrivKeys->add_option("--password", wallet_pw, localized("The password returned by wallet create"))->expected(0, 1);
   listPrivKeys->callback([&wallet_name, &wallet_pw] {
      prompt_for_wallet_password(wallet_pw, wallet_name);
      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw)};
      const auto& v = call(wallet_url, wallet_list_keys, vs);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto stopKeosd = wallet->add_subcommand("stop", localized("Stop ${k}.", ("k", key_store_executable_name)));
   stopKeosd->callback([] {
      const auto& v = call(wallet_url, keosd_stop);
      if ( !v.is_object() || v.get_object().size() != 0 ) { //on success keosd responds with empty object
         std::cerr << fc::json::to_pretty_string(v) << std::endl;
      } else {
         std::cout << "OK" << std::endl;
      }
   });

   // sign subcommand
   string trx_json_to_sign;
   string str_private_key;
   str_chain_id = {};
   string str_private_key_file;
   string str_public_key;
   bool push_trx = false;

   auto sign = app.add_subcommand("sign", localized("Sign a transaction"));
   sign->add_option("transaction", trx_json_to_sign,
                                 localized("The JSON string or filename defining the transaction to sign"), true)->required();
   sign->add_option("-k,--private-key", str_private_key, localized("The private key that will be used to sign the transaction"))->expected(0, 1);
   sign->add_option("--public-key", str_public_key, localized("Ask ${exec} to sign with the corresponding private key of the given public key", ("exec", key_store_executable_name)));
   sign->add_option("-c,--chain-id", str_chain_id, localized("The chain id that will be used to sign the transaction"));
   sign->add_flag("-p,--push-transaction", push_trx, localized("Push transaction after signing"));

   sign->callback([&] {

      EOSC_ASSERT( str_private_key.empty() || str_public_key.empty(), "ERROR: Either -k/--private-key or --public-key or none of them can be set" );
      fc::variant trx_var = json_from_file_or_string(trx_json_to_sign);
      signed_transaction trx;
      try {
        abi_serializer::from_variant( trx_var, trx, abi_serializer_resolver_empty, abi_serializer::create_yield_function( abi_serializer_max_time ) );
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Invalid transaction format: '${data}'",
                               ("data", fc::json::to_string(trx_var, fc::time_point::maximum())))

      fc::optional<chain_id_type> chain_id;

      if( str_chain_id.size() == 0 ) {
         ilog( "grabbing chain_id from ${n}", ("n", node_executable_name) );
         auto info = get_info();
         chain_id = info.chain_id;
      } else {
         chain_id = chain_id_type(str_chain_id);
      }

      if( str_public_key.size() > 0 ) {
         public_key_type pub_key;
         try {
            pub_key = public_key_type(str_public_key);
         } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", str_public_key))
         fc::variant keys_var(flat_set<public_key_type>{ pub_key });
         sign_transaction(trx, keys_var, *chain_id);
      } else {
         if( str_private_key.size() == 0 ) {
            std::cerr << localized("private key: ");
            fc::set_console_echo(false);
            std::getline( std::cin, str_private_key, '\n' );
            fc::set_console_echo(true);
         }
         private_key_type priv_key;
         try {
            priv_key = private_key_type(str_private_key);
         } EOS_RETHROW_EXCEPTIONS(private_key_type_exception, "Invalid private key")
         trx.sign(priv_key, *chain_id);
      }

      if(push_trx) {
         auto trx_result = call(push_txn_func, packed_transaction_v0(trx, packed_transaction_v0::compression_type::none));
         std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
      } else {
         std::cout << fc::json::to_pretty_string(trx) << std::endl;
      }
   });

   // Push subcommand
   auto push = app.add_subcommand("push", localized("Push arbitrary transactions to the blockchain"));
   push->require_subcommand();

   // push action
   string contract_account;
   string action;
   string data;
   vector<string> permissions;
   auto actionsSubcommand = push->add_subcommand("action", localized("Push a transaction with a single action"));
   actionsSubcommand->fallthrough(false);
   actionsSubcommand->add_option("account", contract_account,
                                 localized("The account providing the contract to execute"), true)->required();
   actionsSubcommand->add_option("action", action,
                                 localized("A JSON string or filename defining the action to execute on the contract"), true)->required();
   actionsSubcommand->add_option("data", data, localized("The arguments to the contract"))->required();

   add_standard_transaction_options_plus_signing(actionsSubcommand);
   actionsSubcommand->callback([&] {
      fc::variant action_args_var;
      if( !data.empty() ) {
         action_args_var = json_from_file_or_string(data, fc::json::parse_type::relaxed_parser);
      }
      auto accountPermissions = get_account_permissions(tx_permission);

      send_actions({chain::action{accountPermissions, name(contract_account), name(action),
                                  variant_to_bin( name(contract_account), name(action), action_args_var ) }}, signing_keys_opt.get_keys());
   });

   // push transaction
   string trx_to_push;
   auto trxSubcommand = push->add_subcommand("transaction", localized("Push an arbitrary JSON transaction"));
   trxSubcommand->add_option("transaction", trx_to_push, localized("The JSON string or filename defining the transaction to push"))->required();
   add_standard_transaction_options_plus_signing(trxSubcommand);

   trxSubcommand->callback([&] {
      fc::variant trx_var = json_from_file_or_string(trx_to_push);
      try {
         signed_transaction trx = trx_var.as<signed_transaction>();
         std::cout << fc::json::to_pretty_string( push_transaction( trx, signing_keys_opt.get_keys() )) << std::endl;
      } catch( fc::exception& ) {
         // unable to convert so try via abi
         signed_transaction trx;
         abi_serializer::from_variant( trx_var, trx, abi_serializer_resolver, abi_serializer::create_yield_function( abi_serializer_max_time ) );
         std::cout << fc::json::to_pretty_string( push_transaction( trx, signing_keys_opt.get_keys() )) << std::endl;
      }
   });

   // push transactions
   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", localized("Push an array of arbitrary JSON transactions"));
   trxsSubcommand->add_option("transactions", trxsJson, localized("The JSON string or filename defining the array of the transactions to push"))->required();
   trxsSubcommand->callback([&] {
      fc::variant trx_var = json_from_file_or_string(trxsJson);
      auto trxs_result = call(push_txns_func, trx_var);
      std::cout << fc::json::to_pretty_string(trxs_result) << std::endl;
   });


   // multisig subcommand
   auto msig = app.add_subcommand("multisig", localized("Multisig contract commands"));
   msig->require_subcommand();

   // multisig propose
   string proposal_name;
   string requested_perm;
   string transaction_perm;
   string proposed_transaction;
   string proposed_contract;
   string proposed_action;
   string proposer;
   unsigned int proposal_expiration_hours = 24;
   CLI::callback_t parse_expiration_hours = [&](CLI::results_t res) -> bool {
      unsigned int value_s;
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_s)) {
         return false;
      }

      proposal_expiration_hours = static_cast<uint64_t>(value_s);
      return true;
   };

   auto propose_action = msig->add_subcommand("propose", localized("Propose action"));
   add_standard_transaction_options_plus_signing(propose_action, "proposer@active");
   propose_action->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   propose_action->add_option("requested_permissions", requested_perm, localized("The JSON string or filename defining requested permissions"))->required();
   propose_action->add_option("trx_permissions", transaction_perm, localized("The JSON string or filename defining transaction permissions"))->required();
   propose_action->add_option("contract", proposed_contract, localized("The contract to which deferred transaction should be delivered"))->required();
   propose_action->add_option("action", proposed_action, localized("The action of deferred transaction"))->required();
   propose_action->add_option("data", proposed_transaction, localized("The JSON string or filename defining the action to propose"))->required();
   propose_action->add_option("proposer", proposer, localized("Account proposing the transaction"));
   propose_action->add_option("proposal_expiration", parse_expiration_hours, localized("Proposal expiration interval in hours"));

   propose_action->callback([&] {
      fc::variant requested_perm_var = json_from_file_or_string(requested_perm);
      fc::variant transaction_perm_var = json_from_file_or_string(transaction_perm);
      fc::variant trx_var = json_from_file_or_string(proposed_transaction);
      transaction proposed_trx;
      try {
         proposed_trx = trx_var.as<transaction>();
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Invalid transaction format: '${data}'",
                               ("data", fc::json::to_string(trx_var, fc::time_point::maximum())))
      bytes proposed_trx_serialized = variant_to_bin( name(proposed_contract), name(proposed_action), trx_var );

      vector<permission_level> reqperm;
      try {
         reqperm = requested_perm_var.as<vector<permission_level>>();
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Wrong requested permissions format: '${data}'", ("data",requested_perm_var));

      vector<permission_level> trxperm;
      try {
         trxperm = transaction_perm_var.as<vector<permission_level>>();
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Wrong transaction permissions format: '${data}'", ("data",transaction_perm_var));

      auto accountPermissions = get_account_permissions(tx_permission);
      if (accountPermissions.empty()) {
         if (!proposer.empty()) {
            accountPermissions = vector<permission_level>{{name(proposer), config::active_name}};
         } else {
            EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <proposer> or -p)");
         }
      }
      if (proposer.empty()) {
         proposer = name(accountPermissions.at(0).actor).to_string();
      }

      transaction trx;

      trx.expiration = fc::time_point_sec( fc::time_point::now() + fc::hours(proposal_expiration_hours) );
      trx.ref_block_num = 0;
      trx.ref_block_prefix = 0;
      trx.max_net_usage_words = 0;
      trx.max_cpu_usage_ms = 0;
      trx.delay_sec = 0;
      trx.actions = { chain::action(trxperm, name(proposed_contract), name(proposed_action), proposed_trx_serialized) };

      fc::to_variant(trx, trx_var);

      auto args = fc::mutable_variant_object()
         ("proposer", proposer )
         ("proposal_name", proposal_name)
         ("requested", requested_perm_var)
         ("trx", trx_var);

      send_actions({chain::action{accountPermissions, N(eosio.msig), N(propose), variant_to_bin( N(eosio.msig), N(propose), args ) }}, signing_keys_opt.get_keys());
   });

   //multisig propose transaction
   auto propose_trx = msig->add_subcommand("propose_trx", localized("Propose transaction"));
   add_standard_transaction_options_plus_signing(propose_trx, "proposer@active");
   propose_trx->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   propose_trx->add_option("requested_permissions", requested_perm, localized("The JSON string or filename defining requested permissions"))->required();
   propose_trx->add_option("transaction", trx_to_push, localized("The JSON string or filename defining the transaction to push"))->required();
   propose_trx->add_option("proposer", proposer, localized("Account proposing the transaction"));

   propose_trx->callback([&] {
      fc::variant requested_perm_var = json_from_file_or_string(requested_perm);
      fc::variant trx_var = json_from_file_or_string(trx_to_push);

      auto accountPermissions = get_account_permissions(tx_permission);
      if (accountPermissions.empty()) {
         if (!proposer.empty()) {
            accountPermissions = vector<permission_level>{{name(proposer), config::active_name}};
         } else {
            EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <proposer> or -p)");
         }
      }
      if (proposer.empty()) {
         proposer = name(accountPermissions.at(0).actor).to_string();
      }

      auto args = fc::mutable_variant_object()
         ("proposer", proposer )
         ("proposal_name", proposal_name)
         ("requested", requested_perm_var)
         ("trx", trx_var);

      send_actions({chain::action{accountPermissions, N(eosio.msig), N(propose), variant_to_bin( N(eosio.msig), N(propose), args ) }}, signing_keys_opt.get_keys());
   });


   // multisig review
   bool show_approvals_in_multisig_review = false;
   auto review = msig->add_subcommand("review", localized("Review transaction"));
   review->add_option("proposer", proposer, localized("The proposer name (string)"))->required();
   review->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   review->add_flag( "--show-approvals", show_approvals_in_multisig_review, localized("Show the status of the approvals requested within the proposal") );

   review->callback([&] {
      const auto result1 = call(get_table_func, fc::mutable_variant_object("json", true)
                                 ("code", "eosio.msig")
                                 ("scope", proposer)
                                 ("table", "proposal")
                                 ("table_key", "")
                                 ("lower_bound", name(proposal_name).to_uint64_t())
                                 ("upper_bound", name(proposal_name).to_uint64_t() + 1)
                                 // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                                 // Change to name(proposal_name).value when cleos no longer needs to support nodeos versions older than 1.5.0
                                 ("limit", 1)
                           );
      //std::cout << fc::json::to_pretty_string(result) << std::endl;

      const auto& rows1 = result1.get_object()["rows"].get_array();
      // Condition in if statement below can simply be rows.empty() when cleos no longer needs to support nodeos versions older than 1.5.0
      if( rows1.empty() || rows1[0].get_object()["proposal_name"] != proposal_name ) {
         std::cerr << "Proposal not found" << std::endl;
         return;
      }

      const auto& proposal_object = rows1[0].get_object();

      enum class approval_status {
         unapproved,
         approved,
         invalidated
      };

      std::map<permission_level, std::pair<fc::time_point, approval_status>>                               all_approvals;
      std::map<eosio::account_name, std::pair<fc::time_point, vector<decltype(all_approvals)::iterator>>>  provided_approvers;

      bool new_multisig = true;
      if( show_approvals_in_multisig_review ) {
         fc::variants rows2;

         try {
            const auto& result2 = call(get_table_func, fc::mutable_variant_object("json", true)
                                       ("code", "eosio.msig")
                                       ("scope", proposer)
                                       ("table", "approvals2")
                                       ("table_key", "")
                                       ("lower_bound", name(proposal_name).to_uint64_t())
                                       ("upper_bound", name(proposal_name).to_uint64_t() + 1)
                                       // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                                       // Change to name(proposal_name).value when cleos no longer needs to support nodeos versions older than 1.5.0
                                       ("limit", 1)
                                 );
            rows2 = result2.get_object()["rows"].get_array();
         } catch( ... ) {
            new_multisig = false;
         }

         if( !rows2.empty() && rows2[0].get_object()["proposal_name"] == proposal_name ) {
            const auto& approvals_object = rows2[0].get_object();

            for( const auto& ra : approvals_object["requested_approvals"].get_array() ) {
               const auto& ra_obj = ra.get_object();
               auto pl = ra["level"].as<permission_level>();
               all_approvals.emplace( pl, std::make_pair(ra["time"].as<fc::time_point>(), approval_status::unapproved) );
            }

            for( const auto& pa : approvals_object["provided_approvals"].get_array() ) {
               const auto& pa_obj = pa.get_object();
               auto pl = pa["level"].as<permission_level>();
               auto res = all_approvals.emplace( pl, std::make_pair(pa["time"].as<fc::time_point>(), approval_status::approved) );
               provided_approvers[pl.actor].second.push_back( res.first );
            }
         } else {
            const auto result3 = call(get_table_func, fc::mutable_variant_object("json", true)
                                       ("code", "eosio.msig")
                                       ("scope", proposer)
                                       ("table", "approvals")
                                       ("table_key", "")
                                       ("lower_bound", name(proposal_name).to_uint64_t())
                                       ("upper_bound", name(proposal_name).to_uint64_t() + 1)
                                       // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                                       // Change to name(proposal_name).value when cleos no longer needs to support nodeos versions older than 1.5.0
                                       ("limit", 1)
                                 );
            const auto& rows3 = result3.get_object()["rows"].get_array();
            if( rows3.empty() || rows3[0].get_object()["proposal_name"] != proposal_name ) {
               std::cerr << "Proposal not found" << std::endl;
               return;
            }

            const auto& approvals_object = rows3[0].get_object();

            for( const auto& ra : approvals_object["requested_approvals"].get_array() ) {
               auto pl = ra.as<permission_level>();
               all_approvals.emplace( pl, std::make_pair(fc::time_point{}, approval_status::unapproved) );
            }

            for( const auto& pa : approvals_object["provided_approvals"].get_array() ) {
               auto pl = pa.as<permission_level>();
               auto res = all_approvals.emplace( pl, std::make_pair(fc::time_point{}, approval_status::approved) );
               provided_approvers[pl.actor].second.push_back( res.first );
            }
         }

         if( new_multisig ) {
            for( auto& a : provided_approvers ) {
               const auto result4 = call(get_table_func, fc::mutable_variant_object("json", true)
                                          ("code", "eosio.msig")
                                          ("scope", "eosio.msig")
                                          ("table", "invals")
                                          ("table_key", "")
                                          ("lower_bound", a.first.to_uint64_t())
                                          ("upper_bound", a.first.to_uint64_t() + 1)
                                          // Less than ideal upper_bound usage preserved so cleos can still work with old buggy nodeos versions
                                          // Change to name(proposal_name).value when cleos no longer needs to support nodeos versions older than 1.5.0
                                          ("limit", 1)
                                    );
               const auto& rows4 = result4.get_object()["rows"].get_array();
               if( rows4.empty() || rows4[0].get_object()["account"].as<eosio::name>() != a.first ) {
                  continue;
               }

               auto invalidation_time = rows4[0].get_object()["last_invalidation_time"].as<fc::time_point>();
               a.second.first = invalidation_time;

               for( auto& itr : a.second.second ) {
                  if( invalidation_time >= itr->second.first ) {
                     itr->second.second = approval_status::invalidated;
                  }
               }
            }
         }
      }

      auto trx_hex = proposal_object["packed_transaction"].as_string();
      vector<char> trx_blob(trx_hex.size()/2);
      fc::from_hex(trx_hex, trx_blob.data(), trx_blob.size());
      transaction trx = fc::raw::unpack<transaction>(trx_blob);

      fc::mutable_variant_object obj;
      obj["proposer"] = proposer;
      obj["proposal_name"] = proposal_object["proposal_name"];
      obj["transaction_id"] = trx.id();

      for( const auto& entry : proposal_object ) {
         if( entry.key() == "proposal_name" ) continue;
         obj.set( entry.key(), entry.value() );
      }

      fc::variant trx_var;
      abi_serializer abi;
      abi.to_variant(trx, trx_var, abi_serializer_resolver, abi_serializer::create_yield_function( abi_serializer_max_time ));
      obj["transaction"] = trx_var;

      if( show_approvals_in_multisig_review ) {
         fc::variants approvals;

         for( const auto& approval : all_approvals ) {
            fc::mutable_variant_object approval_obj;
            approval_obj["level"] = approval.first;
            switch( approval.second.second ) {
               case approval_status::unapproved:
               {
                  approval_obj["status"] = "unapproved";
                  if( approval.second.first != fc::time_point{} ) {
                     approval_obj["last_unapproval_time"] = approval.second.first;
                  }
               }
               break;
               case approval_status::approved:
               {
                  approval_obj["status"] = "approved";
                  if( new_multisig ) {
                     approval_obj["last_approval_time"] = approval.second.first;
                  }
               }
               break;
               case approval_status::invalidated:
               {
                  approval_obj["status"] = "invalidated";
                  approval_obj["last_approval_time"] = approval.second.first;
                  approval_obj["invalidation_time"] = provided_approvers[approval.first.actor].first;
               }
               break;
            }

            approvals.push_back( std::move(approval_obj) );
         }

         obj["approvals"] = std::move(approvals);
      }

      std::cout << fc::json::to_pretty_string(obj) << std::endl;
   });

   string perm;
   string proposal_hash;
   auto approve_or_unapprove = [&](const string& action) {
      fc::variant perm_var = json_from_file_or_string(perm);

      auto args = fc::mutable_variant_object()
         ("proposer", proposer)
         ("proposal_name", proposal_name)
         ("level", perm_var);

      if( proposal_hash.size() ) {
         args("proposal_hash", proposal_hash);
      }

      auto accountPermissions = get_account_permissions(tx_permission, {name(proposer), config::active_name});
      send_actions({chain::action{accountPermissions, N(eosio.msig), name(action), variant_to_bin( N(eosio.msig), name(action), args ) }}, signing_keys_opt.get_keys());
   };

   // multisig approve
   auto approve = msig->add_subcommand("approve", localized("Approve proposed transaction"));
   add_standard_transaction_options_plus_signing(approve, "proposer@active");
   approve->add_option("proposer", proposer, localized("The proposer name (string)"))->required();
   approve->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   approve->add_option("permissions", perm, localized("The JSON string of filename defining approving permissions"))->required();
   approve->add_option("proposal_hash", proposal_hash, localized("Hash of proposed transaction (i.e. transaction ID) to optionally enforce as a condition of the approval"));
   approve->callback([&] { approve_or_unapprove("approve"); });

   // multisig unapprove
   auto unapprove = msig->add_subcommand("unapprove", localized("Unapprove proposed transaction"));
   add_standard_transaction_options_plus_signing(unapprove, "proposer@active");
   unapprove->add_option("proposer", proposer, localized("The proposer name (string)"))->required();
   unapprove->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   unapprove->add_option("permissions", perm, localized("The JSON string of filename defining approving permissions"))->required();
   unapprove->callback([&] { approve_or_unapprove("unapprove"); });

   // multisig invalidate
   string invalidator;
   auto invalidate = msig->add_subcommand("invalidate", localized("Invalidate all multisig approvals of an account"));
   add_standard_transaction_options_plus_signing(invalidate, "invalidator@active");
   invalidate->add_option("invalidator", invalidator, localized("Invalidator name (string)"))->required();
   invalidate->callback([&] {
      auto args = fc::mutable_variant_object()
         ("account", invalidator);

      auto accountPermissions = get_account_permissions(tx_permission, {name(invalidator), config::active_name});
      send_actions({chain::action{accountPermissions, N(eosio.msig), N(invalidate), variant_to_bin( N(eosio.msig), N(invalidate), args ) }}, signing_keys_opt.get_keys());
   });

   // multisig cancel
   string canceler;
   auto cancel = msig->add_subcommand("cancel", localized("Cancel proposed transaction"));
   add_standard_transaction_options_plus_signing(cancel, "canceler@active");
   cancel->add_option("proposer", proposer, localized("The proposer name (string)"))->required();
   cancel->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   cancel->add_option("canceler", canceler, localized("The canceler name (string)"));
   cancel->callback([&]() {
      auto accountPermissions = get_account_permissions(tx_permission);
      if (accountPermissions.empty()) {
         if (!canceler.empty()) {
            accountPermissions = vector<permission_level>{{name(canceler), config::active_name}};
         } else {
            EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <canceler> or -p)");
         }
      }
      if (canceler.empty()) {
         canceler = name(accountPermissions.at(0).actor).to_string();
      }
      auto args = fc::mutable_variant_object()
         ("proposer", proposer)
         ("proposal_name", proposal_name)
         ("canceler", canceler);

      send_actions({chain::action{accountPermissions, N(eosio.msig), N(cancel), variant_to_bin( N(eosio.msig), N(cancel), args ) }}, signing_keys_opt.get_keys());
      }
   );

   // multisig exec
   string executer;
   auto exec = msig->add_subcommand("exec", localized("Execute proposed transaction"));
   add_standard_transaction_options_plus_signing(exec, "executer@active");
   exec->add_option("proposer", proposer, localized("The proposer name (string)"))->required();
   exec->add_option("proposal_name", proposal_name, localized("The proposal name (string)"))->required();
   exec->add_option("executer", executer, localized("The account paying for execution (string)"));
   exec->callback([&] {
      auto accountPermissions = get_account_permissions(tx_permission);
      if (accountPermissions.empty()) {
         if (!executer.empty()) {
            accountPermissions = vector<permission_level>{{name(executer), config::active_name}};
         } else {
            EOS_THROW(missing_auth_exception, "Authority is not provided (either by multisig parameter <executer> or -p)");
         }
      }
      if (executer.empty()) {
         executer = name(accountPermissions.at(0).actor).to_string();
      }

      auto args = fc::mutable_variant_object()
         ("proposer", proposer )
         ("proposal_name", proposal_name)
         ("executer", executer);

      send_actions({chain::action{accountPermissions, N(eosio.msig), N(exec), variant_to_bin( N(eosio.msig), N(exec), args ) }}, signing_keys_opt.get_keys());
      }
   );

   // wrap subcommand
   auto wrap = app.add_subcommand("wrap", localized("Wrap contract commands"));
   wrap->require_subcommand();

   // wrap exec
   string wrap_con = "eosio.wrap";
   executer = "";
   string trx_to_exec;
   auto wrap_exec = wrap->add_subcommand("exec", localized("Execute a transaction while bypassing authorization checks"));
   add_standard_transaction_options_plus_signing(wrap_exec, "executer@active & --contract@active");
   wrap_exec->add_option("executer", executer, localized("Account executing the transaction and paying for the deferred transaction RAM"))->required();
   wrap_exec->add_option("transaction", trx_to_exec, localized("The JSON string or filename defining the transaction to execute"))->required();
   wrap_exec->add_option("--contract,-c", wrap_con, localized("The account which controls the wrap contract"));

   wrap_exec->callback([&] {
      fc::variant trx_var = json_from_file_or_string(trx_to_exec);

      auto accountPermissions = get_account_permissions(tx_permission);
      if( accountPermissions.empty() ) {
         accountPermissions = vector<permission_level>{{name(executer), config::active_name}, {name(wrap_con), config::active_name}};
      }

      auto args = fc::mutable_variant_object()
         ("executer", executer )
         ("trx", trx_var);

      send_actions({chain::action{accountPermissions, name(wrap_con), N(exec), variant_to_bin( name(wrap_con), N(exec), args ) }}, signing_keys_opt.get_keys());
   });

   // system subcommand
   auto system = app.add_subcommand("system", localized("Send eosio.system contract action to the blockchain."));
   system->require_subcommand();

   auto createAccountSystem = create_account_subcommand( system, false /*simple*/ );
   auto registerProducer = register_producer_subcommand(system);
   auto unregisterProducer = unregister_producer_subcommand(system);

   auto voteProducer = system->add_subcommand("voteproducer", localized("Vote for a producer"));
   voteProducer->require_subcommand();
   auto voteProxy = vote_producer_proxy_subcommand(voteProducer);
   auto voteProducers = vote_producers_subcommand(voteProducer);
   auto approveProducer = approve_producer_subcommand(voteProducer);
   auto unapproveProducer = unapprove_producer_subcommand(voteProducer);

   auto listProducers = list_producers_subcommand(system);

   auto delegateBandWidth = delegate_bandwidth_subcommand(system);
   auto undelegateBandWidth = undelegate_bandwidth_subcommand(system);
   auto listBandWidth = list_bw_subcommand(system);
   auto bidname = bidname_subcommand(system);
   auto bidnameinfo = bidname_info_subcommand(system);

   auto buyram = buyram_subcommand(system);
   auto sellram = sellram_subcommand(system);

   auto claimRewards = claimrewards_subcommand(system);

   auto regProxy = regproxy_subcommand(system);
   auto unregProxy = unregproxy_subcommand(system);

   auto cancelDelay = canceldelay_subcommand(system);

   auto rex = system->add_subcommand("rex", localized("Actions related to REX (the resource exchange)"));
   rex->require_subcommand();
   auto deposit        = deposit_subcommand(rex);
   auto withdraw       = withdraw_subcommand(rex);
   auto buyrex         = buyrex_subcommand(rex);
   auto lendrex        = lendrex_subcommand(rex);
   auto unstaketorex   = unstaketorex_subcommand(rex);
   auto sellrex        = sellrex_subcommand(rex);
   auto cancelrexorder = cancelrexorder_subcommand(rex);
   auto mvtosavings    = mvtosavings_subcommand(rex);
   auto mvfromsavings  = mvfrsavings_subcommand(rex);
   auto rentcpu        = rentcpu_subcommand(rex);
   auto rentnet        = rentnet_subcommand(rex);
   auto fundcpuloan    = fundcpuloan_subcommand(rex);
   auto fundnetloan    = fundnetloan_subcommand(rex);
   auto defcpuloan     = defcpuloan_subcommand(rex);
   auto defnetloan     = defnetloan_subcommand(rex);
   auto consolidate    = consolidate_subcommand(rex);
   auto updaterex      = updaterex_subcommand(rex);
   auto rexexec        = rexexec_subcommand(rex);
   auto closerex       = closerex_subcommand(rex);


   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const explained_exception& e) {
      return 1;
   } catch (connection_exception& e) {
      if (verbose) {
         elog("connect error: ${e}", ("e", e.to_detail_string()));
      }
      return 1;
   } catch (const fc::exception& e) {
      // attempt to extract the error code if one is present
      if (!print_recognized_errors(e, verbose)) {
         // Error is not recognized
         if (!print_help_text(e) || verbose) {
            elog("Failed with error: ${e}", ("e", verbose ? e.to_detail_string() : e.to_string()));
         }
      }
      return 1;
   }

   return 0;
}
