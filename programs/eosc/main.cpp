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
  -h,--help                   Print this help message and exit
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
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/exception/exception.hpp>
#include <eos/utilities/key_conversion.hpp>

#include <eos/chain/config.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/range/algorithm/copy.hpp>

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
#include <config.hpp>

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
const string get_required_keys = chain_func_base + "/get_required_keys";

const string account_history_func_base = "/v1/account_history";
const string get_transaction_func = account_history_func_base + "/get_transaction";
const string get_transactions_func = account_history_func_base + "/get_transactions";
const string get_key_accounts_func = account_history_func_base + "/get_key_accounts";
const string get_controlled_accounts_func = account_history_func_base + "/get_controlled_accounts";

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

inline std::vector<Name> sort_names( std::vector<Name>&& names ) {
   std::sort( names.begin(), names.end() );
   auto itr = std::unique( names.begin(), names.end() );
   names.erase( itr, names.end() );
   return names;
}

inline std::vector<Name> sort_names( const std::vector<Name>& names ) {
   auto results = std::vector<Name>(names);
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

auto tx_expiration = fc::seconds(1);
bool tx_force_unique = false;
void add_standard_transaction_options(CLI::App* cmd) {
   CLI::callback_t parse_exipration = [](CLI::results_t res) -> bool {
      double value_ms; 
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_ms)) {
         return false;
      }
      
      tx_expiration = fc::microseconds(static_cast<uint64_t>(value_ms * 1000.0));
      return true;
   };

   cmd->add_option("-x,--expiration", parse_exipration, localized("set the time in milliseconds before a transaction expires, defaults to 1000ms"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localized("force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times"));
}

std::string generate_nonce_string() {
   return std::to_string(fc::time_point::now().time_since_epoch().count() % 1000000);
}

types::Message generate_nonce() {
   return Message(N(eos),{}, N(nonce), generate_nonce_string());
}

vector<types::AccountPermission> get_account_permissions(const vector<string>& permissions) {
   auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
      vector<string> pieces;
      split(pieces, p, boost::algorithm::is_any_of("@"));
      EOSC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
      return types::AccountPermission(pieces[0], pieces[1]);
   });
   vector<types::AccountPermission> accountPermissions;
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

void sign_transaction(SignedTransaction& trx) {
   // TODO better error checking
   const auto& public_keys = call(wallet_host, wallet_port, wallet_public_keys);
   auto get_arg = fc::mutable_variant_object
         ("transaction", trx)
         ("available_keys", public_keys);
   const auto& required_keys = call(host, port, get_required_keys, get_arg);
   // TODO determine chain id
   fc::variants sign_args = {fc::variant(trx), required_keys["required_keys"], fc::variant(chain_id_type{})};
   const auto& signed_trx = call(wallet_host, wallet_port, wallet_sign_trx, sign_args);
   trx = signed_trx.as<SignedTransaction>();
}

fc::variant push_transaction( SignedTransaction& trx, bool sign ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + tx_expiration;
    transaction_set_reference_block(trx, info.head_block_id);
    boost::sort( trx.scope );

    if (sign) {
       sign_transaction(trx);
    }

    return call( push_txn_func, trx );
}


void create_account(Name creator, Name newaccount, public_key_type owner, public_key_type active, bool sign) {
      auto owner_auth   = eosio::chain::Authority{1, {{owner, 1}}, {}};
      auto active_auth  = eosio::chain::Authority{1, {{active, 1}}, {}};
      auto recovery_auth = eosio::chain::Authority{1, {}, {{{creator, "active"}, 1}}};

      uint64_t deposit = 1;

      SignedTransaction trx;
      trx.scope = sort_names({creator,config::EosContractName});
      transaction_emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
                                           types::newaccount{creator, newaccount, owner_auth,
                                                             active_auth, recovery_auth, deposit});
      std::cout << fc::json::to_pretty_string(push_transaction(trx, sign)) << std::endl;
}

