/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @defgroup eosclienttool EOSIO Command Line Client Reference
 *  @brief Tool for sending transactions and querying state from @ref nodeos
 *  @ingroup eosclienttool
 */

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
  -H,--host TEXT=localhost    the host where nodeos is running
  -p,--port UINT=8888         the port where nodeos is running
  --wallet-host TEXT=localhost
                              the host where keosd is running
  --wallet-port UINT=8888     the port where keosd is running
  -v,--verbose                output verbose actions on error

Subcommands:
  version                     Retrieve version information
  create                      Create various items, on and off the blockchain
  get                         Retrieve various items and information from the blockchain
  set                         Set or update blockchain state
  transfer                    Transfer EOS from account to account
  net                         Interact with local p2p network connections
  wallet                      Interact with local wallet
  sign                        Sign a transaction
  push                        Push arbitrary transactions to the blockchain

```
To get help with any particular subcommand, run it with no arguments as well:
```
$ ./cleos create
Create various items, on and off the blockchain
Usage: ./cleos create SUBCOMMAND

Subcommands:
  key                         Create a new keypair and print the public and private keys
  account                     Create a new account on the blockchain

$ ./cleos create account
Create a new account on the blockchain
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
#include <string>
#include <vector>
#include <regex>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <fc/crypto/hex.hpp>
#include <fc/variant.hpp>
#include <fc/io/datastream.hpp>
#include <fc/io/json.hpp>
#include <fc/io/console.hpp>
#include <fc/exception/exception.hpp>
#include <eosio/utilities/key_conversion.hpp>

#include <eosio/chain/config.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain/transaction_trace.hpp>
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
#include "httpc.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::utilities;
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

string program = "eosc";
string host = "localhost";
uint32_t port = 8888;

// restricting use of wallet to localhost
string wallet_host = "localhost";
uint32_t wallet_port = 8888;

auto   tx_expiration = fc::seconds(30);
string tx_ref_block_num_or_id;
bool   tx_force_unique = false;
bool   tx_dont_broadcast = false;
bool   tx_skip_sign = false;
bool   tx_print_json = false;

uint32_t tx_max_cpu_usage = 0;
uint32_t tx_max_net_usage = 0;

vector<string> tx_permission;

void add_standard_transaction_options(CLI::App* cmd, string default_permission = "") {
   CLI::callback_t parse_expiration = [](CLI::results_t res) -> bool {
      double value_s;
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_s)) {
         return false;
      }

      tx_expiration = fc::seconds(static_cast<uint64_t>(value_s));
      return true;
   };

   cmd->add_option("-x,--expiration", parse_expiration, localized("set the time in seconds before a transaction expires, defaults to 30s"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localized("force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times"));
   cmd->add_flag("-s,--skip-sign", tx_skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   cmd->add_flag("-j,--json", tx_print_json, localized("print result as json"));
   cmd->add_flag("-d,--dont-broadcast", tx_dont_broadcast, localized("don't broadcast transaction to the network (just print to stdout)"));
   cmd->add_option("-r,--ref-block", tx_ref_block_num_or_id, (localized("set the reference block num or block id used for TAPOS (Transaction as Proof-of-Stake)")));

   string msg = "An account and permission level to authorize, as in 'account@permission'";
   if(!default_permission.empty())
      msg += " (defaults to '" + default_permission + "')";
   cmd->add_option("-p,--permission", tx_permission, localized(msg.c_str()));

   cmd->add_option("--max-cpu-usage", tx_max_cpu_usage, localized("set an upper limit on the cpu usage budget, in instructions-retired, for the execution of the transaction (defaults to 0 which means no limit)"));
   cmd->add_option("--max-net-usage", tx_max_net_usage, localized("set an upper limit on the net usage budget, in bytes, for the transaction (defaults to 0 which means no limit)"));
}

vector<chain::permission_level> get_account_permissions(const vector<string>& permissions) {
   auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
      vector<string> pieces;
      split(pieces, p, boost::algorithm::is_any_of("@"));
      if( pieces.size() == 1 ) pieces.push_back( "active" );
      return chain::permission_level{ .actor = pieces[0], .permission = pieces[1] };
   });
   vector<chain::permission_level> accountPermissions;
   boost::range::copy(fixedPermissions, back_inserter(accountPermissions));
   return accountPermissions;
}

template<typename T>
fc::variant call( const std::string& server, uint16_t port,
                  const std::string& path,
                  const T& v ) { return eosio::client::http::call( server, port, path, fc::variant(v) ); }

template<typename T>
fc::variant call( const std::string& path,
                  const T& v ) { return eosio::client::http::call( host, port, path, fc::variant(v) ); }

eosio::chain_apis::read_only::get_info_results get_info() {
  return call(host, port, get_info_func ).as<eosio::chain_apis::read_only::get_info_results>();
}

string generate_nonce_value() {
   return fc::to_string(fc::time_point::now().time_since_epoch().count());
}

