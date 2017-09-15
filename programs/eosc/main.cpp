/**
 *  @file
 *  @defgroup eosclienttool EOS Command Line Client Reference
 *  @brief Tool for sending transactions and querying state from @ref eosd
 *  @ingroup eosclienttool
 */

/**
  @defgroup eosclienttool

  @section intro Introduction to EOSC

  `eosc` is a command line tool that interfaces with the REST api exposed by @ref eosd. In order to use `eosc` you will need to
  have a local copy of `eosd` running and configured to load the 'eos::chain_api_plugin'.

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
#include <boost/locale.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
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

#define localize(str, ...) fc::format_string(boost::locale::gettext(str), fc::mutable_variant_object() __VA_ARGS__ )

using namespace boost::locale;
using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;
using namespace eos::client::help;

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );
FC_DECLARE_EXCEPTION( localized_exception, 10000000, "an error occured" );
#define EOSC_ASSERT( TEST, ... ) \
  FC_EXPAND_MACRO( \
    FC_MULTILINE_MACRO_BEGIN \
      if( UNLIKELY(!(TEST)) ) \
      {                                                   \
        std::cerr << localize( __VA_ARGS__ ) << std::endl;  \
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

vector<uint8_t> assemble_wast( const std::string& wast ) {
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << localize("wast_parse_error_help_text") << std::endl;
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
      std::cerr << localize("wast_serialize_error_help_text") << std::endl;
      std::cerr << exception.message << std::endl;
      FC_THROW_EXCEPTION( explained_exception, "wasm serialize error");
   }
}

auto tx_expiration = fc::microseconds(100);
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

   cmd->add_option("-x,--expiration", parse_exipration, localize("!?expiration"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localize("!?force_unique"));
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
      EOSC_ASSERT(pieces.size() == 2, "invalid_permission_assert", ("p", p));
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

eos::chain_apis::read_only::get_info_results get_info() {
  return call(host, port, get_info_func ).as<eos::chain_apis::read_only::get_info_results>();
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
    trx.expiration = info.head_block_time + tx_expiration; //chain.head_block_time() + 100;
    transaction_set_reference_block(trx, info.head_block_id);
    boost::sort( trx.scope );

    if (sign) {
       sign_transaction(trx);
    }

    return call( push_txn_func, trx );
}


void create_account(Name creator, Name newaccount, public_key_type owner, public_key_type active, bool sign) {
      auto owner_auth   = eos::chain::Authority{1, {{owner, 1}}, {}};
      auto active_auth  = eos::chain::Authority{1, {{active, 1}}, {}};
      auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};

      uint64_t deposit = 1;

      SignedTransaction trx;
      trx.scope = sort_names({creator,config::EosContractName});
      transaction_emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
                                           types::newaccount{creator, newaccount, owner_auth,
                                                             active_auth, recovery_auth, deposit});
      std::cout << fc::json::to_pretty_string(push_transaction(trx, sign)) << std::endl;
}

int main( int argc, char** argv ) {
   generator gen;
   gen.add_messages_path("./locale");
   gen.add_messages_domain("eosc");
   locale::global(gen(""));

   bool localization_available = false;
   if (std::has_facet<message_format<char>>(std::locale())) {
      localization_available = std::use_facet<message_format<char>>(std::locale()).get(0, nullptr, "translation_canary") != nullptr;
   }

   if (!localization_available) {
      // make sure we translate our ID's to a "reasonable" default
      locale::global(gen("en_US.UTF-8"));
   }
   
   cout.imbue(locale());
   cerr.imbue(locale());

   CLI::App app{"Command Line Interface to Eos Daemon"};
   app.require_subcommand();
   app.add_option( "-H,--host", host, localize("!?host"), true );
   app.add_option( "-p,--port", port, localize("!?port"), true );
   app.add_option( "--wallet-host", wallet_host, localize("!?wallet_host"), true );
   app.add_option( "--wallet-port", wallet_port, localize("!?wallet_port"), true );

   bool verbose_errors = false;
   app.add_flag( "-v,--verbose", verbose_errors, localize("!?verbose"));

   // Create subcommand
   auto create = app.add_subcommand("create", localize("!create"), false);
   create->require_subcommand();

   // create key
   create->add_subcommand("key", localize("!create_key"))->set_callback([] {
      auto privateKey = fc::ecc::private_key::generate();
      std::cout << localize("private_key_label") << key_to_wif(privateKey.get_secret()) << "\n";
      std::cout << localize("public_key_label") << string(public_key_type(privateKey.get_public_key())) << std::endl;
   });

   // create account
   string creator;
   string name;
   string ownerKey;
   string activeKey;
   bool skip_sign = false;
   auto createAccount = create->add_subcommand("account", localize("!create_account"), false);
   createAccount->add_option("creator", creator, localize("!create_account?creator"))->required();
   createAccount->add_option("name", name, localize("!create_account?name"))->required();
   createAccount->add_option("OwnerKey", ownerKey, localize("!create_account?OwnerKey"))->required();
   createAccount->add_option("ActiveKey", activeKey, localize("!create_account?ActiveKey"))->required();
   createAccount->add_flag("-s,--skip-signature", skip_sign, localize("!create_account?skip_signature"));
   createAccount->set_callback([&] {
      create_account(creator, name, public_key_type(ownerKey), public_key_type(activeKey), !skip_sign);
   });

   // create producer
   vector<string> permissions;
   auto createProducer = create->add_subcommand("producer", localize("!create_producer"), false);
   createProducer->add_option("name", name, localize("!create_producer?producer_name"))->required();
   createProducer->add_option("OwnerKey", ownerKey, localize("!create_producer?producer_OwnerKey"))->required();
   createProducer->add_option("-p,--permission", permissions,
                              localize("!create_producer?permission"));
   createProducer->add_flag("-s,--skip-signature", skip_sign, localize("!create_producer?skip_signature"));
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
   auto get = app.add_subcommand("get", localize("!get"), false);
   get->require_subcommand();

   // get info
   get->add_subcommand("info", localize("!get_info"))->set_callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", localize("!get_block"), false);
   getBlock->add_option("block", blockArg, localize("!get_block?block"))->required();
   getBlock->set_callback([&blockArg] {
      auto arg = fc::mutable_variant_object("block_num_or_id", blockArg);
      std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
   });

   // get account
   string accountName;
   auto getAccount = get->add_subcommand("account", localize("!get_account"), false);
   getAccount->add_option("name", accountName, localize("!get_account?name"))->required();
   getAccount->set_callback([&] {
      std::cout << fc::json::to_pretty_string(call(get_account_func,
                                                   fc::mutable_variant_object("name", accountName)))
                << std::endl;
   });

   // get code
   string codeFilename;
   string abiFilename;
   auto getCode = get->add_subcommand("code", localize("!get_code"), false);
   getCode->add_option("name", accountName, localize("!get_code?name"))->required();
   getCode->add_option("-c,--code",codeFilename, localize("!get_code?code") );
   getCode->add_option("-a,--abi",abiFilename, localize("!get_code?abi") );
   getCode->set_callback([&] {
      auto result = call(get_code_func, fc::mutable_variant_object("name", accountName));

      std::cout << localize("code_hash_label") << result["code_hash"].as_string() <<"\n";

      if( codeFilename.size() ){
         std::cout << localize("saving_wast_to_label") << codeFilename <<"\n";
         auto code = result["wast"].as_string();
         std::ofstream out( codeFilename.c_str() );
         out << code;
      }
      if( abiFilename.size() ) {
         std::cout << localize("saving_abi_to_label") << abiFilename <<"\n";
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
   auto getTable = get->add_subcommand( "table", localize("!get_table"), false);
   getTable->add_option( "scope", scope, localize("!get_table?scope") )->required();
   getTable->add_option( "contract", code, localize("!get_table?contract") )->required();
   getTable->add_option( "table", table, localize("!get_table?table") )->required();
   getTable->add_option( "-b,--binary", binary, localize("!get_table?binary") );
   getTable->add_option( "-l,--limit", limit, localize("!get_table?limit") );
   getTable->add_option( "-k,--key", limit, localize("!get_table?key") );
   getTable->add_option( "-L,--lower", lower, localize("!get_table?lower") );
   getTable->add_option( "-U,--upper", upper, localize("!get_table?upper") );

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
   auto getAccounts = get->add_subcommand("accounts", localize("!get_accounts"), false);
   getAccounts->add_option("public_key", publicKey, localize("!get_accounts?public_key"))->required();
   getAccounts->set_callback([&] {
      auto arg = fc::mutable_variant_object( "public_key", publicKey);
      std::cout << fc::json::to_pretty_string(call(get_key_accounts_func, arg)) << std::endl;
   });

   // get servants
   string controllingAccount;
   auto getServants = get->add_subcommand("servants", localize("!get_servants"), false);
   getServants->add_option("account", controllingAccount, localize("!get_servants?account"))->required();
   getServants->set_callback([&] {
      auto arg = fc::mutable_variant_object( "controlling_account", controllingAccount);
      std::cout << fc::json::to_pretty_string(call(get_controlled_accounts_func, arg)) << std::endl;
   });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", localize("!get_transaction"), false);
   getTransaction->add_option("id", transactionId, localize("!get_transaction?id"))->required();
   getTransaction->set_callback([&] {
      auto arg= fc::mutable_variant_object( "transaction_id", transactionId);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get transactions
   string account_name;
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", localize("!get_transactions"), false);
   getTransactions->add_option("account_name", account_name, localize("!get_transactions?account_name"))->required();
   getTransactions->add_option("skip_seq", skip_seq, localize("!get_transactions?skip_seq"));
   getTransactions->add_option("num_seq", num_seq, localize("!get_transactions?num_seq"));
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
   auto setSubcommand = app.add_subcommand("set", localize("!set"));
   setSubcommand->require_subcommand();

   // set contract subcommand
   string account;
   string wastPath;
   string abiPath;
   auto contractSubcommand = setSubcommand->add_subcommand("contract", localize("!set_contract"));
   contractSubcommand->add_option("account", account, localize("!set_contract?account"))->required();
   contractSubcommand->add_option("wast-file", wastPath, localize("!set_contract?wast_file"))->required()
         ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, localize("!set_contract?abi_file"))
              ->check(CLI::ExistingFile);
   contractSubcommand->add_flag("-s,--skip-sign", skip_sign, localize("!set_contract?skip_sign"));
   contractSubcommand->set_callback([&] {
      std::string wast;
      std::cout << localize("reading_wast_status") << std::endl;
      fc::read_file_contents(wastPath, wast);
      std::cout << localize("assembling_wast_status") << std::endl;
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

      std::cout << localize("publishing_contract_status") << std::endl;
      std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign)) << std::endl;
   });

   // set producer approve/unapprove subcommand
   string producer;
   auto producerSubcommand = setSubcommand->add_subcommand("producer", localize("!set_producer"));
   producerSubcommand->require_subcommand();
   auto approveCommand = producerSubcommand->add_subcommand("approve", localize("!set_producer_approve"));
   auto unapproveCommand = producerSubcommand->add_subcommand("unapprove", localize("!set_producer_unapprove"));
   producerSubcommand->add_option("user-name", name, localize("!set_producer_unapprove?user_name"))->required();
   producerSubcommand->add_option("producer-name", producer, localize("!set_producer_unapprove?producer_name"))->required();
   producerSubcommand->add_option("-p,--permission", permissions,
                              localize("!set_producer_unapprove?permission"));
   producerSubcommand->add_flag("-s,--skip-signature", skip_sign, localize("!set_producer_unapprove?skip_signature"));
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
      std::cout << localize("producer_approval_result", 
            ("name", name)("producer", producer)("value", approve ? "approve" : "unapprove"))
          << std::endl;
   });

   // set proxy subcommand
   string proxy;
   auto proxySubcommand = setSubcommand->add_subcommand("proxy", localize("!set_proxy"));
   proxySubcommand->add_option("user-name", name, localize("!set_proxy?user_name"))->required();
   proxySubcommand->add_option("proxy-name", proxy, localize("!set_proxy?proxy_name"));
   proxySubcommand->add_option("-p,--permission", permissions,
                                  localize("!set_proxy?permission"));
   proxySubcommand->add_flag("-s,--skip-signature", skip_sign, localize("!set_proxy?skip_signature"));
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
      std::cout << localize("set_proxy_result", ("name", name)("proxy", proxy)) << std::endl;
   });

   // Transfer subcommand
   string sender;
   string recipient;
   uint64_t amount;
   string memo;
   auto transfer = app.add_subcommand("transfer", localize("!transfer"), false);
   transfer->add_option("sender", sender, localize("!transfer?sender"))->required();
   transfer->add_option("recipient", recipient, localize("!transfer?recipient"))->required();
   transfer->add_option("amount", amount, localize("!transfer?amount"))->required();
   transfer->add_option("memo", memo, localize("!transfer?memo"));
   transfer->add_flag("-s,--skip-sign", skip_sign, localize("!transfer?skip_sign"));
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
      trx.expiration = info.head_block_time + tx_expiration; //chain.head_block_time() + 100;
      transaction_set_reference_block(trx, info.head_block_id);
      if (!skip_sign) {
         sign_transaction(trx);
      }

      std::cout << fc::json::to_pretty_string( call( push_txn_func, trx )) << std::endl;
   });


   // Wallet subcommand
   auto wallet = app.add_subcommand( "wallet", localize("!wallet"), false );
   wallet->require_subcommand();
   // create wallet
   string wallet_name = "default";
   auto createWallet = wallet->add_subcommand("create", localize("!wallet_create"), false);
   createWallet->add_option("-n,--name", wallet_name, localize("!wallet_create?name"), true);
   createWallet->set_callback([&wallet_name] {
      const auto& v = call(wallet_host, wallet_port, wallet_create, wallet_name);
      std::cout << localize("wallet_create_result", ("name", wallet_name)) << std::endl;
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // open wallet
   auto openWallet = wallet->add_subcommand("open", localize("!wallet_open"), false);
   openWallet->add_option("-n,--name", wallet_name, localize("!wallet_open?name"));
   openWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_open, wallet_name);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << localize("wallet_open_result", ("name",wallet_name)) << std::endl;
   });

   // lock wallet
   auto lockWallet = wallet->add_subcommand("lock", localize("!wallet_lock"), false);
   lockWallet->add_option("-n,--name", wallet_name, localize("!wallet_lock?name"));
   lockWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock, wallet_name);
      std::cout << localize("wallet_lock_result", ("name",wallet_name)) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;

   });

   // lock all wallets
   auto locakAllWallets = wallet->add_subcommand("lock_all", localize("!wallet_lock_all"), false);
   locakAllWallets->set_callback([] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock_all);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << localize("wallet_lock_all_result") << std::endl;
   });

   // unlock wallet
   string wallet_pw;
   auto unlockWallet = wallet->add_subcommand("unlock", localize("!wallet_unlock"), false);
   unlockWallet->add_option("-n,--name", wallet_name, localize("!wallet_unlock?name"));
   unlockWallet->add_option("--password", wallet_pw, localize("!wallet_unlock?password"));
   unlockWallet->set_callback([&wallet_name, &wallet_pw] {
      if( wallet_pw.size() == 0 ) {
         std::cout << localize("password_prompt");
         fc::set_console_echo(false);
         std::getline( std::cin, wallet_pw, '\n' );
         fc::set_console_echo(true);
      }


      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw)};
      /*const auto& v = */call(wallet_host, wallet_port, wallet_unlock, vs);
      std::cout << localize("wallet_unlock_result", ("name",wallet_name)) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // import keys into wallet
   string wallet_key;
   auto importWallet = wallet->add_subcommand("import", localize("!wallet_import"), false);
   importWallet->add_option("-n,--name", wallet_name, localize("!wallet_import?name"));
   importWallet->add_option("key", wallet_key, localize("!wallet_import?key"))->required();
   importWallet->set_callback([&wallet_name, &wallet_key] {
      auto key = utilities::wif_to_key(wallet_key);
      EOSC_ASSERT( key, "invalid_private_key_assert", ("k",wallet_key) );
      public_key_type pubkey = key->get_public_key();

      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_key)};
      const auto& v = call(wallet_host, wallet_port, wallet_import_key, vs);
      std::cout << localize("wallet_import_result", ("pubkey", std::string(pubkey))) << std::endl;
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list wallets
   auto listWallet = wallet->add_subcommand("list", localize("!wallet_list"), false);
   listWallet->set_callback([] {
      std::cout << localize("wallet_list_result") << std::endl;
      const auto& v = call(wallet_host, wallet_port, wallet_list);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list keys
   auto listKeys = wallet->add_subcommand("keys", localize("!wallet_keys"), false);
   listKeys->set_callback([] {
      const auto& v = call(wallet_host, wallet_port, wallet_list_keys);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // Benchmark subcommand
   auto benchmark = app.add_subcommand( "benchmark", localize("!benchmark"), false );
   benchmark->require_subcommand();
   auto benchmark_setup = benchmark->add_subcommand( "setup", localize("!setup") );
   uint64_t number_of_accounts = 2;
   benchmark_setup->add_option("accounts", number_of_accounts, localize("!setup?accounts"))->required();
   benchmark_setup->set_callback([&]{
      std::cerr << localize("benchmark_create_accounts_status", ("accounts", number_of_accounts)) << std::endl;
      EOSC_ASSERT( number_of_accounts >= 2, "benchmark_too_few_accounts_assert" );

      auto info = get_info();

      vector<SignedTransaction> batch;
      batch.reserve( number_of_accounts );
      for( uint32_t i = 0; i < number_of_accounts; ++i ) {
        Name newaccount( Name("benchmark").value + i );
        public_key_type owner, active;
        Name creator("inita" );

        auto owner_auth   = eos::chain::Authority{1, {{owner, 1}}, {}};
        auto active_auth  = eos::chain::Authority{1, {{active, 1}}, {}};
        auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};
        
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

   auto benchmark_transfer = benchmark->add_subcommand( "transfer", localize("!benchmark_transfer") );
   uint64_t number_of_transfers = 0;
   bool loop = false;
   benchmark_transfer->add_option("accounts", number_of_accounts, localize("!benchmark_transfer?accounts"))->required();
   benchmark_transfer->add_option("count", number_of_transfers, localize("!benchmark_transfer?count"))->required();
   benchmark_transfer->add_option("loop", loop, localize("!benchmark_transfer?loop"));
   benchmark_transfer->set_callback([&]{
      EOSC_ASSERT( number_of_accounts >= 2, "benchmark_too_few_accounts_assert" );

      std::cerr << localize("benchmark_fund_accounts_status", ("accounts", number_of_accounts)) << std::endl;
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


      std::cerr << localize("benchmark_generate_transfer_status", ("transfers", number_of_transfers)("accounts", number_of_accounts)) << std::endl;
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
   auto push = app.add_subcommand("push", localize("!push"), false);
   push->require_subcommand();

   // push message
   string contract;
   string action;
   string data;
   vector<string> scopes;
   auto messageSubcommand = push->add_subcommand("message", localize("!push_message"));
   messageSubcommand->fallthrough(false);
   messageSubcommand->add_option("contract", contract,
                                 localize("!push_message?contract"), true)->required();
   messageSubcommand->add_option("action", action, localize("!push_message?action"), true)
         ->required();
   messageSubcommand->add_option("data", data, localize("!push_message?data"))->required();
   messageSubcommand->add_option("-p,--permission", permissions,
                                 localize("!push_message?permission"));
   messageSubcommand->add_option("-S,--scope", scopes, localize("!push_message?scope"), true);
   messageSubcommand->add_flag("-s,--skip-sign", skip_sign, localize("!push_message?skip_sign"));
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
   auto trxSubcommand = push->add_subcommand("transaction", localize("!push_transaction"));
   trxSubcommand->add_option("transaction", trxJson, localize("!push_transaction?transaction"))->required();
   trxSubcommand->set_callback([&] {
      auto trx_result = call(push_txn_func, fc::json::from_string(trxJson));
      std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
   });


   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", localize("!push_transactions"));
   trxsSubcommand->add_option("transactions", trxsJson, localize("!push_transactions?transactions"))->required();
   trxsSubcommand->set_callback([&] {
      auto trxs_result = call(push_txn_func, fc::json::from_string(trxsJson));
      std::cout << fc::json::to_pretty_string(trxs_result) << std::endl;
   });

   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const explained_exception&) {
      return 1;
   } catch (const fc::exception& e) {
      auto errorString = e.to_detail_string();
      if (errorString.find("Connection refused") != string::npos) {
         if (errorString.find(fc::json::to_string(port)) != string::npos) {
            std::cerr << localize("connect_failure_eosd_help_text", ("ip", host)("port", port)) << std::endl;
         } else if (errorString.find(fc::json::to_string(wallet_port)) != string::npos) {
            std::cerr << localize("connect_failure_wallet_help_text", ("ip", wallet_host)("port", wallet_port)) << std::endl;
         } else {
            std::cerr << localize("connect_failure_generic_text") << std::endl;
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
