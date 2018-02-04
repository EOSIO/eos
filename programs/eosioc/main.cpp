/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @defgroup eosclienttool EOS Command Line Client Reference
 *  @brief Tool for sending transactions and querying state from @ref eosd
 *  @ingroup eosclienttool
 */

/**
  @defgroup eosclienttool

  @section intro Introduction to EOSC

  `eosc` is a command line tool that interfaces with the REST api exposed by @ref eosd. In order to use `eosc` you will need to
  have a local copy of `eosd` running and configured to load the 'eosio::chain_api_plugin'.

   eosc contains documentation for all of its commands. For a list of all commands known to eosc, simply run it with no arguments:
```
$ ./eosc
Command Line Interface to Eos Daemon
Usage: ./eosc [OPTIONS] SUBCOMMAND

Options:
  -h,--help                   Print this help actions and exit
  -H,--host TEXT=localhost    the host where eosd is running
  -p,--port UINT=8888         the port where eosd is running
  --wallet-host TEXT=localhost
                              the host where eos-walletd is running
  --wallet-port UINT=8888     the port where eos-walletd is running

Subcommands:
  create                      Create various items, on and off the blockchain
  get                         Retrieve various items and information from the blockchain
  set                         Set or update blockchain state
  transfer                    Transfer EOS from account to account
  wallet                      Interact with local wallet
  benchmark                   Configure and execute benchmarks
  push                        Push arbitrary transactions to the blockchain

```
To get help with any particular subcommand, run it with no arguments as well:
```
$ ./eosc create
Create various items, on and off the blockchain
Usage: ./eosc create SUBCOMMAND

Subcommands:
  key                         Create a new keypair and print the public and private keys
  account                     Create a new account on the blockchain

$ ./eosc create account
Create a new account on the blockchain
Usage: ./eosc create account [OPTIONS] creator name OwnerKey ActiveKey

Positionals:
  creator TEXT                The name of the account creating the new account
  name TEXT                   The name of the new account
  OwnerKey TEXT               The owner public key for the account
  ActiveKey TEXT              The active public key for the account

Options:
  -s,--skip-signature         Specify that unlocked wallet keys should not be used to sign transaction
```
*/
#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/utilities/key_conversion.hpp>

#include <eosio/chain/config.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <fc/io/fstream.hpp>

#include "CLI11.hpp"
#include "help_text.hpp"
#include "localize.hpp"
#include "config.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::utilities;
using namespace eosio::client::help;
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

string program = "eosc";
string host = "localhost";
uint32_t port = 8888;

// restricting use of wallet to localhost
string wallet_host = "localhost";
uint32_t wallet_port = 8888;

const string chain_func_base = "/v1/chain";
const string get_info_func = chain_func_base + "/get_info";
const string push_txn_func = chain_func_base + "/push_transaction";
const string push_txns_func = chain_func_base + "/push_transactions";
const string json_to_bin_func = chain_func_base + "/abi_json_to_bin";
const string get_block_func = chain_func_base + "/get_block";
const string get_account_func = chain_func_base + "/get_account";
const string get_table_func = chain_func_base + "/get_table_rows";
const string get_code_func = chain_func_base + "/get_code";
const string get_currency_balance_func = chain_func_base + "/get_currency_balance";
const string get_currency_stats_func = chain_func_base + "/get_currency_stats";
const string get_required_keys = chain_func_base + "/get_required_keys";

const string account_history_func_base = "/v1/account_history";
const string get_transaction_func = account_history_func_base + "/get_transaction";
const string get_transactions_func = account_history_func_base + "/get_transactions";
const string get_key_accounts_func = account_history_func_base + "/get_key_accounts";
const string get_controlled_accounts_func = account_history_func_base + "/get_controlled_accounts";

const string net_func_base = "/v1/net";
const string net_connect = net_func_base + "/connect";
const string net_disconnect = net_func_base + "/disconnect";
const string net_status = net_func_base + "/status";
const string net_connections = net_func_base + "/connections";


const string wallet_func_base = "/v1/wallet";
const string wallet_create = wallet_func_base + "/create";
const string wallet_open = wallet_func_base + "/open";
const string wallet_list = wallet_func_base + "/list_wallets";
const string wallet_list_keys = wallet_func_base + "/list_keys";
const string wallet_public_keys = wallet_func_base + "/get_public_keys";
const string wallet_lock = wallet_func_base + "/lock";
const string wallet_lock_all = wallet_func_base + "/lock_all";
const string wallet_unlock = wallet_func_base + "/unlock";
const string wallet_import_key = wallet_func_base + "/import_key";
const string wallet_sign_trx = wallet_func_base + "/sign_transaction";