chain::action generate_nonce() {
   auto v = generate_nonce_value();
   variant nonce = fc::mutable_variant_object()
         ("value", v);

   try {
      auto result = call(get_code_func, fc::mutable_variant_object("account_name", name(config::system_account_name)));
      abi_serializer eosio_serializer(result["abi"].as<contracts::abi_def>());
      return chain::action( {}, config::system_account_name, "nonce", eosio_serializer.variant_to_binary("nonce", nonce));
   }
   catch (...) {
      EOS_THROW(account_query_exception, "A system contract is required to use nonce");
   }
}

fc::variant determine_required_keys(const signed_transaction& trx) {
   // TODO better error checking
   //wdump((trx));
   const auto& public_keys = call(wallet_host, wallet_port, wallet_public_keys);
   auto get_arg = fc::mutable_variant_object
           ("transaction", (transaction)trx)
           ("available_keys", public_keys);
   const auto& required_keys = call(host, port, get_required_keys, get_arg);
   return required_keys["required_keys"];
}

void sign_transaction(signed_transaction& trx, fc::variant& required_keys) {
   // TODO determine chain id
   fc::variants sign_args = {fc::variant(trx), required_keys, fc::variant(chain_id_type{})};
   const auto& signed_trx = call(wallet_host, wallet_port, wallet_sign_trx, sign_args);
   trx = signed_trx.as<signed_transaction>();
}

fc::variant push_transaction( signed_transaction& trx, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none ) {
   auto info = get_info();
   trx.expiration = info.head_block_time + tx_expiration;

   // Set tapos, default to last irreversible block if it's not specified by the user
   block_id_type ref_block_id;
   try {
      fc::variant ref_block;
      if (!tx_ref_block_num_or_id.empty()) {
         ref_block = call(get_block_func, fc::mutable_variant_object("block_num_or_id", tx_ref_block_num_or_id));
      } else {
         ref_block = call(get_block_func, fc::mutable_variant_object("block_num_or_id", info.last_irreversible_block_num));
      }
      ref_block_id = ref_block["id"].as<block_id_type>();
   } EOS_RETHROW_EXCEPTIONS(invalid_ref_block_exception, "Invalid reference block num or id: ${block_num_or_id}", ("block_num_or_id", tx_ref_block_num_or_id));
   trx.set_reference_block(ref_block_id);

   if (tx_force_unique) {
      trx.context_free_actions.emplace_back( generate_nonce() );
   }

   auto required_keys = determine_required_keys(trx);
   size_t num_keys = required_keys.is_array() ? required_keys.get_array().size() : 1;

   trx.max_kcpu_usage = (tx_max_cpu_usage + 1023)/1024;
   trx.max_net_usage_words = (tx_max_net_usage + 7)/8;

   if (!tx_skip_sign) {
      sign_transaction(trx, required_keys);
   }

   if (!tx_dont_broadcast) {
      return call(push_txn_func, packed_transaction(trx, compression));
   } else {
      return fc::variant(trx);
   }
}

fc::variant push_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none ) {
   signed_transaction trx;
   trx.actions = std::forward<decltype(actions)>(actions);

   return push_transaction(trx, extra_kcpu, compression);
}

void print_result( const fc::variant& result ) {
      const auto& processed = result["processed"];
      const auto& transaction_id = processed["id"].as_string();
      const auto& status = processed["status"].as_string() ;
      auto net = processed["net_usage"].as_int64();
      auto cpu = processed["cpu_usage"].as_int64();

      cout << status << " transaction: " << transaction_id << "  " << net << " bytes  " << cpu << " cycles\n";

      const auto& actions = processed["action_traces"].get_array();
      for( const auto& at : actions ) {
         auto receiver = at["receiver"].as_string();
         const auto& act = at["act"].get_object();
         auto code = act["account"].as_string();
         auto func = act["name"].as_string();
         auto args = fc::json::to_string( act["data"] );
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
            std::getline( ss, line );
            cout << ">> " << line << "\n";
         }
      }
}

using std::cout;
void send_actions(std::vector<chain::action>&& actions, int32_t extra_kcpu = 1000, packed_transaction::compression_type compression = packed_transaction::none ) {
   auto result = push_actions( move(actions), extra_kcpu, compression);

   if( tx_print_json ) {
      cout << fc::json::to_pretty_string( result );
   } else {
      print_result( result );
   }
}

void send_transaction( signed_transaction& trx, int32_t extra_kcpu, packed_transaction::compression_type compression = packed_transaction::none  ) {
   auto result = push_transaction(trx, extra_kcpu, compression);

   if( tx_print_json ) {
      cout << fc::json::to_pretty_string( result );
   } else {
      auto trace = result["processed"].as<eosio::chain::transaction_trace>();
      print_result( result );
   }
}

chain::action create_newaccount(const name& creator, const name& newaccount, public_key_type owner, public_key_type active) {
   return action {
      tx_permission.empty() ? vector<chain::permission_level>{{creator,config::active_name}} : get_account_permissions(tx_permission),
      contracts::newaccount{
         .creator      = creator,
         .name         = newaccount,
         .owner        = eosio::chain::authority{1, {{owner, 1}}, {}},
         .active       = eosio::chain::authority{1, {{active, 1}}, {}},
         .recovery     = eosio::chain::authority{1, {}, {{{creator, config::active_name}, 1}}}
      }
   };
}

