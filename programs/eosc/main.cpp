#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/chain/config.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/algorithm/string/split.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <fc/io/fstream.hpp>

#include "CLI11.hpp"

using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;

string program = "eosc";
string host = "localhost";
uint32_t port = 8888;

const string chain_func_base = "/v1/chain";
const string get_info_func = chain_func_base + "/get_info";
const string push_txn_func = chain_func_base + "/push_transaction";
const string push_txns_func = chain_func_base + "/push_transactions";
const string json_to_bin_func = chain_func_base + "/abi_json_to_bin";
const string get_block_func = chain_func_base + "/get_block";
const string get_account_func = chain_func_base + "/get_account";

const string account_history_func_base = "/v1/account_history";
const string get_transaction_func = account_history_func_base + "/get_transaction";
const string get_transactions_func = account_history_func_base + "/get_transactions";
const string get_key_accounts_func = account_history_func_base + "/get_key_accounts";
const string get_controlled_accounts_func = account_history_func_base + "/get_controlled_accounts";

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

fc::variant push_transaction( SignedTransaction& trx ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100;
    transaction_set_reference_block(trx, info.head_block_id);
    boost::sort( trx.scope );

    return call( push_txn_func, trx );
}


void create_account(Name creator, Name newaccount, public_key_type owner, public_key_type active) {
      auto owner_auth   = eos::chain::Authority{1, {{owner, 1}}, {}};
      auto active_auth  = eos::chain::Authority{1, {{active, 1}}, {}};
      auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};

      uint64_t deposit = 1;

      SignedTransaction trx;
      trx.scope = sort_names({creator,config::EosContractName});
      transaction_emplace_message(trx, config::EosContractName, vector<types::AccountPermission>{{creator,"active"}}, "newaccount",
                                           types::newaccount{creator, newaccount, owner_auth,
                                                             active_auth, recovery_auth, deposit});
      if (creator == "inita")
      {
         fc::optional<fc::ecc::private_key> private_key = eos::utilities::wif_to_key("5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3");
         if (private_key)
         {
            wlog("public key ${k}",("k", private_key->get_public_key()));
            trx.sign(*private_key, eos::chain::chain_id_type{});
         }
      }

      std::cout << fc::json::to_pretty_string(push_transaction(trx)) << std::endl;
}