inline std::vector<name> sort_names( const std::vector<name>& names ) {
   auto results = std::vector<name>(names);
   std::sort( results.begin(), results.end() );
   auto itr = std::unique( results.begin(), results.end() );
   results.erase( itr, results.end() );
   return results;
}

vector<uint8_t> assemble_wast( const std::string& wast ) {
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << localized("Error parsing WebAssembly text file:") << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_THROW_EXCEPTION( explained_exception, "wast parse error" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << localized("Error serializing WebAssembly binary file:") << std::endl;
      std::cerr << exception.message << std::endl;
      FC_THROW_EXCEPTION( explained_exception, "wasm serialize error");
   }
}

auto tx_expiration = fc::seconds(30);
bool tx_force_unique = false;
void add_standard_transaction_options(CLI::App* cmd) {
   CLI::callback_t parse_exipration = [](CLI::results_t res) -> bool {
      double value_s;
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_s)) {
         return false;
      }
      
      tx_expiration = fc::seconds(static_cast<uint64_t>(value_s));
      return true;
   };

   cmd->add_option("-x,--expiration", parse_exipration, localized("set the time in seconds before a transaction expires, defaults to 30s"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localized("force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times"));
}

uint64_t generate_nonce_value() {
   return fc::time_point::now().time_since_epoch().count();
}

chain::action generate_nonce() {
   auto v = generate_nonce_value();
   return chain::action( {}, config::eosio_system_acount_name, "nonce", fc::raw::pack(v));
}

vector<chain::permission_level> get_account_permissions(const vector<string>& permissions) {
   auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
      vector<string> pieces;
      split(pieces, p, boost::algorithm::is_any_of("@"));
      EOSC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
      return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
   });
   vector<chain::permission_level> accountPermissions;
   boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
   return accountPermissions;
}

fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const fc::variant& postdata = fc::variant() );


template<typename T>
fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const T& v ) { return call( server, port, path, fc::variant(v) ); }

template<typename T>
fc::variant call( const std::string& path,
                  const T& v ) { return call( host, port, path, fc::variant(v) ); }

eosio::chain_apis::read_only::get_info_results get_info() {
  return call(host, port, get_info_func ).as<eosio::chain_apis::read_only::get_info_results>();
}

void sign_transaction(signed_transaction& trx) {
   // TODO better error checking
   const auto& public_keys = call(wallet_host, wallet_port, wallet_public_keys);
   auto get_arg = fc::mutable_variant_object
         ("transaction", (transaction)trx)
         ("available_keys", public_keys);
   const auto& required_keys = call(host, port, get_required_keys, get_arg);
   // TODO determine chain id
   fc::variants sign_args = {fc::variant(trx), required_keys["required_keys"], fc::variant(chain_id_type{})};
   const auto& signed_trx = call(wallet_host, wallet_port, wallet_sign_trx, sign_args);
   trx = signed_trx.as<signed_transaction>();
}

fc::variant push_transaction( signed_transaction& trx, bool skip_sign, packed_transaction::compression_type compression = packed_transaction::none ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + tx_expiration;
    trx.set_reference_block(info.head_block_id);

    if (!skip_sign) {
       sign_transaction(trx);
    }

    return call( push_txn_func, packed_transaction(trx, compression) );
}

fc::variant push_actions(std::vector<chain::action>&& actions, bool skip_sign, packed_transaction::compression_type compression = packed_transaction::none ) {
   signed_transaction trx;
   trx.actions = std::forward<decltype(actions)>(actions);

   return push_transaction(trx, skip_sign, compression);
}

void send_actions(std::vector<chain::action>&& actions, bool skip_sign, packed_transaction::compression_type compression = packed_transaction::none ) {
   std::cout << fc::json::to_pretty_string(push_actions(std::forward<decltype(actions)>(actions), skip_sign, compression)) << std::endl;
}

void send_transaction( signed_transaction& trx, bool skip_sign, packed_transaction::compression_type compression = packed_transaction::none  ) {
   std::cout << fc::json::to_pretty_string(push_transaction(trx, skip_sign, compression)) << std::endl;
}