chain::action create_transfer(const name& sender, const name& recipient, uint64_t amount, const string& memo ) {

   auto transfer = fc::mutable_variant_object
      ("from", sender)
      ("to", recipient)
      ("quantity", asset(amount))
      ("memo", memo);

   auto args = fc::mutable_variant_object
      ("code", name(config::system_account_name))
      ("action", "transfer")
      ("args", transfer);

   auto result = call(json_to_bin_func, args);

   return action {
      tx_permission.empty() ? vector<chain::permission_level>{{sender,config::active_name}} : get_account_permissions(tx_permission),
      config::system_account_name, "transfer", result.get_object()["binargs"].as<bytes>()
   };
}

chain::action create_setabi(const name& account, const contracts::abi_def& abi) {
   return action {
      tx_permission.empty() ? vector<chain::permission_level>{{account,config::active_name}} : get_account_permissions(tx_permission),
      contracts::setabi{
         .account   = account,
         .abi       = abi
      }
   };
}

chain::action create_setcode(const name& account, const bytes& code) {
   return action {
      tx_permission.empty() ? vector<chain::permission_level>{{account,config::active_name}} : get_account_permissions(tx_permission),
      contracts::setcode{
         .account   = account,
         .vmtype    = 0,
         .vmversion = 0,
         .code      = code
      }
   };
}

chain::action create_updateauth(const name& account, const name& permission, const name& parent, const authority& auth) {
   return action { tx_permission.empty() ? vector<chain::permission_level>{{account,config::active_name}} : get_account_permissions(tx_permission),
                   contracts::updateauth{account, permission, parent, auth}};
}

chain::action create_deleteauth(const name& account, const name& permission) {
   return action { tx_permission.empty() ? vector<chain::permission_level>{{account,config::active_name}} : get_account_permissions(tx_permission),
                   contracts::deleteauth{account, permission}};
}

chain::action create_linkauth(const name& account, const name& code, const name& type, const name& requirement) {
   return action { tx_permission.empty() ? vector<chain::permission_level>{{account,config::active_name}} : get_account_permissions(tx_permission),
                   contracts::linkauth{account, code, type, requirement}};
}

chain::action create_unlinkauth(const name& account, const name& code, const name& type) {
   return action { tx_permission.empty() ? vector<chain::permission_level>{{account,config::active_name}} : get_account_permissions(tx_permission),
                   contracts::unlinkauth{account, code, type}};
}

fc::variant json_from_file_or_string(const string& file_or_str, fc::json::parse_type ptype = fc::json::legacy_parser)
{
   regex r("^[ \t]*[\{\[]");
   if ( !regex_search(file_or_str, r) && is_regular_file(file_or_str) ) {
      return fc::json::from_file(file_or_str, ptype);
   } else {
      return fc::json::from_string(file_or_str, ptype);
   }
}

struct set_account_permission_subcommand {
   string accountStr;
   string permissionStr;
   string authorityJsonOrFile;
   string parentStr;

   set_account_permission_subcommand(CLI::App* accountCmd) {
      auto permissions = accountCmd->add_subcommand("permission", localized("set parmaters dealing with account permissions"));
      permissions->add_option("account", accountStr, localized("The account to set/delete a permission authority for"))->required();
      permissions->add_option("permission", permissionStr, localized("The permission name to set/delete an authority for"))->required();
      permissions->add_option("authority", authorityJsonOrFile, localized("[delete] NULL, [create/update] JSON string or filename defining the authority"))->required();
      permissions->add_option("parent", parentStr, localized("[create] The permission name of this parents permission (Defaults to: \"Active\")"));

      add_standard_transaction_options(permissions, "account@active");

      permissions->set_callback([this] {
         name account = name(accountStr);
         name permission = name(permissionStr);
         bool is_delete = boost::iequals(authorityJsonOrFile, "null");

         if (is_delete) {
            send_actions({create_deleteauth(account, permission)});
         } else {
            authority auth;
            if (boost::istarts_with(authorityJsonOrFile, "EOS")) {
               try {
                  auth = authority(public_key_type(authorityJsonOrFile));
               } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", authorityJsonOrFile))
            } else {
               try {
                  auth = json_from_file_or_string(authorityJsonOrFile).as<authority>();
               } EOS_RETHROW_EXCEPTIONS(authority_type_exception, "Fail to parse Authority JSON '${data}'", ("data",authorityJsonOrFile))

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
                  parent = name(config::active_name);

               }
            } else {
               parent = name(parentStr);
            }

            send_actions({create_updateauth(account, permission, parent, auth)});
         }
      });
   }

};

struct set_action_permission_subcommand {
   string accountStr;
   string codeStr;
   string typeStr;
   string requirementStr;

