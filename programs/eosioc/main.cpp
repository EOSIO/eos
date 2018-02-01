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
#include <eos/utilities/key_conversion.hpp>

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
#include "eosioc_helper.hpp"

using namespace std;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::utilities;
using namespace eosio::client::help;
using namespace eosio::client::localize;
using namespace eosio::client::config;
using namespace boost::filesystem;

FC_DECLARE_EXCEPTION( explained_exception, 9000000, "explained exception, see error log" );

string program = "eosc";

eosio::client::eosioc_helper helper;

bool tx_force_unique = false;

void add_standard_transaction_options(CLI::App* cmd) {
   CLI::callback_t parse_exipration = [](CLI::results_t res) -> bool {
      double value_s;
      if (res.size() == 0 || !CLI::detail::lexical_cast(res[0], value_s)) {
         return false;
      }

      helper.set_tx_expiration(fc::seconds(static_cast<uint64_t>(value_s)));
      return true;
   };

   cmd->add_option("-x,--expiration", parse_exipration, localized("set the time in seconds before a transaction expires, defaults to 30s"));
   cmd->add_flag("-f,--force-unique", tx_force_unique, localized("force the transaction to be unique. this will consume extra bandwidth and remove any protections against accidently issuing the same transaction multiple times"));
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

      permissions->set_callback([this] { helper.set_account_permission(accountStr,
                                                                    permissionStr,
                                                                    authorityJsonOrFile,
                                                                    parentStr,
                                                                    permissionAuth,
                                                                    skip_sign); });
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

      permissions->set_callback([this] { helper.set_action_permission(accountStr,
                                                                           codeStr,
                                                                           typeStr,
                                                                           requirementStr,
                                                                           skip_sign); });
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
   app.add_option( "-H,--host", [&](CLI::results_t res) {
       if(res.size() != 1)
           return false;

       std::string variable;
       if (!CLI::detail::lexical_cast(res[0], variable))
           return false;

       helper.set_host_peer(variable);
       return true; }, localized("the host where eosd is running"), true );
   app.add_option( "-p,--port", [&](CLI::results_t res) {
       if(res.size() != 1)
           return false;

       uint32_t variable;
       if (!CLI::detail::lexical_cast(res[0], variable))
           return false;

       helper.set_port_peer(variable);
       return true; }, localized("the port where eosd is running"), true );
   app.add_option( "--wallet-host", [&](CLI::results_t res) {
       if(res.size() != 1)
           return false;

       std::string variable;
       if (!CLI::detail::lexical_cast(res[0], variable))
           return false;

       helper.set_host_wallet(variable);
       return true; }, localized("the host where eos-walletd is running"), true );
   app.add_option( "--wallet-port", [&](CLI::results_t res) {
       if(res.size() != 1)
           return false;

       uint32_t variable;
       if (!CLI::detail::lexical_cast(res[0], variable))
           return false;

       helper.set_port_wallet(variable);
       return true; }, localized("the port where eos-walletd is running"), true );

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
   uint64_t staked_deposit=10000;
   auto createAccount = create->add_subcommand("account", localized("Create a new account on the blockchain"), false);
   createAccount->add_option("creator", creator, localized("The name of the account creating the new account"))->required();
   createAccount->add_option("name", account_name, localized("The name of the new account"))->required();
   createAccount->add_option("OwnerKey", ownerKey, localized("The owner public key for the account"))->required();
   createAccount->add_option("ActiveKey", activeKey, localized("The active public key for the account"))->required();
   createAccount->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   createAccount->add_option("--staked-deposit", staked_deposit, localized("the staked deposit transfered to the new account"));
   add_standard_transaction_options(createAccount);
   createAccount->set_callback([&] { helper.create_account(creator,
                                                                account_name,
                                                                public_key_type(ownerKey),
                                                                public_key_type(activeKey),
                                                                !skip_sign,
                                                                staked_deposit); });

   // create producer
   vector<string> permissions;
   auto createProducer = create->add_subcommand("producer", localized("Create a new producer on the blockchain"), false);
   createProducer->add_option("name", account_name, localized("The name of the new producer"))->required();
   createProducer->add_option("OwnerKey", ownerKey, localized("The public key for the producer"))->required();
   createProducer->add_option("-p,--permission", permissions,
                              localized("An account and permission level to authorize, as in 'account@permission' (default user@active)"));
   createProducer->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   add_standard_transaction_options(createProducer);
   createProducer->set_callback([&account_name, &ownerKey, &permissions, &skip_sign] {
       helper.create_producer(account_name, ownerKey, permissions, skip_sign);
      });

   // Get subcommand
   auto get = app.add_subcommand("get", localized("Retrieve various items and information from the blockchain"), false);
   get->require_subcommand();

   // get info
   get->add_subcommand("info", localized("Get current blockchain information"))->set_callback([] { helper.print_info(); });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", localized("Retrieve a full block from the blockchain"), false);
   getBlock->add_option("block", blockArg, localized("The number or ID of the block to retrieve"))->required();
   getBlock->set_callback([&blockArg] { helper.get_block(blockArg); });

   // get account
   string accountName;
   auto getAccount = get->add_subcommand("account", localized("Retrieve an account from the blockchain"), false);
   getAccount->add_option("name", accountName, localized("The name of the account to retrieve"))->required();
   getAccount->set_callback([&] { helper.get_account(accountName); });

   // get code
   string codeFilename;
   string abiFilename;
   auto getCode = get->add_subcommand("code", localized("Retrieve the code and ABI for an account"), false);
   getCode->add_option("name", accountName, localized("The name of the account whose code should be retrieved"))->required();
   getCode->add_option("-c,--code",codeFilename, localized("The name of the file to save the contract .wast to") );
   getCode->add_option("-a,--abi",abiFilename, localized("The name of the file to save the contract .abi to") );
   getCode->set_callback([&] { helper.save_code(accountName, codeFilename, abiFilename); });

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
   getTable->set_callback([&] { helper.print_table(scope, code, table); });

   // currency accessors
   // get currency balance
   string symbol;
   auto get_currency = get->add_subcommand( "currency", localized("Retrieve information related to standard currencies"), true);
   auto get_balance = get_currency->add_subcommand( "balance", localized("Retrieve the balance of an account for a given currency"), false);
   get_balance->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_balance->add_option( "account", accountName, localized("The account to query balances for") )->required();
   get_balance->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") );
   get_balance->set_callback([&] { helper.get_balance(accountName, code, symbol); });

   auto get_currency_stats = get_currency->add_subcommand( "stats", localized("Retrieve the stats of for a given currency"), false);
   get_currency_stats->add_option( "contract", code, localized("The contract that operates the currency") )->required();
   get_currency_stats->add_option( "symbol", symbol, localized("The symbol for the currency if the contract operates multiple currencies") );
   get_currency_stats->set_callback([&] { helper.get_currency_stats(code, symbol); });

   // get accounts
   string publicKey;
   auto getAccounts = get->add_subcommand("accounts", localized("Retrieve accounts associated with a public key"), false);
   getAccounts->add_option("public_key", publicKey, localized("The public key to retrieve accounts for"))->required();
   getAccounts->set_callback([&] { helper.get_key_accounts(publicKey); });

   // get servants
   string controllingAccount;
   auto getServants = get->add_subcommand("servants", localized("Retrieve accounts which are servants of a given account "), false);
   getServants->add_option("account", controllingAccount, localized("The name of the controlling account"))->required();
   getServants->set_callback([&] { helper.get_controlled_accounts(controllingAccount); });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", localized("Retrieve a transaction from the blockchain"), false);
   getTransaction->add_option("id", transactionId, localized("ID of the transaction to retrieve"))->required();
   getTransaction->set_callback([&] { helper.get_transaction(transactionId); });

   // get transactions
   string skip_seq;
   string num_seq;
   auto getTransactions = get->add_subcommand("transactions", localized("Retrieve all transactions with specific account name referenced in their scope"), false);
   getTransactions->add_option("account_name", account_name, localized("name of account to query on"))->required();
   getTransactions->add_option("skip_seq", skip_seq, localized("Number of most recent transactions to skip (0 would start at most recent transaction)"));
   getTransactions->add_option("num_seq", num_seq, localized("Number of transactions to return"));
   getTransactions->set_callback([&] { helper.get_transactions(account_name, skip_seq, num_seq); });

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
   contractSubcommand->add_option("abi-file,-a,--abi", abiPath, localized("The ABI for the contract"))
         ->check(CLI::ExistingFile);
   contractSubcommand->add_flag("-s,--skip-sign", skip_sign, localized("Specify if unlocked wallet keys should be used to sign transaction"));
   add_standard_transaction_options(contractSubcommand);
   contractSubcommand->set_callback([&] { helper.create_and_update_contract(account, wastPath, abiPath, skip_sign); });

   // set producer approve/unapprove subcommand
   string producer;
   auto producerSubcommand = setSubcommand->add_subcommand("producer", localized("Approve/unapprove producer"));
   producerSubcommand->require_subcommand();
   auto approveCommand = producerSubcommand->add_subcommand("approve", localized("Approve producer"));
   producerSubcommand->add_subcommand("unapprove", localized("Unapprove producer"));
   producerSubcommand->add_option("user-name", account_name, localized("The name of the account approving"))->required();
   producerSubcommand->add_option("producer-name", producer, localized("The name of the producer to approve"))->required();
   producerSubcommand->add_option("-p,--permission", permissions,
                              localized("An account and permission level to authorize, as in 'account@permission' (default user@active)"));
   producerSubcommand->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   add_standard_transaction_options(producerSubcommand);
   bool approve = producerSubcommand->got_subcommand(approveCommand);
   producerSubcommand->set_callback([&] { helper.approve_unapprove_producer(account_name,
                                                                                 producer,
                                                                                 permissions,
                                                                                 approve,
                                                                                 skip_sign); });

   // set proxy subcommand
   string proxy;
   auto proxySubcommand = setSubcommand->add_subcommand("proxy", localized("Set proxy account for voting"));
   proxySubcommand->add_option("user-name", account_name, localized("The name of the account to proxy from"))->required();
   proxySubcommand->add_option("proxy-name", proxy, localized("The name of the account to proxy (unproxy if not provided)"));
   proxySubcommand->add_option("-p,--permission", permissions,
                                  localized("An account and permission level to authorize, as in 'account@permission' (default user@active)"));
   proxySubcommand->add_flag("-s,--skip-signature", skip_sign, localized("Specify that unlocked wallet keys should not be used to sign transaction"));
   add_standard_transaction_options(proxySubcommand);
   proxySubcommand->set_callback([&] { helper.set_proxy_account_for_voting(account_name,
                                                                                proxy,
                                                                                permissions,
                                                                                skip_sign); });

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
   transfer->set_callback([&] { helper.transfer(sender,
                                                     recipient,
                                                     amount,
                                                     memo,
                                                     skip_sign,
                                                     tx_force_unique); });
   // Net subcommand
   string new_host;
   auto net = app.add_subcommand( "net", localized("Interact with local p2p network connections"), false );
   net->require_subcommand();
   auto connect = net->add_subcommand("connect", localized("start a new connection to a peer"), false);
   connect->add_option("host", new_host, localized("The hostname:port to connect to."))->required();
   connect->set_callback([&] { helper.start_new_connection_to_peer(new_host); });

   auto disconnect = net->add_subcommand("disconnect", localized("close an existing connection"), false);
   disconnect->add_option("host", new_host, localized("The hostname:port to disconnect from."))->required();
   disconnect->set_callback([&] { helper.close_connection_to_peer(new_host); });

   auto status = net->add_subcommand("status", localized("status of existing connection"), false);
   status->add_option("host", new_host, localized("The hostname:port to query status of connection"))->required();
   status->set_callback([&] { helper.status_of_connection_to_peer(new_host); });

   auto connections = net->add_subcommand("peers", localized("status of all existing peers"), false);
   connections->set_callback([&] { helper.status_of_all_existing_peers(new_host); });

   // Wallet subcommand
   auto wallet = app.add_subcommand( "wallet", localized("Interact with local wallet"), false );
   wallet->require_subcommand();
   // create wallet
   string wallet_name = "default";
   auto createWallet = wallet->add_subcommand("create", localized("Create a new wallet locally"), false);
   createWallet->add_option("-n,--name", wallet_name, localized("The name of the new wallet"), true);
   createWallet->set_callback([&wallet_name] { helper.create_wallet(wallet_name); });

   // open wallet
   auto openWallet = wallet->add_subcommand("open", localized("Open an existing wallet"), false);
   openWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to open"));
   openWallet->set_callback([&wallet_name] { helper.open_existing_wallet(wallet_name); });

   // lock wallet
   auto lockWallet = wallet->add_subcommand("lock", localized("Lock wallet"), false);
   lockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to lock"));
   lockWallet->set_callback([&wallet_name] { helper.lock_wallet(wallet_name); });

   // lock all wallets
   auto locakAllWallets = wallet->add_subcommand("lock_all", localized("Lock all unlocked wallets"), false);
   locakAllWallets->set_callback([] { helper.lock_all(); });

   // unlock wallet
   string wallet_pw;
   auto unlockWallet = wallet->add_subcommand("unlock", localized("Unlock wallet"), false);
   unlockWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to unlock"));
   unlockWallet->add_option("--password", wallet_pw, localized("The password returned by wallet create"));
   unlockWallet->set_callback([&wallet_name, &wallet_pw] { helper.unlock_wallet(wallet_name, wallet_pw); });

   // import keys into wallet
   string wallet_key;
   auto importWallet = wallet->add_subcommand("import", localized("Import private key into wallet"), false);
   importWallet->add_option("-n,--name", wallet_name, localized("The name of the wallet to import key into"));
   importWallet->add_option("key", wallet_key, localized("Private key in WIF format to import"))->required();
   importWallet->set_callback([&wallet_name, &wallet_key] { helper.import_private_key(wallet_name, wallet_key); });

   // list wallets
   auto listWallet = wallet->add_subcommand("list", localized("List opened wallets, * = unlocked"), false);
   listWallet->set_callback([] { helper.list_opened_wallet(); });

   // list keys
   auto listKeys = wallet->add_subcommand("keys", localized("List of private keys from all unlocked wallets in wif format."), false);
   listKeys->set_callback([] { helper.list_private_keys_from_opened_wallets(); });

   // Benchmark subcommand
   auto benchmark = app.add_subcommand( "benchmark", localized("Configure and execute benchmarks"), false );
   benchmark->require_subcommand();
   auto benchmark_setup = benchmark->add_subcommand( "setup", localized("Configures initial condition for benchmark") );
   uint64_t number_of_accounts = 2;
   benchmark_setup->add_option("accounts", number_of_accounts, localized("the number of accounts in transfer among"))->required();
   string c_account;
   benchmark_setup->add_option("creator", c_account, localized("The creator account for benchmark accounts"))->required();
   string owner_key;
   string active_key;
   benchmark_setup->add_option("owner", owner_key, localized("The owner key to use for account creation"))->required();
   benchmark_setup->add_option("active", active_key, localized("The active key to use for account creation"))->required();
   add_standard_transaction_options(benchmark_setup);
   benchmark_setup->set_callback([&]{ helper.configure_benchmarks(number_of_accounts,
                                                                       c_account,
                                                                       owner_key,
                                                                       active_key); });

   auto benchmark_transfer = benchmark->add_subcommand( "transfer", localized("executes random transfers among accounts") );
   uint64_t number_of_transfers = 0;
   bool loop = false;
   benchmark_transfer->add_option("accounts", number_of_accounts, localized("the number of accounts in transfer among"))->required();
   benchmark_transfer->add_option("count", number_of_transfers, localized("the number of transfers to execute"))->required();
   benchmark_transfer->add_option("loop", loop, localized("whether or not to loop for ever"));
   add_standard_transaction_options(benchmark_transfer);
   benchmark_transfer->set_callback([&]{ helper.execute_random_transactions(number_of_accounts,
                                                                                 number_of_transfers,
                                                                                 loop,
                                                                                 memo); });

   // Push subcommand
   auto push = app.add_subcommand("push", localized("Push arbitrary transactions to the blockchain"), false);
   push->require_subcommand();

   // push action
   string contract;
   string action;
   string data;
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
   actionsSubcommand->set_callback([&] { helper.push_transaction_with_single_action(contract,
                                                                                         action,
                                                                                         data,
                                                                                         permissions,
                                                                                         skip_sign,
                                                                                         tx_force_unique);

   });

   // push transaction
   string trxJson;
   auto trxSubcommand = push->add_subcommand("transaction", localized("Push an arbitrary JSON transaction"));
   trxSubcommand->add_option("transaction", trxJson, localized("The JSON of the transaction to push"))->required();
   trxSubcommand->set_callback([&] { helper.push_JSON_transaction(trxJson); });

   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", localized("Push an array of arbitrary JSON transactions"));
   trxsSubcommand->add_option("transactions", trxsJson, localized("The JSON array of the transactions to push"))->required();
   trxsSubcommand->set_callback([&] { helper.push_JSON_transaction(trxsJson); });

   try {
       app.parse(argc, argv);
   } catch (const CLI::ParseError &e) {
       return app.exit(e);
   } catch (const explained_exception& e) {
      return 1;
   } catch (const fc::exception& e) {
      auto errorString = e.to_detail_string();
      if (errorString.find("Connection refused") != string::npos) {
         if (errorString.find(fc::json::to_string(helper.port_peer())) != string::npos) {
            std::cerr << localized("Failed to connect to eosd at ${ip}:${port}; is eosd running?", ("ip", helper.host_peer())("port", helper.port_peer())) << std::endl;
         } else if (errorString.find(fc::json::to_string(helper.port_wallet())) != string::npos) {
            std::cerr << localized("Failed to connect to eos-walletd at ${ip}:${port}; is eos-walletd running?", ("ip", helper.host_wallet())("port", helper.port_wallet())) << std::endl;
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