void create_account(name creator, name newaccount, public_key_type owner, public_key_type active, bool skip_sign) {
      auto owner_auth   = eosio::chain::authority{1, {{owner, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{active, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      vector<chain::action> actions;
      actions.emplace_back( vector<chain::permission_level>{{creator,"active"}},
                                contracts::newaccount{creator, newaccount, owner_auth, active_auth, recovery_auth});

      send_actions(std::move(actions), skip_sign);
}

chain::action create_updateauth(const name& account, const name& permission, const name& parent, const authority& auth, const name& permissionAuth) {
   return action { vector<chain::permission_level>{{account,permissionAuth}},
                   contracts::updateauth{account, permission, parent, auth}};
}

chain::action create_deleteauth(const name& account, const name& permission, const name& permissionAuth) {
   return action { vector<chain::permission_level>{{account,permissionAuth}},
                   contracts::deleteauth{account, permission}};
}

chain::action create_linkauth(const name& account, const name& code, const name& type, const name& requirement) {
   return action { vector<chain::permission_level>{{account,"active"}},
                   contracts::linkauth{account, code, type, requirement}};
}

chain::action create_unlinkauth(const name& account, const name& code, const name& type) {
   return action { vector<chain::permission_level>{{account,"active"}},
                   contracts::unlinkauth{account, code, type}};
}

struct set_account_permission_subcommand {
   string accountStr;
   string permissionStr;
   string authorityJsonOrFile;
   string parentStr;
   string permissionAuth = "active";
   bool skip_sign;

   set_account_permission_subcommand(CLI::App* accountCmd) {
      auto permissions = accountCmd->add_subcommand("permission", localized("set parmaters dealing with account permissions"));
      permissions->add_option("-p,--permission", permissionAuth,localized("Permission level to authorize, (Defaults to: 'active'"));
      permissions->add_option("account", accountStr, localized("The account to set/delete a permission authority for"))->required();
      permissions->add_option("permission", permissionStr, localized("The permission name to set/delete an authority for"))->required();
      permissions->add_option("authority", authorityJsonOrFile, localized("[delete] NULL, [create/update] JSON string or filename defining the authority"))->required();
      permissions->add_option("parent", parentStr, localized("[create] The permission name of this parents permission (Defaults to: \"Active\")"));
      permissions->add_flag("-s,--skip-sign", skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
      add_standard_transaction_options(permissions);

      permissions->set_callback([this] {
         name account = name(accountStr);
         name permission = name(permissionStr);
         bool is_delete = boost::iequals(authorityJsonOrFile, "null");
         
         if (is_delete) {
            send_actions({create_deleteauth(account, permission, name(permissionAuth))}, skip_sign);
         } else {
            authority auth;
            if (boost::istarts_with(authorityJsonOrFile, "EOS")) {
               auth = authority(public_key_type(authorityJsonOrFile));
            } else {
               fc::variant parsedAuthority;
               if (boost::istarts_with(authorityJsonOrFile, "{")) {
                  parsedAuthority = fc::json::from_string(authorityJsonOrFile);
               } else {
                  parsedAuthority = fc::json::from_file(authorityJsonOrFile);
               }

               auth = parsedAuthority.as<authority>();
            }

            name parent;
            if (parentStr.size() == 0 && permissionStr != "owner") {
               // see if we can auto-determine the proper parent
               const auto account_result = call(get_account_func, fc::mutable_variant_object("account_name", accountStr));
               const auto& existing_permissions = account_result.get_object()["permissions"].get_array();
               auto permissionPredicate = [this](const auto& perm) { 
                  return perm.is_object() && 
                        perm.get_object().contains("permission") &&
                        boost::equals(perm.get_object()["permission"].get_string(), permissionStr); 
               };

               auto itr = boost::find_if(existing_permissions, permissionPredicate);
               if (itr != existing_permissions.end()) {
                  parent = name((*itr).get_object()["parent"].get_string());
               } else {
                  // if this is a new permission and there is no parent we default to "active"
                  parent = name("active");

               }
            } else {
               parent = name(parentStr);
            }

            send_actions({create_updateauth(account, permission, parent, auth, name(permissionAuth))}, skip_sign);
         }      
      });
   }
   
};

struct set_action_permission_subcommand {
   string accountStr;
   string codeStr;
   string typeStr;
   string requirementStr;
   bool skip_sign;

   set_action_permission_subcommand(CLI::App* actionRoot) {
      auto permissions = actionRoot->add_subcommand("permission", localized("set parmaters dealing with account permissions"));
      permissions->add_option("account", accountStr, localized("The account to set/delete a permission authority for"))->required();
      permissions->add_option("code", codeStr, localized("The account that owns the code for the action"))->required();
      permissions->add_option("type", typeStr, localized("the type of the action"))->required();
      permissions->add_option("requirement", requirementStr, localized("[delete] NULL, [set/update] The permission name require for executing the given action"))->required();
      permissions->add_flag("-s,--skip-sign", skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
      add_standard_transaction_options(permissions);

      permissions->set_callback([this] {
         name account = name(accountStr);
         name code = name(codeStr);
         name type = name(typeStr);
         bool is_delete = boost::iequals(requirementStr, "null");
         
         if (is_delete) {
            send_actions({create_unlinkauth(account, code, type)}, skip_sign);
         } else {
            name requirement = name(requirementStr);
            send_actions({create_linkauth(account, code, type, requirement)}, skip_sign);
         }      
      });
   }
};

int main( int argc, char** argv ) {
   fc::path binPath = argv[0];
   if (binPath.is_relative()) {
      binPath = relative(binPath, current_path()); 
   }

   setlocale(LC_ALL, "");
   bindtextdomain(locale_domain, locale_path);
   textdomain(locale_domain);

   CLI::App app{"Command Line Interface to Eos Client"};
   app.require_subcommand();
   app.add_option( "-H,--host", host, localized("the host where eosd is running"), true );
   app.add_option( "-p,--port", port, localized("the port where eosd is running"), true );
   app.add_option( "--wallet-host", wallet_host, localized("the host where eos-walletd is running"), true );
   app.add_option( "--wallet-port", wallet_port, localized("the port where eos-walletd is running"), true );

   bool verbose_errors = false;
   app.add_flag( "-v,--verbose", verbose_errors, localized("output verbose actions on error"));

   auto version = app.add_subcommand("version", localized("Retrieve version information"), false);
   version->require_subcommand();

   version->add_subcommand("client", localized("Retrieve version information of the client"))->set_callback([] {
     std::cout << localized("Build version: ${ver}", ("ver", eosio::client::config::version_str)) << std::endl;
   });

   // Create subcommand
   auto create = app.add_subcommand("create", localized("Create various items, on and off the blockchain"), false);
   create->require_subcommand();

   // create key
   create->add_subcommand("key", localized("Create a new keypair and print the public and private keys"))->set_callback( [](){
      auto pk    = private_key_type::generate();
      auto privs = string(pk);
      auto pubs  = string(pk.get_public_key());
      std::cout << localized("Private key: ${key}", ("key",  privs) ) << std::endl;
      std::cout << localized("Public key: ${key}", ("key", pubs ) ) << std::endl;
   });

   // create account
   string creator;
   string account_name;
   string ownerKey;
   string activeKey;
   bool skip_sign = false;
   auto createAccount = create->add_subcommand("account", localized("Create a new account on the blockchain"), false);
   createAccount->add_option("creator", creator, localized("The name of the account creating the new account"))->required();
   createAccount->add_option("name", account_name, localized("The name of the new account"))->required();
   createAccount->add_option("OwnerKey", ownerKey, localized("The owner public key for the account"))->required();
   createAccount->add_option("ActiveKey", activeKey, localized("The active public key for the account"))->required();
   createAccount->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   add_standard_transaction_options(createAccount);
   createAccount->set_callback([&] {
                   create_account(creator, account_name, public_key_type(ownerKey), public_key_type(activeKey), skip_sign);
   });

   // Get subcommand
   auto get = app.add_subcommand("get", localized("Retrieve various items and information from the blockchain"), false);
   get->require_subcommand();

   // get info
   get->add_subcommand("info", localized("Get current blockchain information"))->set_callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", localized("Retrieve a full block from the blockchain"), false);
   getBlock->add_option("block", blockArg, localized("The number or ID of the block to retrieve"))->required();
   getBlock->set_callback([&blockArg] {
      auto arg = fc::mutable_variant_object("block_num_or_id", blockArg);
      std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
   });

   // get account
   string accountName;
   auto getAccount = get->add_subcommand("account", localized("Retrieve an account from the blockchain"), false);
   getAccount->add_option("name", accountName, localized("The name of the account to retrieve"))->required();
   getAccount->set_callback([&] {
      std::cout << fc::json::to_pretty_string(call(get_account_func,
                                                   fc::mutable_variant_object("account_name", accountName)))
                << std::endl;
   });

   // get code
   string codeFilename;
   string abiFilename;
   auto getCode = get->add_subcommand("code", localized("Retrieve the code and ABI for an account"), false);
   getCode->add_option("name", accountName, localized("The name of the account whose code should be retrieved"))->required();
   getCode->add_option("-c,--code",codeFilename, localized("The name of the file to save the contract .wast to") );
   getCode->add_option("-a,--abi",abiFilename, localized("The name of the file to save the contract .abi to") );
   getCode->set_callback([&] {
      auto result = call(get_code_func, fc::mutable_variant_object("account_name", accountName));

      std::cout << localized("code hash: ${code_hash}", ("code_hash", result["code_hash"].as_string())) << std::endl;

      if( codeFilename.size() ){
         std::cout << localized("saving wast to ${codeFilename}", ("codeFilename", codeFilename)) << std::endl;
         auto code = result["wast"].as_string();
         std::ofstream out( codeFilename.c_str() );
         out << code;
      }
      if( abiFilename.size() ) {
         std::cout << localized("saving abi to ${abiFilename}", ("abiFilename", abiFilename)) << std::endl;
         auto abi  = fc::json::to_pretty_string( result["abi"] );
         std::ofstream abiout( abiFilename.c_str() );
         abiout << abi;
      }
   });

   // get table 
   string scope;
   string code;
   string table;
   string lower;
   string upper;
   bool binary = false;
   uint32_t limit = 10;
   auto getTable = get->add_subcommand( "table", localized("Retrieve the contents of a database table"), false);
   getTable->add_option( "scope", scope, localized("The account scope where the table is found") )->required();
   getTable->add_option( "contract", code, localized("The contract within scope who owns the table") )->required();
   getTable->add_option( "table", table, localized("The name of the table as specified by the contract abi") )->required();
   getTable->add_option( "-b,--binary", binary, localized("Return the value as BINARY rather than using abi to interpret as JSON") );
   getTable->add_option( "-l,--limit", limit, localized("The maximum number of rows to return") );
   getTable->add_option( "-k,--key", limit, localized("The name of the key to index by as defined by the abi, defaults to primary key") );
   getTable->add_option( "-L,--lower", lower, localized("JSON representation of lower bound value of key, defaults to first") );
   getTable->add_option( "-U,--upper", upper, localized("JSON representation of upper bound value value of key, defaults to last") );

   getTable->set_callback([&] {
      auto result = call(get_table_func, fc::mutable_variant_object("json", !binary)
                         ("scope",scope)
                         ("code",code)
                         ("table",table)
                         );

      std::cout << fc::json::to_pretty_string(result)
                << std::endl;
   });

   // currency accessors
   // get currency balance
   string symbol;
   auto get_currency = get->add_subcommand( "currency", localized("Retrieve information related to standard currencies"), true);
   auto get_balance = get_currency->add_subcommand( "balance", localized("Retrieve the balance of an account for a given currency"), false);
   get_balance->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_balance->add_option( "account", accountName, localized("The account to query balances for") )->required();
   get_balance->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") );
   get_balance->set_callback([&] {
      auto result = call(get_currency_balance_func, fc::mutable_variant_object("json", false)
         ("account", accountName)
         ("code", code)
         ("symbol", symbol)
      );

      const auto& rows = result.get_array();
      if (symbol.empty()) {
         std::cout << fc::json::to_pretty_string(rows)
                   << std::endl;
      } else if ( rows.size() > 0 ){
         std::cout << fc::json::to_pretty_string(rows[0])
                   << std::endl;
      }
   });

   auto get_currency_stats = get_currency->add_subcommand( "stats", localized("Retrieve the stats of for a given currency"), false);
   get_currency_stats->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_currency_stats->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") );
   get_currency_stats->set_callback([&] {
      auto result = call(get_currency_stats_func, fc::mutable_variant_object("json", false)
         ("code", code)
         ("symbol", symbol)
      );

      if (symbol.empty()) {
         std::cout << fc::json::to_pretty_string(result)
                   << std::endl;
      } else {
         const auto& mapping = result.get_object();
         std::cout << fc::json::to_pretty_string(mapping[symbol])
                   << std::endl;
      }
   });

   // get accounts
   string publicKey;
   auto getAccounts = get->add_subcommand("accounts", localized("Retrieve accounts associated with a public key"), false);
   getAccounts->add_option("public_key", publicKey, localized("The public key to retrieve accounts for"))->required();
   getAccounts->set_callback([&] {
      auto arg = fc::mutable_variant_object( "public_key", publicKey);
      std::cout << fc::json::to_pretty_string(call(get_key_accounts_func, arg)) << std::endl;
   });


   // get servants
   string controllingAccount;
   auto getServants = get->add_subcommand("servants", localized("Retrieve accounts which are servants of a given account "), false);
   getServants->add_option("account", controllingAccount, localized("The name of the controlling account"))->required();
   getServants->set_callback([&] {
      auto arg = fc::mutable_variant_object( "controlling_account", controllingAccount);
      std::cout << fc::json::to_pretty_string(call(get_controlled_accounts_func, arg)) << std::endl;
   });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", localized("Retrieve a transaction from the blockchain"), false);
   getTransaction->add_option("id", transactionId, localized("ID of the transaction to retrieve"))->required();
   getTransaction->set_callback([&] {
      auto arg= fc::mutable_variant_object( "transaction_id", transactionId);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get transactions
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"), false);
   getTransactions->add_option("account_name", account_name, localized("name of account to query on"))->required();
   getTransactions->add_option("skip_seq", skip_seq, localized("Number of most recent transactions to skip (0 would start at most recent transaction)"));
   getTransactions->add_option("num_seq", num_seq, localized("Number of transactions to return"));
   getTransactions->set_callback([&] {
      auto arg = (skip_seq.empty())
                  ? fc::mutable_variant_object( "account_name", account_name)
                  : (num_seq.empty())
                     ? fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq)
                     : fc::mutable_variant_object( "account_name", account_name)("skip_seq", skip_seq)("num_seq", num_seq);
      auto result = call(get_transactions_func, arg);
      std::cout << fc::json::to_pretty_string(call(get_transactions_func, arg)) << std::endl;


      const auto& trxs = result.get_object()["transactions"].get_array();
      for( const auto& t : trxs ) {
         const auto& tobj = t.get_object();
         int64_t seq_num  = tobj["seq_num"].as<int64_t>();
         string  id       = tobj["transaction_id"].as_string();
         const auto& trx  = tobj["transaction"].get_object();
         const auto& data = trx["data"].get_object();
         const auto& exp  = data["expiration"].as<fc::time_point_sec>();
         const auto& msgs = data["actions"].get_array();
         std::cout << tobj["seq_num"].as_string() <<"] " << id << "  " << data["expiration"].as_string() << std::endl;
      }

   });

   // set subcommand
   auto setSubcommand = app.add_subcommand("set", localized("Set or update blockchain state"));
   setSubcommand->require_subcommand();

   // set contract subcommand
   string account;
   string wastPath;
   string abiPath;
   auto contractSubcommand = setSubcommand->add_subcommand("contract", localized("Create or update the contract on an account"));
   contractSubcommand->add_option("account", account, localized("The account to publish a contract for"))->required();
   contractSubcommand->add_option("wast-file", wastPath, localized("The file containing the contract WAST or WASM"))->required()
         ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, localized("The ABI for the contract"))
              ->check(CLI::ExistingFile);
   contractSubcommand->add_flag("-s,--skip-sign", skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   add_standard_transaction_options(contractSubcommand);
   contractSubcommand->set_callback([&] {
      std::string wast;
      std::cout << localized("Reading WAST...") << std::endl;
      fc::read_file_contents(wastPath, wast);

      vector<uint8_t> wasm;
      const string binary_wasm_header = "\x00\x61\x73\x6d";
      if(wast.compare(0, 4, binary_wasm_header) == 0) {
         std::cout << localized("Using already assembled WASM...") << std::endl;
         wasm = vector<uint8_t>(wast.begin(), wast.end());
      }
      else {
         std::cout << localized("Assembling WASM...") << std::endl;
         wasm = assemble_wast(wast);
      }

      contracts::setcode handler;
      handler.account = account;
      handler.code.assign(wasm.begin(), wasm.end());

      vector<chain::action> actions;
      actions.emplace_back( vector<chain::permission_level>{{account,"active"}}, handler);

      if (abi->count()) {
         contracts::setabi handler;
         handler.account = account;
         handler.abi = fc::json::from_file(abiPath).as<contracts::abi_def>();
         actions.emplace_back( vector<chain::permission_level>{{account,"active"}}, handler);
      }

      std::cout << localized("Publishing contract...") << std::endl;
      send_actions(std::move(actions), skip_sign, packed_transaction::zlib);
   });

   // set account
   auto setAccount = setSubcommand->add_subcommand("account", localized("set or update blockchain account state"))->require_subcommand();

   // set account permission
   auto setAccountPermission = set_account_permission_subcommand(setAccount);

   // set action
   auto setAction = setSubcommand->add_subcommand("action", localized("set or update blockchain action state"))->require_subcommand();
   
   // set action permission
   auto setActionPermission = set_action_permission_subcommand(setAction);

   // Transfer subcommand
   string sender;
   string recipient;
   uint64_t amount;
   string memo;
   auto transfer = app.add_subcommand("transfer", localized("Transfer EOS from account to account"), false);
   transfer->add_option("sender", sender, localized("The account sending EOS"))->required();
   transfer->add_option("recipient", recipient, localized("The account receiving EOS"))->required();
   transfer->add_option("amount", amount, localized("The amount of EOS to send"))->required();
   transfer->add_option("memo", memo, localized("The memo for the transfer"));
   transfer->add_flag("-s,--skip-sign", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   add_standard_transaction_options(transfer);
   transfer->set_callback([&] {
      auto transfer = fc::mutable_variant_object
            ("from", sender)
            ("to", recipient)
            ("quantity", asset(amount))
            ("memo", memo);
      auto args = fc::mutable_variant_object
            ("code", name(config::eosio_system_acount_name))
            ("action", "transfer")
            ("args", transfer);

      auto result = call(json_to_bin_func, args);

      std::vector<chain::action> actions;
      actions.emplace_back(vector<chain::permission_level>{{sender,"active"}},
                               config::eosio_system_acount_name, "transfer", result.get_object()["binargs"].as<bytes>());

      if (tx_force_unique) {
         actions.emplace_back( generate_nonce() );
      }

      send_actions(std::move(actions), skip_sign);
   });

   // Net subcommand 
   string new_host;
   auto net = app.add_subcommand( "net", localized("Interact with local p2p network connections"), false );
   net->require_subcommand();
   auto connect = net->add_subcommand("connect", localized("start a new connection to a peer"), false);
   connect->add_option("host", new_host, localized("The hostname:port to connect to."))->required();
   connect->set_callback([&] {
      const auto& v = call(host, port, net_connect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto disconnect = net->add_subcommand("disconnect", localized("close an existing connection"), false);
   disconnect->add_option("host", new_host, localized("The hostname:port to disconnect from."))->required();
   disconnect->set_callback([&] {
      const auto& v = call(host, port, net_disconnect, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto status = net->add_subcommand("status", localized("status of existing connection"), false);
   status->add_option("host", new_host, localized("The hostname:port to query status of connection"))->required();
   status->set_callback([&] {
      const auto& v = call(host, port, net_status, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   auto connections = net->add_subcommand("peers", localized("status of all existing peers"), false);
   connections->set_callback([&] {
      const auto& v = call(host, port, net_connections, new_host);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });



   // Wallet subcommand
   auto wallet = app.add_subcommand( "wallet", localized("Interact with local wallet"), false );
   wallet->require_subcommand();
   // create wallet
   string wallet_name = "default";
   auto createWallet = wallet->add_subcommand("create", localized("Create a new wallet locally"), false);
   createWallet->add_option("-n,--name", wallet_name, localized("The name of the new wallet"), true);
   createWallet->set_callback([&wallet_name] {
      const auto& v = call(wallet_host, wallet_port, wallet_create, wallet_name);
      std::cout << localized("Creating wallet: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      std::cout << localized("Save password to use in the future to unlock this wallet.") << std::endl;
      std::cout << localized("Without password imported keys will not be retrievable.") << std::endl;
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // open wallet
   auto openWallet = wallet->add_subcommand("open", localized("Open an existing wallet"), false);
   openWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to open"));
   openWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_open, wallet_name);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << localized("Opened: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
   });

   // lock wallet
   auto lockWallet = wallet->add_subcommand("lock", localized("Lock wallet"), false);
   lockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to lock"));
   lockWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock, wallet_name);
      std::cout << localized("Locked: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;

   });

   // lock all wallets
   auto locakAllWallets = wallet->add_subcommand("lock_all", localized("Lock all unlocked wallets"), false);
   locakAllWallets->set_callback([] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock_all);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << localized("Locked All Wallets") << std::endl;
   });

   // unlock wallet
   string wallet_pw;
   auto unlockWallet = wallet->add_subcommand("unlock", localized("Unlock wallet"), false);
   unlockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to unlock"));
   unlockWallet->add_option("--password", wallet_pw, localized("The password returned by wallet create"));
   unlockWallet->set_callback([&wallet_name, &wallet_pw] {
      if( wallet_pw.size() == 0 ) {
         std::cout << localized("password: ");
         fc::set_console_echo(false);
         std::getline( std::cin, wallet_pw, '\n' );
         fc::set_console_echo(true);
      }


      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw)};
      /*const auto& v = */call(wallet_host, wallet_port, wallet_unlock, vs);
      std::cout << localized("Unlocked: ${wallet_name}", ("wallet_name", wallet_name)) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // import keys into wallet
   string wallet_key;
   auto importWallet = wallet->add_subcommand("import", localized("Import private key into wallet"), false);
   importWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to import key into"));
   importWallet->add_option("key", wallet_key, localized("Private key in WIF format to import"))->required();
   importWallet->set_callback([&wallet_name, &wallet_key] {
      private_key_type key( wallet_key );
      public_key_type pubkey = key.get_public_key();

      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_key)};
      const auto& v = call(wallet_host, wallet_port, wallet_import_key, vs);
      std::cout << localized("imported private key for: ${pubkey}", ("pubkey", std::string(pubkey))) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list wallets
   auto listWallet = wallet->add_subcommand("list", localized("List opened wallets, * = unlocked"), false);
   listWallet->set_callback([] {
      std::cout << localized("Wallets:") << std::endl;
      const auto& v = call(wallet_host, wallet_port, wallet_list);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list keys
   auto listKeys = wallet->add_subcommand("keys", localized("List of private keys from all unlocked wallets in wif format."), false);
   listKeys->set_callback([] {
      const auto& v = call(wallet_host, wallet_port, wallet_list_keys);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // Push subcommand
   auto push = app.add_subcommand("push", localized("Push arbitrary transactions to the blockchain"), false);
   push->require_subcommand();

   // push action
   string contract;
   string action;
   string data;
   vector<string> permissions;
   auto actionsSubcommand = push->add_subcommand("action", localized("Push a transaction with a single action"));
   actionsSubcommand->fallthrough(false);
   actionsSubcommand->add_option("contract", contract,
                                 localized("The account providing the contract to execute"), true)->required();
   actionsSubcommand->add_option("action", action, localized("The action to execute on the contract"), true)
         ->required();
   actionsSubcommand->add_option("data", data, localized("The arguments to the contract"))->required();
   actionsSubcommand->add_option("-p,--permission", permissions,
                                 localized("An account and permission level to authorize, as in 'account@permission'"));
   actionsSubcommand->add_flag("-s,--skip-sign", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   add_standard_transaction_options(actionsSubcommand);
   actionsSubcommand->set_callback([&] {
      ilog("Converting argument to binary...");
      auto arg= fc::mutable_variant_object
                ("code", contract)
                ("action", action)
                ("args", fc::json::from_string(data));
      auto result = call(json_to_bin_func, arg);

      auto accountPermissions = get_account_permissions(permissions);

      vector<chain::action> actions;
      actions.emplace_back(accountPermissions, contract, action, result.get_object()["binargs"].as<bytes>());

      if (tx_force_unique) {
         actions.emplace_back( generate_nonce() );
      }                                                      

      send_actions(std::move(actions), skip_sign);
   });

   // push transaction
   string trxJson;
   auto trxSubcommand = push->add_subcommand("transaction", localized("Push an arbitrary JSON transaction"));
   trxSubcommand->add_option("transaction", trxJson, localized("The JSON of the transaction to push"))->required();
   trxSubcommand->set_callback([&] {
      auto trx_result = call(push_txn_func, fc::json::from_string(trxJson));
      std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
   });


   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", localized("Push an array of arbitrary JSON transactions"));
   trxsSubcommand->add_option("transactions", trxsJson, localized("The JSON array of the transactions to push"))->required();
   trxsSubcommand->set_callback([&] {
      auto trxs_result = call(push_txn_func, fc::json::from_string(trxsJson));
      std::cout << fc::json::to_pretty_string(trxs_result) << std::endl;
   });

   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const explained_exception& e) {
      return 1;
   } catch (const fc::exception& e) {
      auto errorString = e.to_detail_string();
      if (errorString.find("Connection refused") != string::npos) {
         if (errorString.find(fc::json::to_string(port)) != string::npos) {
            std::cerr << localized("Failed to connect to eosd at ${ip}:${port}; is eosd running?", ("ip", host)("port", port)) << std::endl;
         } else if (errorString.find(fc::json::to_string(wallet_port)) != string::npos) {
            std::cerr << localized("Failed to connect to eos-walletd at ${ip}:${port}; is eos-walletd running?", ("ip", wallet_host)("port", wallet_port)) << std::endl;
         } else {
            std::cerr << localized("Failed to connect") << std::endl;
         }

         if (verbose_errors) {
            elog("connect error: ${e}", ("e", errorString));
         }
      } else {
         // attempt to extract the error code if one is present
         if (verbose_errors || !print_help_text(e)) {
            elog("Failed with error: ${e}", ("e", verbose_errors ? e.to_detail_string() : e.to_string()));
         }
      }
      return 1;
   }

   return 0;
}