   set_action_permission_subcommand(CLI::App* actionRoot) {
      auto permissions = actionRoot->add_subcommand("permission", localized("set parmaters dealing with account permissions"));
      permissions->add_option("account", accountStr, localized("The account to set/delete a permission authority for"))->required();
      permissions->add_option("code", codeStr, localized("The account that owns the code for the action"))->required();
      permissions->add_option("type", typeStr, localized("the type of the action"))->required();
      permissions->add_option("requirement", requirementStr, localized("[delete] NULL, [set/update] The permission name require for executing the given action"))->required();

      add_standard_transaction_options(permissions, "account@active");

      permissions->set_callback([this] {
         name account = name(accountStr);
         name code = name(codeStr);
         name type = name(typeStr);
         bool is_delete = boost::iequals(requirementStr, "null");

         if (is_delete) {
            send_actions({create_unlinkauth(account, code, type)});
         } else {
            name requirement = name(requirementStr);
            send_actions({create_linkauth(account, code, type, requirement)});
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

   CLI::App app{"Command Line Interface to EOSIO Client"};
   app.require_subcommand();
   app.add_option( "-H,--host", host, localized("the host where nodeos is running"), true );
   app.add_option( "-p,--port", port, localized("the port where nodeos is running"), true );
   app.add_option( "--wallet-host", wallet_host, localized("the host where keosd is running"), true );
   app.add_option( "--wallet-port", wallet_port, localized("the port where keosd is running"), true );

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
   string owner_key_str;
   string active_key_str;

   auto createAccount = create->add_subcommand("account", localized("Create a new account on the blockchain"), false);
   createAccount->add_option("creator", creator, localized("The name of the account creating the new account"))->required();
   createAccount->add_option("name", account_name, localized("The name of the new account"))->required();
   createAccount->add_option("OwnerKey", owner_key_str, localized("The owner public key for the new account"))->required();
   createAccount->add_option("ActiveKey", active_key_str, localized("The active public key for the new account"))->required();
   add_standard_transaction_options(createAccount, "creator@active");
   createAccount->set_callback([&] {
      public_key_type owner_key, active_key;
      try {
         owner_key = public_key_type(owner_key_str);
      } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid owner public key: ${public_key}", ("public_key", owner_key_str))
      try {
         active_key = public_key_type(active_key_str);
      } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid active public key: ${public_key}", ("public_key", active_key_str))
      send_actions({create_newaccount(creator, account_name, owner_key, active_key)});
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
   string table_key;
   bool binary = false;
   uint32_t limit = 10;
   auto getTable = get->add_subcommand( "table", localized("Retrieve the contents of a database table"), false);
   getTable->add_option( "contract", code, localized("The contract who owns the table") )->required();
   getTable->add_option( "scope", scope, localized("The scope within the contract in which the table is found") )->required();
   getTable->add_option( "table", table, localized("The name of the table as specified by the contract abi") )->required();
   getTable->add_option( "-b,--binary", binary, localized("Return the value as BINARY rather than using abi to interpret as JSON") );
   getTable->add_option( "-l,--limit", limit, localized("The maximum number of rows to return") );
   getTable->add_option( "-k,--key", table_key, localized("The name of the key to index by as defined by the abi, defaults to primary key") );
   getTable->add_option( "-L,--lower", lower, localized("JSON representation of lower bound value of key, defaults to first") );
   getTable->add_option( "-U,--upper", upper, localized("JSON representation of upper bound value value of key, defaults to last") );

   getTable->set_callback([&] {
      auto result = call(get_table_func, fc::mutable_variant_object("json", !binary)
                         ("code",code)
                         ("scope",scope)
                         ("table",table)
                         ("table_key",table_key)
                         ("lower_bound",lower)
                         ("upper_bound",upper)
                         ("limit",limit)
                         );

      std::cout << fc::json::to_pretty_string(result)
                << std::endl;
   });

   // currency accessors
   // get currency balance
   string symbol;
   auto get_currency = get->add_subcommand( "currency", localized("Retrieve information related to standard currencies"), true);
   get_currency->require_subcommand();
   auto get_balance = get_currency->add_subcommand( "balance", localized("Retrieve the balance of an account for a given currency"), false);
   get_balance->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_balance->add_option( "account", accountName, localized("The account to query balances for") )->required();
   get_balance->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") );
   get_balance->set_callback([&] {
      auto result = call(get_currency_balance_func, fc::mutable_variant_object
         ("account", accountName)
         ("code", code)
         ("symbol", symbol.empty() ? fc::variant() : symbol)
      );

      const auto& rows = result.get_array();
      for( const auto& r : rows ) {
         std::cout << r.as_string()
                   << std::endl;
      }
   });

   auto get_currency_stats = get_currency->add_subcommand( "stats", localized("Retrieve the stats of for a given currency"), false);
   get_currency_stats->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_currency_stats->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") )->required();
   get_currency_stats->set_callback([&] {
      auto result = call(get_currency_stats_func, fc::mutable_variant_object("json", false)
         ("code", code)
         ("symbol", symbol)
      );

      std::cout << fc::json::to_pretty_string(result)
                << std::endl;
   });

   // get accounts
   string public_key_str;
   auto getAccounts = get->add_subcommand("accounts", localized("Retrieve accounts associated with a public key"), false);
   getAccounts->add_option("public_key", public_key_str, localized("The public key to retrieve accounts for"))->required();
   getAccounts->set_callback([&] {
      public_key_type public_key;
      try {
         public_key = public_key_type(public_key_str);
      } EOS_RETHROW_EXCEPTIONS(public_key_type_exception, "Invalid public key: ${public_key}", ("public_key", public_key_str))
      auto arg = fc::mutable_variant_object( "public_key", public_key);
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
   string transaction_id_str;
   auto getTransaction = get->add_subcommand("transaction", localized("Retrieve a transaction from the blockchain"), false);
   getTransaction->add_option("id", transaction_id_str, localized("ID of the transaction to retrieve"))->required();
   getTransaction->set_callback([&] {
      transaction_id_type transaction_id;
      try {
         transaction_id = transaction_id_type(transaction_id_str);
      } EOS_RETHROW_EXCEPTIONS(transaction_id_type_exception, "Invalid transaction ID: ${transaction_id}", ("transaction_id", transaction_id_str))
      auto arg= fc::mutable_variant_object( "transaction_id", transaction_id);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get transactions
   string skip_seq_str;
   string num_seq_str;
   bool printjson = false;
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"), false);
   getTransactions->add_option("account_name", account_name, localized("name of account to query on"))->required();
   getTransactions->add_option("skip_seq", skip_seq_str, localized("Number of most recent transactions to skip (0 would start at most recent transaction)"));
   getTransactions->add_option("num_seq", num_seq_str, localized("Number of transactions to return"));
   getTransactions->add_flag("--json,-j", printjson, localized("print full json"));
   getTransactions->set_callback([&] {
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
         std::cout << fc::json::to_pretty_string(call(get_transactions_func, arg)) << std::endl;
      }
      else {
         const auto& trxs = result.get_object()["transactions"].get_array();
         for( const auto& t : trxs ) {

            const auto& tobj = t.get_object();
            const auto& trx  = tobj["transaction"].get_object();
            const auto& data = trx["data"].get_object();
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

   // set subcommand
   auto setSubcommand = app.add_subcommand("set", localized("Set or update blockchain state"));
   setSubcommand->require_subcommand();

   // set contract subcommand
   string account;
   string contractPath;
   string wastPath;
   string abiPath;
   auto contractSubcommand = setSubcommand->add_subcommand("contract", localized("Create or update the contract on an account"));
   contractSubcommand->add_option("account", account, localized("The account to publish a contract for"))
                     ->required();
   contractSubcommand->add_option("contract-dir", contractPath, localized("The the path containing the .wast and .abi"))
                     ->required();
   contractSubcommand->add_option("wast-file", wastPath, localized("The file containing the contract WAST or WASM relative to contract-dir"));
//                     ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, localized("The ABI for the contract relative to contract-dir"));
//                                ->check(CLI::ExistingFile);


   add_standard_transaction_options(contractSubcommand, "account@active");
   contractSubcommand->set_callback([&] {
      std::string wast;
      fc::path cpath(contractPath);

      if( cpath.filename().generic_string() == "." ) cpath = cpath.parent_path();

      if( wastPath.empty() )
      {
         wastPath = (cpath / (cpath.filename().generic_string()+".wasm")).generic_string();
         if (!fc::exists(wastPath))
            wastPath = (cpath / (cpath.filename().generic_string()+".wast")).generic_string();
      }

      if( abiPath.empty() )
      {
         abiPath = (cpath / (cpath.filename().generic_string()+".abi")).generic_string();
      }
      
      std::cout << localized(("Reading WAST/WASM from " + wastPath + "...").c_str()) << std::endl;
      fc::read_file_contents(wastPath, wast);
      FC_ASSERT( !wast.empty(), "no wast file found ${f}", ("f", wastPath) );
      vector<uint8_t> wasm;
      const string binary_wasm_header("\x00\x61\x73\x6d", 4);
      if(wast.compare(0, 4, binary_wasm_header) == 0) {
         std::cout << localized("Using already assembled WASM...") << std::endl;
         wasm = vector<uint8_t>(wast.begin(), wast.end());
      }
      else {
         std::cout << localized("Assembling WASM...") << std::endl;
         wasm = wast_to_wasm(wast);
      }

      std::vector<chain::action> actions;
      actions.emplace_back( create_setcode(account, bytes(wasm.begin(), wasm.end()) ) );

      FC_ASSERT( fc::exists( abiPath ), "no abi file found ${f}", ("f", abiPath)  );

      try {
         actions.emplace_back( create_setabi(account, fc::json::from_file(abiPath).as<contracts::abi_def>()) );
      } EOS_RETHROW_EXCEPTIONS(abi_type_exception,  "Fail to parse ABI JSON")

      std::cout << localized("Publishing contract...") << std::endl;
      send_actions(std::move(actions), 10000, packed_transaction::zlib);
      /*
      auto result = push_actions(std::move(actions), 10000, packed_transaction::zlib);

      if( tx_dont_broadcast ) {
         std::cout << fc::json::to_pretty_string(result) << "\n";
      }
      */
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

   add_standard_transaction_options(transfer, "sender@active");
   transfer->set_callback([&] {
      signed_transaction trx;
      if (tx_force_unique && memo.size() == 0) {
         // use the memo to add a nonce
         memo = generate_nonce_value();
         tx_force_unique = false;
      }

      send_actions({create_transfer(sender, recipient, amount, memo)});
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
   string wallet_key_str;
   auto importWallet = wallet->add_subcommand("import", localized("Import private key into wallet"), false);
   importWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to import key into"));
   importWallet->add_option("key", wallet_key_str, localized("Private key in WIF format to import"))->required();
   importWallet->set_callback([&wallet_name, &wallet_key_str] {
      private_key_type wallet_key;
      try {
         wallet_key = private_key_type( wallet_key_str );
      } catch (...) {
          EOS_THROW(private_key_type_exception, "Invalid private key: ${private_key}", ("private_key", wallet_key_str))
      }
      public_key_type pubkey = wallet_key.get_public_key();

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

   // sign subcommand
   string trx_json_to_sign;
   string str_private_key;
   bool push_trx = false;

   auto sign = app.add_subcommand("sign", localized("Sign a transaction"), false);
   sign->add_option("transaction", trx_json_to_sign,
                                 localized("The JSON string or filename defining the transaction to sign"), true)->required();
   sign->add_option("-k,--private-key", str_private_key, localized("The private key that will be used to sign the transaction"));
   sign->add_flag( "-p,--push-transaction", push_trx, localized("Push transaction after signing"));

   sign->set_callback([&] {
      signed_transaction trx = json_from_file_or_string(trx_json_to_sign).as<signed_transaction>();

      if( str_private_key.size() == 0 ) {
         std::cerr << localized("private key: ");
         fc::set_console_echo(false);
         std::getline( std::cin, str_private_key, '\n' );
         fc::set_console_echo(true);
      }

      auto priv_key = fc::crypto::private_key::regenerate(*utilities::wif_to_key(str_private_key));
      trx.sign(priv_key, chain_id_type{});

      if(push_trx) {
         auto trx_result = call(push_txn_func, packed_transaction(trx, packed_transaction::none));
         std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
      } else {
         std::cout << fc::json::to_pretty_string(trx) << std::endl;
      }
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
   actionsSubcommand->add_option("action", action,
                                 localized("A JSON string or filename defining the action to execute on the contract"), true)->required();
   actionsSubcommand->add_option("data", data, localized("The arguments to the contract"))->required();

   add_standard_transaction_options(actionsSubcommand);
   actionsSubcommand->set_callback([&] {
      fc::variant action_args_var;
      try {
         action_args_var = json_from_file_or_string(data, fc::json::relaxed_parser);
      } EOS_RETHROW_EXCEPTIONS(action_type_exception, "Fail to parse action JSON data='${data}'", ("data",data))

      auto arg= fc::mutable_variant_object
                ("code", contract)
                ("action", action)
                ("args", action_args_var);
      auto result = call(json_to_bin_func, arg);

      auto accountPermissions = get_account_permissions(tx_permission);

      send_actions({chain::action{accountPermissions, contract, action, result.get_object()["binargs"].as<bytes>()}});
   });

   // push transaction
   string trx_to_push;
   auto trxSubcommand = push->add_subcommand("transaction", localized("Push an arbitrary JSON transaction"));
   trxSubcommand->add_option("transaction", trx_to_push, localized("The JSON string or filename defining the transaction to push"))->required();

   trxSubcommand->set_callback([&] {
      fc::variant trx_var;
      try {
         trx_var = json_from_file_or_string(trx_to_push);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON '${data}'", ("data",trx_to_push))
      signed_transaction trx = trx_var.as<signed_transaction>();
      auto trx_result = call(push_txn_func, packed_transaction(trx, packed_transaction::none));
      std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
   });


   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", localized("Push an array of arbitrary JSON transactions"));
   trxsSubcommand->add_option("transactions", trxsJson, localized("The JSON string or filename defining the array of the transactions to push"))->required();
   trxsSubcommand->set_callback([&] {
      fc::variant trx_var;
      try {
         trx_var = json_from_file_or_string(trxsJson);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON '${data}'", ("data",trxsJson))
      auto trxs_result = call(push_txns_func, trx_var);
      std::cout << fc::json::to_pretty_string(trxs_result) << std::endl;
   });


   // multisig subcommand
   auto msig = app.add_subcommand("multisig", localized("Multisig contract commands"), false);
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

   auto propose_action = msig->add_subcommand("propose", localized("Propose transaction"));
   //auto propose_action = msig->add_subcommand("action", localized("Propose action"));
   add_standard_transaction_options(propose_action);
   propose_action->add_option("proposal_name", proposal_name, localized("proposal name (string)"))->required();
   propose_action->add_option("requested_permissions", requested_perm, localized("The JSON string of filename defining requested permissions"))->required();
   propose_action->add_option("trx_permissions", transaction_perm, localized("The JSON string of filename defining transaction permissions"))->required();
   propose_action->add_option("contract", proposed_contract, localized("contract to wich deferred transaction should be delivered"))->required();
   propose_action->add_option("action", proposed_action, localized("action of deferred transaction"))->required();
   propose_action->add_option("data", proposed_transaction, localized("The JSON string or filename defining the action to propose"))->required();
   propose_action->add_option("proposer", proposer, localized("Account proposing the transaction"));
   propose_action->add_option("proposal_expiration", parse_expiration_hours, localized("Proposal expiration interval in hours"));

   propose_action->set_callback([&] {
      fc::variant requested_perm_var;
      try {
         requested_perm_var = json_from_file_or_string(requested_perm);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",requested_perm))
      fc::variant transaction_perm_var;
      try {
         transaction_perm_var = json_from_file_or_string(transaction_perm);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",transaction_perm))
      fc::variant trx_var;
      try {
         trx_var = json_from_file_or_string(proposed_transaction);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse transaction JSON '${data}'", ("data",proposed_transaction))
      transaction proposed_trx = trx_var.as<transaction>();

      auto arg= fc::mutable_variant_object()
         ("code", proposed_contract)
         ("action", proposed_action)
         ("args", trx_var);

      auto result = call(json_to_bin_func, arg);
      //std::cout << "Result: "; fc::json::to_stream(std::cout, result); std::cout << std::endl;

      bytes proposed_trx_serialized = result.get_object()["binargs"].as<bytes>();

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
            accountPermissions = vector<permission_level>{{proposer, config::active_name}};
         } else {
            EOS_THROW(tx_missing_auth, "Authority is not provided (either by multisig parameter <proposer> or -p)");
         }
      }
      if (proposer.empty()) {
         proposer = name(accountPermissions.at(0).actor).to_string();
      }

      transaction trx;

      trx.expiration = fc::time_point_sec( fc::time_point::now() + fc::hours(proposal_expiration_hours) );
      trx.region = 0;
      trx.ref_block_num = 0;
      trx.ref_block_prefix = 0;
      trx.max_net_usage_words = 0;
      trx.max_kcpu_usage = 0;
      trx.delay_sec = 0;
      trx.actions = { chain::action(trxperm, name(proposed_contract), name(proposed_action), proposed_trx_serialized) };

      fc::to_variant(trx, trx_var);

      arg = fc::mutable_variant_object()
         ("code", "eosio.msig")
         ("action", "propose")
         ("args", fc::mutable_variant_object()
          ("proposer", proposer )
          ("proposal_name", proposal_name)
          ("requested", requested_perm_var)
          ("trx", trx_var)
         );
      result = call(json_to_bin_func, arg);
      send_actions({chain::action{accountPermissions, "eosio.msig", "propose", result.get_object()["binargs"].as<bytes>()}});
   });

   //resolver for ABI serializer to decode actions in proposed transaction in multisig contract
   auto resolver = [](const name& code) -> optional<contracts::abi_serializer> {
      auto result = call(get_code_func, fc::mutable_variant_object("account_name", code.to_string()));
      if (result["abi"].is_object()) {
         //std::cout << "ABI: " << fc::json::to_pretty_string(result) << std::endl;
         return optional<contracts::abi_serializer>(abi_serializer(result["abi"].as<contracts::abi_def>()));
      } else {
         std::cerr << "ABI for contract " << code.to_string() << " not found. Action data will be shown in hex only." << std::endl;
         return optional<contracts::abi_serializer>();
      }
   };

   // multisig review
   auto review = msig->add_subcommand("review", localized("Review transaction"));
   add_standard_transaction_options(review);
   review->add_option("proposer", proposer, localized("proposer name (string)"))->required();
   review->add_option("proposal_name", proposal_name, localized("proposal name (string)"))->required();

   review->set_callback([&] {
      auto result = call(get_table_func, fc::mutable_variant_object("json", true)
                         ("code", "eosio.msig")
                         ("scope", proposer)
                         ("table", "proposal")
                         ("table_key", "")
                         ("lower_bound", eosio::chain::string_to_name(proposal_name.c_str()))
                         ("upper_bound", "")
                         ("limit", 1)
                         );
      //std::cout << fc::json::to_pretty_string(result) << std::endl;

      fc::variants rows = result.get_object()["rows"].get_array();
      if (rows.empty()) {
         std::cerr << "Proposal not found" << std::endl;
         return;
      }
      fc::mutable_variant_object obj = rows[0].get_object();
      if (obj["proposal_name"] != proposal_name) {
         std::cerr << "Proposal not found" << std::endl;
         return;
      }
      auto trx_hex = obj["packed_transaction"].as_string();
      vector<char> trx_blob(trx_hex.size()/2);
      fc::from_hex(trx_hex, trx_blob.data(), trx_blob.size());
      transaction trx = fc::raw::unpack<transaction>(trx_blob);

      fc::variant trx_var;
      abi_serializer abi;
      abi.to_variant(trx, trx_var, resolver);
      obj["transaction"] = trx_var;
      std::cout << fc::json::to_pretty_string(obj)
                << std::endl;
   });

   string perm;
   auto approve_or_unapprove = [&](const string& action) {
      fc::variant perm_var;
      try {
         perm_var = json_from_file_or_string(perm);
      } EOS_RETHROW_EXCEPTIONS(transaction_type_exception, "Fail to parse permissions JSON '${data}'", ("data",perm))
      auto arg = fc::mutable_variant_object()
         ("code", "eosio.msig")
         ("action", action)
         ("args", fc::mutable_variant_object()
          ("proposer", proposer)
          ("proposal_name", proposal_name)
          ("level", perm_var)
         );
      auto result = call(json_to_bin_func, arg);
      auto accountPermissions = tx_permission.empty() ? vector<chain::permission_level>{{sender,config::active_name}} : get_account_permissions(tx_permission);
      send_actions({chain::action{accountPermissions, "eosio.msig", action, result.get_object()["binargs"].as<bytes>()}});
   };

   // multisig approve
   auto approve = msig->add_subcommand("approve", localized("Approve proposed transaction"));
   add_standard_transaction_options(approve);
   approve->add_option("proposer", proposer, localized("proposer name (string)"))->required();
   approve->add_option("proposal_name", proposal_name, localized("proposal name (string)"))->required();
   approve->add_option("permissions", perm, localized("The JSON string of filename defining approving permissions"))->required();
   approve->set_callback([&] { approve_or_unapprove("approve"); });

   // multisig unapprove
   auto unapprove = msig->add_subcommand("unapprove", localized("Unapprove proposed transaction"));
   add_standard_transaction_options(unapprove);
   unapprove->add_option("proposer", proposer, localized("proposer name (string)"))->required();
   unapprove->add_option("proposal_name", proposal_name, localized("proposal name (string)"))->required();
   unapprove->add_option("permissions", perm, localized("The JSON string of filename defining approving permissions"))->required();
   unapprove->set_callback([&] { approve_or_unapprove("unapprove"); });

   // multisig cancel
   string canceler;
   auto cancel = msig->add_subcommand("cancel", localized("Cancel proposed transaction"));
   add_standard_transaction_options(cancel);
   cancel->add_option("proposer", proposer, localized("proposer name (string)"))->required();
   cancel->add_option("proposal_name", proposal_name, localized("proposal name (string)"))->required();
   cancel->add_option("canceler", canceler, localized("canceler name (string)"));
   cancel->set_callback([&]() {
      auto accountPermissions = get_account_permissions(tx_permission);
      if (accountPermissions.empty()) {
         if (!canceler.empty()) {
            accountPermissions = vector<permission_level>{{canceler, config::active_name}};
         } else {
            EOS_THROW(tx_missing_auth, "Authority is not provided (either by multisig parameter <canceler> or -p)");
         }
      }
      if (canceler.empty()) {
         canceler = name(accountPermissions.at(0).actor).to_string();
      }
      auto arg = fc::mutable_variant_object()
         ("code", "eosio.msig")
         ("action", "cancel")
         ("args", fc::mutable_variant_object()
          ("proposer", proposer)
          ("proposal_name", proposal_name)
          ("canceler", canceler)
         );
      auto result = call(json_to_bin_func, arg);
      send_actions({chain::action{accountPermissions, "eosio.msig", "cancel", result.get_object()["binargs"].as<bytes>()}});
      }
   );

   // multisig exec
   string executer;
   auto exec = msig->add_subcommand("exec", localized("Execute proposed transaction"));
   add_standard_transaction_options(exec);
   exec->add_option("proposer", proposer, localized("proposer name (string)"))->required();
   exec->add_option("proposal_name", proposal_name, localized("proposal name (string)"))->required();
   exec->add_option("executer", executer, localized("account paying for execution (string)"));
   exec->set_callback([&] {
      auto accountPermissions = get_account_permissions(tx_permission);
      if (accountPermissions.empty()) {
         if (!executer.empty()) {
            accountPermissions = vector<permission_level>{{executer, config::active_name}};
         } else {
            EOS_THROW(tx_missing_auth, "Authority is not provided (either by multisig parameter <executer> or -p)");
         }
      }
      if (executer.empty()) {
         executer = name(accountPermissions.at(0).actor).to_string();
      }

      fc::variant perm_var;
      abi_serializer abi;
      abi.to_variant(accountPermissions, perm_var, resolver);

      auto arg = fc::mutable_variant_object()
         ("code", "eosio.msig")
         ("action", "exec")
         ("args", fc::mutable_variant_object()
          ("proposer", proposer )
          ("proposal_name", proposal_name)
          ("executer", executer)
         );
      auto result = call(json_to_bin_func, arg);
      //std::cout << "Result: " << result << std::endl;
      send_actions({chain::action{accountPermissions, "eosio.msig", "exec", result.get_object()["binargs"].as<bytes>()}});
      }
   );

   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const explained_exception& e) {
      return 1;
   } catch (connection_exception& e) {
      auto errorString = e.to_detail_string();
         if (errorString.find(fc::json::to_string(port)) != string::npos) {
            std::cerr << localized("Failed to connect to nodeos at ${ip}:${port}; is nodeos running?", ("ip", host)("port", port)) << std::endl;
         } else if (errorString.find(fc::json::to_string(wallet_port)) != string::npos) {
            std::cerr << localized("Failed to connect to keosd at ${ip}:${port}; is keosd running?", ("ip", wallet_host)("port", wallet_port)) << std::endl;
         } else {
            std::cerr << localized("Failed to connect") << std::endl;
         }

         if (verbose_errors) {
            elog("connect error: ${e}", ("e", errorString));
         }
   } catch (const fc::exception& e) {
      // attempt to extract the error code if one is present
      if (!print_recognized_errors(e, verbose_errors)) {
         // Error is not recognized
         if (!print_help_text(e) || verbose_errors) {
            elog("Failed with error: ${e}", ("e", verbose_errors ? e.to_detail_string() : e.to_string()));
         }
      }
      return 1;
   }

   return 0;
}
