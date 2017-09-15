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

#define format_output(format, ...) fc::format_string(format, fc::mutable_variant_object() __VA_ARGS__ )

using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;
using namespace eos::client::help;
using namespace boost::locale;

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
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
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
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
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

   cmd->add_option("-x,--expiration", parse_exipration, gettext("option_expiration_description"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, gettext("flag_force_unique_description"));
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
      FC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
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
   app.add_option( "-H,--host", host, gettext("option_host_description"), true );
   app.add_option( "-p,--port", port, gettext("option_port_description"), true );
   app.add_option( "--wallet-host", wallet_host, gettext("option_wallet_host_description"), true );
   app.add_option( "--wallet-port", wallet_port, gettext("option_wallet_port_description"), true );

   bool verbose_errors = false;
   app.add_flag( "-v,--verbose", verbose_errors, gettext("flag_verbose_description"));

   // Create subcommand
   auto create = app.add_subcommand("create", gettext("subcommand_create_description"), false);
   create->require_subcommand();

   // create key
   create->add_subcommand("key", gettext("subcommand_create_key_description"))->set_callback([] {
      auto privateKey = fc::ecc::private_key::generate();
      std::cout << "Private key: " << key_to_wif(privateKey.get_secret()) << "\n";
      std::cout << "Public key:  " << string(public_key_type(privateKey.get_public_key())) << std::endl;
   });

   // create account
   string creator;
   string name;
   string ownerKey;
   string activeKey;
   bool skip_sign = false;
   auto createAccount = create->add_subcommand("account", gettext("subcommand_create_account_description"), false);
   createAccount->add_option("creator", creator, gettext("option_creator_description"))->required();
   createAccount->add_option("name", name, gettext("option_name_description"))->required();
   createAccount->add_option("OwnerKey", ownerKey, gettext("option_OwnerKey_description"))->required();
   createAccount->add_option("ActiveKey", activeKey, gettext("option_ActiveKey_description"))->required();
   createAccount->add_flag("-s,--skip-signature", skip_sign, gettext("flag_skip_signature_description"));
   createAccount->set_callback([&] {
      create_account(creator, name, public_key_type(ownerKey), public_key_type(activeKey), !skip_sign);
   });

   // create producer
   vector<string> permissions;
   auto createProducer = create->add_subcommand("producer", gettext("subcommand_create_producer_description"), false);
   createProducer->add_option("name", name, gettext("option_producer_name_description"))->required();
   createProducer->add_option("OwnerKey", ownerKey, gettext("option_producer_OwnerKey_description"))->required();
   createProducer->add_option("-p,--permission", permissions,
                              gettext("option_permission_description"));
   createProducer->add_flag("-s,--skip-signature", skip_sign, gettext("flag_skip_signature_description"));
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
   auto get = app.add_subcommand("get", gettext("subcommand_get_description"), false);
   get->require_subcommand();

   // get info
   get->add_subcommand("info", gettext("subcommand_get_info_description"))->set_callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", gettext("subcommand_get_block_description"), false);
   getBlock->add_option("block", blockArg, gettext("option_block_description"))->required();
   getBlock->set_callback([&blockArg] {
      auto arg = fc::mutable_variant_object("block_num_or_id", blockArg);
      std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
   });

   // get account
   string accountName;
   auto getAccount = get->add_subcommand("account", gettext("subcommand_get_account_description"), false);
   getAccount->add_option("name", accountName, gettext("option_name_description"))->required();
   getAccount->set_callback([&] {
      std::cout << fc::json::to_pretty_string(call(get_account_func,
                                                   fc::mutable_variant_object("name", accountName)))
                << std::endl;
   });

   // get code
   string codeFilename;
   string abiFilename;
   auto getCode = get->add_subcommand("code", gettext("subcommand_get_code_description"), false);
   getCode->add_option("name", accountName, gettext("option_name_description"))->required();
   getCode->add_option("-c,--code",codeFilename, gettext("option_code_description") );
   getCode->add_option("-a,--abi",abiFilename, gettext("option_abi_description") );
   getCode->set_callback([&] {
      auto result = call(get_code_func, fc::mutable_variant_object("name", accountName));

      std::cout << "code hash: " << result["code_hash"].as_string() <<"\n";

      if( codeFilename.size() ){
         std::cout << "saving wast to " << codeFilename <<"\n";
         auto code = result["wast"].as_string();
         std::ofstream out( codeFilename.c_str() );
         out << code;
      }
      if( abiFilename.size() ) {
         std::cout << "saving abi to " << abiFilename <<"\n";
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
   auto getTable = get->add_subcommand( "table", gettext("subcommand_get_table_description"), false);
   getTable->add_option( "scope", scope, gettext("option_scope_description") )->required();
   getTable->add_option( "contract", code, gettext("option_contract_description") )->required();
   getTable->add_option( "table", table, gettext("option_table_description") )->required();
   getTable->add_option( "-b,--binary", binary, gettext("option_binary_description") );
   getTable->add_option( "-l,--limit", limit, gettext("option_limit_description") );
   getTable->add_option( "-k,--key", limit, gettext("option_key_description") );
   getTable->add_option( "-L,--lower", lower, gettext("option_lower_description") );
   getTable->add_option( "-U,--upper", upper, gettext("option_upper_description") );

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
   auto getAccounts = get->add_subcommand("accounts", gettext("subcommand_get_accounts_description"), false);
   getAccounts->add_option("public_key", publicKey, gettext("option_public_key_description"))->required();
   getAccounts->set_callback([&] {
      auto arg = fc::mutable_variant_object( "public_key", publicKey);
      std::cout << fc::json::to_pretty_string(call(get_key_accounts_func, arg)) << std::endl;
   });

   // get servants
   string controllingAccount;
   auto getServants = get->add_subcommand("servants", gettext("subcommand_get_servants_description"), false);
   getServants->add_option("account", controllingAccount, gettext("option_account_description"))->required();
   getServants->set_callback([&] {
      auto arg = fc::mutable_variant_object( "controlling_account", controllingAccount);
      std::cout << fc::json::to_pretty_string(call(get_controlled_accounts_func, arg)) << std::endl;
   });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", gettext("subcommand_get_transaction_description"), false);
   getTransaction->add_option("id", transactionId, gettext("option_id_description"))->required();
   getTransaction->set_callback([&] {
      auto arg= fc::mutable_variant_object( "transaction_id", transactionId);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // get transactions
   string account_name;
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", gettext("subcommand_get_transactions_description"), false);
   getTransactions->add_option("account_name", account_name, gettext("option_account_name_description"))->required();
   getTransactions->add_option("skip_seq", skip_seq, gettext("option_skip_seq_description"));
   getTransactions->add_option("num_seq", num_seq, gettext("option_num_seq_description"));
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
   auto setSubcommand = app.add_subcommand("set", gettext("subcommand_set_description"));
   setSubcommand->require_subcommand();

   // set contract subcommand
   string account;
   string wastPath;
   string abiPath;
   auto contractSubcommand = setSubcommand->add_subcommand("contract", gettext("subcommand_set_contract_description"));
   contractSubcommand->add_option("account", account, gettext("option_account_description"))->required();
   contractSubcommand->add_option("wast-file", wastPath, gettext("option_wast_file_description"))->required()
         ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, gettext("option_abi_file_description"))
              ->check(CLI::ExistingFile);
   contractSubcommand->add_flag("-s,--skip-sign", skip_sign, gettext("flag_skip_sign_description"));
   contractSubcommand->set_callback([&] {
      std::string wast;
      std::cout << "Reading WAST..." << std::endl;
      fc::read_file_contents(wastPath, wast);
      std::cout << "Assembling WASM..." << std::endl;
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

      std::cout << "Publishing contract..." << std::endl;
      std::cout << fc::json::to_pretty_string(push_transaction(trx, !skip_sign)) << std::endl;
   });

   // set producer approve/unapprove subcommand
   string producer;
   auto producerSubcommand = setSubcommand->add_subcommand("producer", gettext("subcommand_set_producer_description"));
   producerSubcommand->require_subcommand();
   auto approveCommand = producerSubcommand->add_subcommand("approve", gettext("subcommand_set_producer_approve_description"));
   auto unapproveCommand = producerSubcommand->add_subcommand("unapprove", gettext("subcommand_set_producer_unapprove_description"));
   producerSubcommand->add_option("user-name", name, gettext("option_user_name_description"))->required();
   producerSubcommand->add_option("producer-name", producer, gettext("option_producer_name_description"))->required();
   producerSubcommand->add_option("-p,--permission", permissions,
                              gettext("option_permission_description"));
   producerSubcommand->add_flag("-s,--skip-signature", skip_sign, gettext("flag_skip_signature_description"));
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
      std::cout << "Set producer approval from " << name << " for " << producer << " to "
                << (approve ? "" : "un") << "approve." << std::endl;
   });

   // set proxy subcommand
   string proxy;
   auto proxySubcommand = setSubcommand->add_subcommand("proxy", gettext("subcommand_set_proxy_description"));
   proxySubcommand->add_option("user-name", name, gettext("option_user_name_description"))->required();
   proxySubcommand->add_option("proxy-name", proxy, gettext("option_proxy_name_description"));
   proxySubcommand->add_option("-p,--permission", permissions,
                                  gettext("option_permission_description"));
   proxySubcommand->add_flag("-s,--skip-signature", skip_sign, gettext("flag_skip_signature_description"));
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
      std::cout << "Set proxy for " << name << " to " << proxy << std::endl;
   });

   // Transfer subcommand
   string sender;
   string recipient;
   uint64_t amount;
   string memo;
   auto transfer = app.add_subcommand("transfer", gettext("subcommand_transfer_description"), false);
   transfer->add_option("sender", sender, gettext("option_sender_description"))->required();
   transfer->add_option("recipient", recipient, gettext("option_recipient_description"))->required();
   transfer->add_option("amount", amount, gettext("option_amount_description"))->required();
   transfer->add_option("memo", memo, gettext("option_memo_description"));
   transfer->add_flag("-s,--skip-sign", skip_sign, gettext("flag_skip_sign_description"));
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
   auto wallet = app.add_subcommand( "wallet", gettext("subcommand_wallet_description"), false );
   wallet->require_subcommand();
   // create wallet
   string wallet_name = "default";
   auto createWallet = wallet->add_subcommand("create", gettext("subcommand_wallet_create_description"), false);
   createWallet->add_option("-n,--name", wallet_name, gettext("option_name_description"), true);
   createWallet->set_callback([&wallet_name] {
      const auto& v = call(wallet_host, wallet_port, wallet_create, wallet_name);
      std::cout << "Creating wallet: " << wallet_name << std::endl;
      std::cout << "Save password to use in the future to unlock this wallet." << std::endl;
      std::cout << "Without password imported keys will not be retrievable." << std::endl;
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // open wallet
   auto openWallet = wallet->add_subcommand("open", gettext("subcommand_wallet_open_description"), false);
   openWallet->add_option("-n,--name", wallet_name, gettext("option_name_description"));
   openWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_open, wallet_name);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << "Opened: '"<<wallet_name<<"'\n";
   });

   // lock wallet
   auto lockWallet = wallet->add_subcommand("lock", gettext("subcommand_wallet_lock_description"), false);
   lockWallet->add_option("-n,--name", wallet_name, gettext("option_name_description"));
   lockWallet->set_callback([&wallet_name] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock, wallet_name);
      std::cout << "Locked: '"<<wallet_name<<"'\n";
      //std::cout << fc::json::to_pretty_string(v) << std::endl;

   });

   // lock all wallets
   auto locakAllWallets = wallet->add_subcommand("lock_all", gettext("subcommand_wallet_lock_all_description"), false);
   locakAllWallets->set_callback([] {
      /*const auto& v = */call(wallet_host, wallet_port, wallet_lock_all);
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
      std::cout << "Locked All Wallets\n";
   });

   // unlock wallet
   string wallet_pw;
   auto unlockWallet = wallet->add_subcommand("unlock", gettext("subcommand_wallet_unlock_description"), false);
   unlockWallet->add_option("-n,--name", wallet_name, gettext("option_name_description"));
   unlockWallet->add_option("--password", wallet_pw, gettext("option_password_description"));
   unlockWallet->set_callback([&wallet_name, &wallet_pw] {
      if( wallet_pw.size() == 0 ) {
         std::cout << "password: ";
         fc::set_console_echo(false);
         std::getline( std::cin, wallet_pw, '\n' );
         fc::set_console_echo(true);
      }


      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_pw)};
      /*const auto& v = */call(wallet_host, wallet_port, wallet_unlock, vs);
      std::cout << "Unlocked: '"<<wallet_name<<"'\n";
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // import keys into wallet
   string wallet_key;
   auto importWallet = wallet->add_subcommand("import", gettext("subcommand_wallet_import_description"), false);
   importWallet->add_option("-n,--name", wallet_name, gettext("option_name_description"));
   importWallet->add_option("key", wallet_key, gettext("option_key_description"))->required();
   importWallet->set_callback([&wallet_name, &wallet_key] {
      auto key = utilities::wif_to_key(wallet_key);
      FC_ASSERT( key, "invalid private key: ${k}", ("k",wallet_key) );
      public_key_type pubkey = key->get_public_key();

      fc::variants vs = {fc::variant(wallet_name), fc::variant(wallet_key)};
      const auto& v = call(wallet_host, wallet_port, wallet_import_key, vs);
      std::cout << "imported private key for: " << std::string(pubkey) <<"\n";
      //std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list wallets
   auto listWallet = wallet->add_subcommand("list", gettext("subcommand_wallet_list_description"), false);
   listWallet->set_callback([] {
      std::cout << "Wallets: \n";
      const auto& v = call(wallet_host, wallet_port, wallet_list);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // list keys
   auto listKeys = wallet->add_subcommand("keys", gettext("subcommand_wallet_keys_description"), false);
   listKeys->set_callback([] {
      const auto& v = call(wallet_host, wallet_port, wallet_list_keys);
      std::cout << fc::json::to_pretty_string(v) << std::endl;
   });

   // Benchmark subcommand
   auto benchmark = app.add_subcommand( "benchmark", gettext("subcommand_benchmark_description"), false );
   benchmark->require_subcommand();
   auto benchmark_setup = benchmark->add_subcommand( "setup", gettext("subcommand_setup_description") );
   uint64_t number_of_accounts = 2;
   benchmark_setup->add_option("accounts", number_of_accounts, gettext("option_accounts_description"))->required();
   benchmark_setup->set_callback([&]{
      std::cerr << "Creating " << number_of_accounts <<" accounts with initial balances\n";
      FC_ASSERT( number_of_accounts >= 2, "must create at least 2 accounts" );

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

   auto benchmark_transfer = benchmark->add_subcommand( "transfer", gettext("subcommand_benchmark_transfer_description") );
   uint64_t number_of_transfers = 0;
   bool loop = false;
   benchmark_transfer->add_option("accounts", number_of_accounts, gettext("option_accounts_description"))->required();
   benchmark_transfer->add_option("count", number_of_transfers, gettext("option_count_description"))->required();
   benchmark_transfer->add_option("loop", loop, gettext("option_loop_description"));
   benchmark_transfer->set_callback([&]{
      FC_ASSERT( number_of_accounts > 1 );

      std::cerr << "funding "<< number_of_accounts << " accounts from init\n";
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


      std::cerr << "generating random "<< number_of_transfers << " transfers among " << number_of_accounts << " benchmark accounts\n";
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
   auto push = app.add_subcommand("push", gettext("subcommand_push_description"), false);
   push->require_subcommand();

   // push message
   string contract;
   string action;
   string data;
   vector<string> scopes;
   auto messageSubcommand = push->add_subcommand("message", gettext("subcommand_push_message_description"));
   messageSubcommand->fallthrough(false);
   messageSubcommand->add_option("contract", contract,
                                 gettext("option_contract_description"), true)->required();
   messageSubcommand->add_option("action", action, gettext("option_action_description"), true)
         ->required();
   messageSubcommand->add_option("data", data, gettext("option_data_description"))->required();
   messageSubcommand->add_option("-p,--permission", permissions,
                                 gettext("option_permission_description"));
   messageSubcommand->add_option("-S,--scope", scopes, gettext("option_scope_description"), true);
   messageSubcommand->add_flag("-s,--skip-sign", skip_sign, gettext("flag_skip_sign_description"));
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
   auto trxSubcommand = push->add_subcommand("transaction", gettext("subcommand_push_transaction_description"));
   trxSubcommand->add_option("transaction", trxJson, gettext("option_transaction_description"))->required();
   trxSubcommand->set_callback([&] {
      auto trx_result = call(push_txn_func, fc::json::from_string(trxJson));
      std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
   });


   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", gettext("subcommand_push_transactions_description"));
   trxsSubcommand->add_option("transactions", trxsJson, gettext("option_transactions_description"))->required();
   trxsSubcommand->set_callback([&] {
      auto trxs_result = call(push_txn_func, fc::json::from_string(trxsJson));
      std::cout << fc::json::to_pretty_string(trxs_result) << std::endl;
   });

   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const fc::exception& e) {
      auto errorString = e.to_detail_string();
      if (errorString.find("Connection refused") != string::npos) {
         if (errorString.find(fc::json::to_string(port)) != string::npos) {
            std::cerr << format_output("Failed to connect to eosd at ${ip}:${port}; is eosd running?", ("ip", host)("port", port)) << std::endl;
         } else if (errorString.find(fc::json::to_string(wallet_port)) != string::npos) {
            std::cerr << format_output("Failed to connect to eos-walletd at ${ip}:${port}; is eos-walletd running?", ("ip", wallet_host)("port", wallet_port)) << std::endl;
         } else {
            std::cerr << format_output("Failed to connect") << std::endl;
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