types::Message create_updateauth(const Name& account, const Name& permission, const Name& parent, const Authority& auth, const Name& permissionAuth) {
   return Message { config::EosContractName,
                           vector<types::AccountPermission>{{account,permissionAuth}},
                           "updateauth", 
                           types::updateauth{account, permission, parent, auth}};
}

types::Message create_deleteauth(const Name& account, const Name& permission, const Name& permissionAuth) {
   return Message { config::EosContractName,
                           vector<types::AccountPermission>{{account,permissionAuth}},
                           "deleteauth", 
                           types::deleteauth{account, permission}};
}

types::Message create_linkauth(const Name& account, const Name& code, const Name& type, const Name& requirement) {
   return Message { config::EosContractName,
                           vector<types::AccountPermission>{{account,"active"}},
                           "linkauth", 
                           types::linkauth{account, code, type, requirement}};
}

types::Message create_unlinkauth(const Name& account, const Name& code, const Name& type) {
   return Message { config::EosContractName,
                           vector<types::AccountPermission>{{account,"active"}},
                           "unlinkauth", 
                           types::unlinkauth{account, code, type}};
}

void send_transaction(const std::vector<types::Message>& messages, const std::vector<Name>& scopes, bool skip_sign = false) {
   SignedTransaction trx;
   trx.scope = sort_names(scopes);
   for (const auto& m: messages) {
      transaction_emplace_message(trx, m);
   }

   auto info = get_info();
   trx.expiration = info.head_block_time + tx_expiration;
   transaction_set_reference_block(trx, info.head_block_id);
   if (!skip_sign) {
      sign_transaction(trx);
   }

   std::cout << fc::json::to_pretty_string( call( push_txn_func, trx )) << std::endl;
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

      permissions->set_callback([this] {
         Name account = Name(accountStr);
         Name permission = Name(permissionStr);
         bool is_delete = boost::iequals(authorityJsonOrFile, "null");
         
         if (is_delete) {
            send_transaction({create_deleteauth(account, permission, Name(permissionAuth))}, {account, config::EosContractName}, skip_sign);
         } else {
            types::Authority authority;
            if (boost::istarts_with(authorityJsonOrFile, "EOS")) {
               authority = types::Authority { 1, { {public_key_type(authorityJsonOrFile), 1 } }, {} };
            } else {
               fc::variant parsedAuthority;
               if (boost::istarts_with(authorityJsonOrFile, "{")) {
                  parsedAuthority = fc::json::from_string(authorityJsonOrFile);
               } else {
                  parsedAuthority = fc::json::from_file(authorityJsonOrFile);
               }

               authority = parsedAuthority.as<types::Authority>();
            }

            Name parent;
            if (parentStr.size() == 0) {
               // see if we can auto-determine the proper parent
               const auto account_result = call(get_account_func, fc::mutable_variant_object("name", accountStr));
               const auto& existing_permissions = account_result.get_object()["permissions"].get_array();
               auto permissionPredicate = [this](const auto& perm) { 
                  return perm.is_object() && 
                        perm.get_object().contains("permission") &&
                        boost::equals(perm.get_object()["permission"].get_string(), permissionStr); 
               };

               auto itr = boost::find_if(existing_permissions, permissionPredicate);
               if (itr != existing_permissions.end()) {
                  parent = Name((*itr).get_object()["parent"].get_string());
               } else {
                  // if this is a new permission and there is no parent we default to "active"
                  parent = Name("active");
               }
            } else {
               parent = Name(parentStr);
            }

            send_transaction({create_updateauth(account, permission, parent, authority, Name(permissionAuth))}, {Name(account), config::EosContractName}, skip_sign);
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

      permissions->set_callback([this] {
         Name account = Name(accountStr);
         Name code = Name(codeStr);
         Name type = Name(typeStr);
         bool is_delete = boost::iequals(requirementStr, "null");
         
         if (is_delete) {
            send_transaction({create_unlinkauth(account, code, type)}, {account, config::EosContractName}, skip_sign);
         } else {
            Name requirement = Name(requirementStr);
            send_transaction({create_linkauth(account, code, type, requirement)}, {Name(account), config::EosContractName}, skip_sign);
         }      
      });
   }
};

int main( int argc, char** argv ) {
   path binPath = argv[0];
   if (binPath.is_relative()) {
      binPath = relative(binPath, current_path()); 
   }

   setlocale(LC_ALL, "");
   bindtextdomain(locale_domain, locale_path);
   textdomain(locale_domain);

   CLI::App app{"Command Line Interface to Eos Daemon"};
   app.require_subcommand();
   app.add_option( "-H,--host", host, localized("the host where eosd is running"), true );
   app.add_option( "-p,--port", port, localized("the port where eosd is running"), true );
   app.add_option( "--wallet-host", wallet_host, localized("the host where eos-walletd is running"), true );
   app.add_option( "--wallet-port", wallet_port, localized("the port where eos-walletd is running"), true );

   bool verbose_errors = false;
   app.add_flag( "-v,--verbose", verbose_errors, localized("output verbose messages on error"));

   // Create subcommand
   auto create = app.add_subcommand("create", localized("Create various items, on and off the blockchain"), false);
   create->require_subcommand();

   // create key
   create->add_subcommand("key", localized("Create a new keypair and print the public and private keys"))->set_callback([] {
      auto privateKey = fc::ecc::private_key::generate();
      std::cout << localized("Private key: ${key}", ("key",  key_to_wif(privateKey.get_secret()))) << std::endl;
      std::cout << localized("Public key: ${key}", ("key", string(public_key_type(privateKey.get_public_key())))) << std::endl;
   });

   // create account
   string creator;
   string name;
   string ownerKey;
   string activeKey;
   bool skip_sign = false;
   auto createAccount = create->add_subcommand("account", localized("Create a new account on the blockchain"), false);
   createAccount->add_option("creator", creator, localized("The name of the account creating the new account"))->required();
   createAccount->add_option("name", name, localized("The name of the new account"))->required();
   createAccount->add_option("OwnerKey", ownerKey, localized("The owner public key for the account"))->required();
   createAccount->add_option("ActiveKey", activeKey, localized("The active public key for the account"))->required();
   createAccount->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   createAccount->set_callback([&] {
      create_account(creator, name, public_key_type(ownerKey), public_key_type(activeKey), !skip_sign);
   });

   // create producer
   vector<string> permissions;
   auto createProducer = create->add_subcommand("producer", localized("Create a new producer on the blockchain"), false);
   createProducer->add_option("name", name, localized("The name of the new producer"))->required();
   createProducer->add_option("OwnerKey", ownerKey, localized("The public key for the producer"))->required();
   createProducer->add_option("-p,--permission", permissions,
                              localized("An account and permission level to authorize, as in 'account@permission' (default user@active)"));
   createProducer->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   createProducer->set_callback([&name, &ownerKey, &permissions, &skip_sign] {
      if (permissions.empty()) {
         permissions.push_back(name + "@active");
      }
      auto account_permissions = get_account_permissions(permissions);

      SignedTransaction trx;
      trx.scope = sort_names({config::EosContractName, name});
      transaction_emplace_message(trx, config::EosContractName, account_permissions,
                                  "setproducer", types::setproducer{name, public_key_type(ownerKey), BlockchainConfiguration{}});

      std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign)) << std::endl;
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
                                                   fc::mutable_variant_object("name", accountName)))
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
      auto result = call(get_code_func, fc::mutable_variant_object("name", accountName));

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
   string account_name;
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"), false);
   getTransactions->add_option("account_name", account_name, localized("Name of account to query on"))->required();
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
         const auto& exp  = trx["expiration"].as<fc::time_point_sec>();
         const auto& msgs = trx["messages"].get_array();
         std::cout << tobj["seq_num"].as_string() <<"] " << id << "  " << trx["expiration"].as_string() << std::endl;
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
   contractSubcommand->add_option("wast-file", wastPath, localized("The file containing the contract WAST"))->required()
         ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, localized("The ABI for the contract"))
              ->check(CLI::ExistingFile);
   contractSubcommand->add_flag("-s,--skip-sign", skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   contractSubcommand->set_callback([&] {
      std::string wast;
      std::cout << localized("Reading WAST...") << std::endl;
      fc::read_file_contents(wastPath, wast);
      std::cout << localized("Assembling WASM...") << std::endl;
      auto wasm = assemble_wast(wast);

      types::setcode handler;
      handler.account = account;
      handler.code.assign(wasm.begin(), wasm.end());
      if (abi->count())
         handler.abi = fc::json::from_file(abiPath).as<types::Abi>();

      SignedTransaction trx;
      trx.scope = sort_names({config::EosContractName, account});
      transaction_emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{account,"active"}},
                                           "setcode", handler);

      std::cout << localized("Publishing contract...") << std::endl;
      std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign)) << std::endl;
   });

   // set producer approve/unapprove subcommand
   string producer;
   auto producerSubcommand = setSubcommand->add_subcommand("producer", localized("Approve/unapprove producer"));
   producerSubcommand->require_subcommand();
   auto approveCommand = producerSubcommand->add_subcommand("approve", localized("Approve producer"));
   auto unapproveCommand = producerSubcommand->add_subcommand("unapprove", localized("Unapprove producer"));
   producerSubcommand->add_option("user-name", name, localized("The name of the account approving"))->required();
   producerSubcommand->add_option("producer-name", producer, localized("The name of the producer to approve"))->required();
   producerSubcommand->add_option("-p,--permission", permissions,
                              localized("An account and permission level to authorize, as in 'account@permission' (default user@active)"));
   producerSubcommand->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   producerSubcommand->set_callback([&] {
      // don't need to check unapproveCommand because one of approve or unapprove is required
      bool approve = producerSubcommand->got_subcommand(approveCommand);
      if (permissions.empty()) {
         permissions.push_back(name + "@active");
      }
      auto account_permissions = get_account_permissions(permissions);

      SignedTransaction trx;
      trx.scope = sort_names({config::EosContractName, name});
      transaction_emplace_message(trx, config::EosContractName, account_permissions,
                                  "okproducer", types::okproducer{name, producer, approve});

      push_transaction(trx, !skip_sign);
      std::cout << localized("Set producer approval from ${name} for ${producer} to ${approve}",
         ("name", name)("producer", producer)("value", approve ? "approve" : "unapprove")) << std::endl;
   });

   // set proxy subcommand
   string proxy;
   auto proxySubcommand = setSubcommand->add_subcommand("proxy", localized("Set proxy account for voting"));
   proxySubcommand->add_option("user-name", name, localized("The name of the account to proxy from"))->required();
   proxySubcommand->add_option("proxy-name", proxy, localized("The name of the account to proxy (unproxy if not provided)"));
   proxySubcommand->add_option("-p,--permission", permissions,
                                  localized("An account and permission level to authorize, as in 'account@permission' (default user@active)"));
   proxySubcommand->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   proxySubcommand->set_callback([&] {
      if (permissions.empty()) {
         permissions.push_back(name + "@active");
      }
      auto account_permissions = get_account_permissions(permissions);
      if (proxy.empty()) {
         proxy = name;
      }

      SignedTransaction trx;
      trx.scope = sort_names({config::EosContractName, name});
      transaction_emplace_message(trx, config::EosContractName, account_permissions,
                                  "setproxy", types::setproxy{name, proxy});

      push_transaction(trx, !skip_sign);
      std::cout << localized("Set proxy for ${name} to ${proxy}", ("name", name)("proxy", proxy)) << std::endl;
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
      SignedTransaction trx;
      trx.scope = sort_names({sender,recipient});
      
      if (tx_force_unique) {
         if (memo.size() == 0) {
            // use the memo to add a nonce
            memo = generate_nonce_string();
         } else {
            // add a nonce message
            transaction_emplace_message(trx, generate_nonce());
         }
      }

      transaction_emplace_message(trx, config::EosContractName,
                                           vector<types::AccountPermission>{{sender,"active"}},
                                           "transfer", types::transfer{sender, recipient, amount, memo});


      auto info = get_info();
      trx.expiration = info.head_block_time + tx_expiration;
      transaction_set_reference_block(trx, info.head_block_id);
      if (!skip_sign) {
         sign_transaction(trx);
      }

      std::cout << fc::json::to_pretty_string( call( push_txn_func, trx )) << std::endl;
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
      auto key = utilities::wif_to_key(wallet_key);
      EOSC_ASSERT( key, "invalid private key: ${k}", ("k",wallet_key) );
      public_key_type pubkey = key->get_public_key();

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

   // Benchmark subcommand
   auto benchmark = app.add_subcommand( "benchmark", localized("Configure and execute benchmarks"), false );
   benchmark->require_subcommand();
   auto benchmark_setup = benchmark->add_subcommand( "setup", localized("Configures initial condition for benchmark") );
   uint64_t number_of_accounts = 2;
   benchmark_setup->add_option("accounts", number_of_accounts, localized("the number of accounts in transfer among"))->required();
   benchmark_setup->set_callback([&]{
      std::cerr << localized("Creating ${number_of_accounts} accounts with initial balances", ("number_of_accounts",number_of_accounts)) << std::endl;
      EOSC_ASSERT( number_of_accounts >= 2, "must create at least 2 accounts" );

      auto info = get_info();

      vector<SignedTransaction> batch;
      batch.reserve( number_of_accounts );
      for( uint32_t i = 0; i < number_of_accounts; ++i ) {
        Name newaccount( Name("benchmark").value + i );
        public_key_type owner, active;
        Name creator("inita" );

        auto owner_auth   = eosio::chain::Authority{1, {{owner, 1}}, {}};
        auto active_auth  = eosio::chain::Authority{1, {{active, 1}}, {}};
        auto recovery_auth = eosio::chain::Authority{1, {}, {{{creator, "active"}, 1}}};
        
        uint64_t deposit = 1;
        
        SignedTransaction trx;
        trx.scope = sort_names({creator,config::EosContractName});
        transaction_emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
                                             types::newaccount{creator, newaccount, owner_auth,
                                                               active_auth, recovery_auth, deposit});

        trx.expiration = info.head_block_time + tx_expiration; 
        transaction_set_reference_block(trx, info.head_block_id);
        batch.emplace_back(trx);
      }
      auto result = call( push_txns_func, batch );
      std::cout << fc::json::to_pretty_string(result) << std::endl;
   });

   auto benchmark_transfer = benchmark->add_subcommand( "transfer", localized("executes random transfers among accounts") );
   uint64_t number_of_transfers = 0;
   bool loop = false;
   benchmark_transfer->add_option("accounts", number_of_accounts, localized("the number of accounts in transfer among"))->required();
   benchmark_transfer->add_option("count", number_of_transfers, localized("the number of transfers to execute"))->required();
   benchmark_transfer->add_option("loop", loop, localized("whether or not to loop for ever"));
   benchmark_transfer->set_callback([&]{
      EOSC_ASSERT( number_of_accounts >= 2, "must create at least 2 accounts" );

      std::cerr << localized("Funding ${number_of_accounts} accounts from init", ("number_of_accounts",number_of_accounts)) << std::endl;
      auto info = get_info();
      vector<SignedTransaction> batch;
      batch.reserve(100);
      for( uint32_t i = 0; i < number_of_accounts; ++i ) {
         Name sender( "initb" );
         Name recipient( Name("benchmark").value + i);
         uint32_t amount = 100000;

         SignedTransaction trx;
         trx.scope = sort_names({sender,recipient});
         transaction_emplace_message(trx, config::EosContractName,
                                              vector<types::AccountPermission>{{sender,"active"}},
                                              "transfer", types::transfer{sender, recipient, amount, memo});
         trx.expiration = info.head_block_time + tx_expiration; 
         transaction_set_reference_block(trx, info.head_block_id);

         batch.emplace_back(trx);
         if( batch.size() == 100 ) {
            auto result = call( push_txns_func, batch );
      //      std::cout << fc::json::to_pretty_string(result) << std::endl;
            batch.resize(0);
         }
      }
      if( batch.size() ) {
         auto result = call( push_txns_func, batch );
         //std::cout << fc::json::to_pretty_string(result) << std::endl;
         batch.resize(0);
      }


      std::cerr << localized("Generating random ${number_of_transfers} transfers among ${number_of_accounts}",("number_of_transfers",number_of_transfers)("number_of_accounts",number_of_accounts)) << std::endl;
      while( true ) {
         auto info = get_info();
         uint64_t amount = 1;

         for( uint32_t i = 0; i < number_of_transfers; ++i ) {
            SignedTransaction trx;

            Name sender( Name("benchmark").value + rand() % number_of_accounts );
            Name recipient( Name("benchmark").value + rand() % number_of_accounts );

            while( recipient == sender )
               recipient = Name( Name("benchmark").value + rand() % number_of_accounts );


            auto memo = fc::variant(fc::time_point::now()).as_string() + " " + fc::variant(fc::time_point::now().time_since_epoch()).as_string();
            trx.scope = sort_names({sender,recipient});
            transaction_emplace_message(trx, config::EosContractName,
                                                 vector<types::AccountPermission>{{sender,"active"}},
                                                 "transfer", types::transfer{sender, recipient, amount, memo});
            trx.expiration = info.head_block_time + tx_expiration; 
            transaction_set_reference_block(trx, info.head_block_id);

            batch.emplace_back(trx);
            if( batch.size() == 600 ) {
               auto result = call( push_txns_func, batch );
               //std::cout << fc::json::to_pretty_string(result) << std::endl;
               batch.resize(0);
            }
         }
         if( !loop ) break;
      }
   });

   

   // Push subcommand
   auto push = app.add_subcommand("push", localized("Push arbitrary transactions to the blockchain"), false);
   push->require_subcommand();

   // push message
   string contract;
   string action;
   string data;
   vector<string> scopes;
   auto messageSubcommand = push->add_subcommand("message", localized("Push a transaction with a single message"));
   messageSubcommand->fallthrough(false);
   messageSubcommand->add_option("contract", contract,
                                 localized("The account providing the contract to execute"), true)->required();
   messageSubcommand->add_option("action", action, localized("The action to execute on the contract"), true)
         ->required();
   messageSubcommand->add_option("data", data, localized("The arguments to the contract"))->required();
   messageSubcommand->add_option("-p,--permission", permissions,
                                 localized("An account and permission level to authorize, as in 'account@permission'"));
   messageSubcommand->add_option("-S,--scope", scopes, localized("An comma separated list of accounts in scope for this operation"), true);
   messageSubcommand->add_flag("-s,--skip-sign", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   messageSubcommand->set_callback([&] {
      ilog("Converting argument to binary...");
      auto arg= fc::mutable_variant_object
                ("code", contract)
                ("action", action)
                ("args", fc::json::from_string(data));
      auto result = call(json_to_bin_func, arg);

      auto accountPermissions = get_account_permissions(permissions);

      SignedTransaction trx;
      transaction_emplace_serialized_message(trx, contract, action, accountPermissions,
                                                      result.get_object()["binargs"].as<Bytes>());

      if (tx_force_unique) {
         transaction_emplace_message(trx, generate_nonce());
      }                                                      

      for( const auto& scope : scopes ) {
         vector<string> subscopes;
         boost::split( subscopes, scope, boost::is_any_of( ", :" ) );
         for( const auto& s : subscopes )
         trx.scope.emplace_back(s);
      }
      std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign )) << std::endl;
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
         if (!print_help_text(e) || verbose_errors) {
            elog("Failed with error: ${e}", ("e", e.to_detail_string()));
         }
      }
      return 1;
   }

   return 0;
}