int main( int argc, char** argv ) {
   CLI::App app{"Command Line Interface to Eos Daemon"};
   app.require_subcommand();
   app.add_option( "-p,--port", port, "the port where eosd is running", true );

   // Create subcommand
   auto create = app.add_subcommand("create", "Create various items, on and off the blockchain", false);
   create->require_subcommand();

   // create key
   create->add_subcommand("key", "Create a new keypair and print the public and private keys")->set_callback([] {
      auto privateKey = fc::ecc::private_key::generate();
      std::cout << "Private key: " << key_to_wif(privateKey.get_secret()) << "\n";
      std::cout << "Public key:  " << string(public_key_type(privateKey.get_public_key())) << std::endl;
   });

   // create account
   string creator;
   string name;
   string ownerKey;
   string activeKey;
   auto createAccount = create->add_subcommand("account", "Create a new account on the blockchain", false);
   createAccount->add_option("creator", creator, "The name of the account creating the new account")->required();
   createAccount->add_option("name", name, "The name of the new account")->required();
   createAccount->add_option("OwnerKey", ownerKey, "The owner public key for the account")->required();
   createAccount->add_option("ActiveKey", activeKey, "The active public key for the account")->required();
   createAccount->set_callback([&] {
      create_account(creator, name, public_key_type(ownerKey), public_key_type(activeKey));
   });

   // Get subcommand
   auto get = app.add_subcommand("get", "Retrieve various items and information from the blockchain", false);
   get->require_subcommand();

   // get info
   get->add_subcommand("info", "Get current blockchain information")->set_callback([] {
      std::cout << fc::json::to_pretty_string(get_info()) << std::endl;
   });

   // get block
   string blockArg;
   auto getBlock = get->add_subcommand("block", "Retrieve a full block from the blockchain", false);
   getBlock->add_option("block", blockArg, "The number or ID of the block to retrieve")->required();
   getBlock->set_callback([&blockArg] {
      auto arg = fc::mutable_variant_object("block_num_or_id", blockArg);
      std::cout << fc::json::to_pretty_string(call(get_block_func, arg)) << std::endl;
   });

   // get account
   string accountName;
   auto getAccount = get->add_subcommand("account", "Retrieve an account from the blockchain", false);
   getAccount->add_option("name", accountName, "The name of the account to retrieve")->required();
   getAccount->set_callback([&] {
      std::cout << fc::json::to_pretty_string(call(get_account_func,
                                                   fc::mutable_variant_object("name", accountName)))
                << std::endl;
   });

   // get accounts
   string publicKey;
   auto getAccounts = get->add_subcommand("accounts", "Retrieve accounts associated with a public key", false);
   getAccounts->add_option("public_key", publicKey, "The public key to retrieve accounts for")->required();
   getAccounts->set_callback([&] {
      auto arg = fc::mutable_variant_object( "public_key", publicKey);
      std::cout << fc::json::to_pretty_string(call(get_key_accounts_func, arg)) << std::endl;
   });

   // get servants
   string controllingAccount;
   auto getServants = get->add_subcommand("servants", "Retrieve accounts which are servants of a given account ", false);
   getServants->add_option("account", controllingAccount, "The name of the controlling account")->required();
   getServants->set_callback([&] {
      auto arg = fc::mutable_variant_object( "accountName", controllingAccount);
      std::cout << fc::json::to_pretty_string(call(get_controlled_accounts_func, arg)) << std::endl;
   });

   // get transaction
   string transactionId;
   auto getTransaction = get->add_subcommand("transaction", "Retrieve a transaction from the blockchain", false);
   getTransaction->add_option("id", transactionId, "ID of the transaction to retrieve")->required();
   getTransaction->set_callback([&] {
      auto arg= fc::mutable_variant_object( "transaction_id", transactionId);
      std::cout << fc::json::to_pretty_string(call(get_transaction_func, arg)) << std::endl;
   });

   // Contract subcommand
   string account;
   string wastPath;
   string abiPath;
   auto contractSubcommand = app.add_subcommand("contract", "Create or update the contract on an account");
   contractSubcommand->add_option("account", account, "The account to publish a contract for")->required();
   contractSubcommand->add_option("wast-file", wastPath, "The file containing the contract WAST")->required()
         ->check(CLI::ExistingFile);
   auto abi = contractSubcommand->add_option("abi-file,-a,--abi", abiPath, "The ABI for the contract")
              ->check(CLI::ExistingFile);
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
      std::cout << fc::json::to_pretty_string(push_transaction(trx)) << std::endl;
   });

   // Transfer subcommand
   string sender;
   string recipient;
   uint64_t amount;
   string memo;
   auto transfer = app.add_subcommand("transfer", "Transfer EOS from account to account", false);
   transfer->add_option("sender", sender, "The account sending EOS")->required();
   transfer->add_option("recipient", recipient, "The account receiving EOS")->required();
   transfer->add_option("amount", amount, "The amount of EOS to send")->required();
   transfer->add_option("memo", amount, "The memo for the transfer");
   transfer->set_callback([&] {
      SignedTransaction trx;
      trx.scope = sort_names({sender,recipient});
      transaction_emplace_message(trx, config::EosContractName,
                                           vector<types::AccountPermission>{{sender,"active"}},
                                           "transfer", types::transfer{sender, recipient, amount, memo});
      auto info = get_info();
      trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100;
      transaction_set_reference_block(trx, info.head_block_id);

      std::cout << fc::json::to_pretty_string( call( push_txn_func, trx )) << std::endl;
   });

   // Benchmark subcommand
   auto benchmark = app.add_subcommand( "benchmark", "Configure and execute benchmarks", false );
   auto benchmark_setup = benchmark->add_subcommand( "setup", "Configures initial condition for benchmark" );
   uint64_t number_of_accounts = 2;
   benchmark_setup->add_option("accounts", number_of_accounts, "the number of accounts in transfer among")->required();
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

        trx.expiration = info.head_block_time + 100; 
        transaction_set_reference_block(trx, info.head_block_id);
        batch.emplace_back(trx);
      }
      auto result = call( push_txns_func, batch );
      std::cout << fc::json::to_pretty_string(result) << std::endl;
   });

   auto benchmark_transfer = benchmark->add_subcommand( "transfer", "executes random transfers among accounts" );
   uint64_t number_of_transfers = 0;
   bool loop = false;
   benchmark_transfer->add_option("accounts", number_of_accounts, "the number of accounts in transfer among")->required();
   benchmark_transfer->add_option("count", number_of_transfers, "the number of transfers to execute")->required();
   benchmark_transfer->add_option("loop", loop, "whether or not to loop for ever");
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
         trx.expiration = info.head_block_time + 100; 
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
            trx.expiration = info.head_block_time + 100; 
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
   auto push = app.add_subcommand("push", "Push arbitrary data to the blockchain", false);
   push->require_subcommand();

   // push message
   vector<string> permissions;
   string contract;
   string action;
   string data;
   vector<string> scopes;
   auto messageSubcommand = push->add_subcommand("message", "Push a transaction with a single message");
   messageSubcommand->fallthrough(false);
   messageSubcommand->add_option("contract", contract,
                                 "The account providing the contract to execute", true)->required();
   messageSubcommand->add_option("action", action, "The action to execute on the contract", true)
         ->required();
   messageSubcommand->add_option("data", data, "The arguments to the contract")->required();
   messageSubcommand->add_option("-p,--permission", permissions,
                                 "An account and permission level to authorize, as in 'account@permission'");
   messageSubcommand->add_option("-s,--scope", scopes, "An account in scope for this operation", true);
   messageSubcommand->set_callback([&] {
      ilog("Converting argument to binary...");
      auto arg= fc::mutable_variant_object
                ("code", contract)
                ("action", action)
                ("args", fc::json::from_string(data));
      auto result = call(json_to_bin_func, arg);

      auto fixedPermissions = permissions | boost::adaptors::transformed([](const string& p) {
         vector<string> pieces;
         boost::split(pieces, p, boost::is_any_of("@"));
         FC_ASSERT(pieces.size() == 2, "Invalid permission: ${p}", ("p", p));
         return types::AccountPermission(pieces[0], pieces[1]);
      });

      SignedTransaction trx;
      transaction_emplace_serialized_message(trx, contract, action,
                                                      vector<types::AccountPermission>{fixedPermissions.front(),
                                                                                       fixedPermissions.back()},
                                                      result.get_object()["binargs"].as<Bytes>());
      trx.scope.assign(scopes.begin(), scopes.end());
      ilog("Transaction result:\n${r}", ("r", fc::json::to_pretty_string(push_transaction(trx))));
   });

   // push transaction
   string trxJson;
   auto trxSubcommand = push->add_subcommand("transaction", "Push an arbitrary JSON transaction");
   trxSubcommand->add_option("transaction", trxJson, "The JSON of the transaction to push")->required();
   trxSubcommand->set_callback([&] {
      auto trx_result = call(push_txn_func, fc::json::from_string(trxJson));
      std::cout << fc::json::to_pretty_string(trx_result) << std::endl;
   });


   string trxsJson;
   auto trxsSubcommand = push->add_subcommand("transactions", "Push an array of arbitrary JSON transactions");
   trxsSubcommand->add_option("transactions", trxsJson, "The JSON array of the transactions to push")->required();
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
      if (errorString.find("Connection refused") != string::npos)
         elog("Failed to connect to eosd at ${ip}:${port}; is eosd running?", ("ip", host)("port", port));
      else
         elog("Failed with error: ${e}", ("e", e.to_detail_string()));
      return 1;
   }

   return 0;
}
