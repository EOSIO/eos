/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <fc/crypto/digest.hpp>

/**
 * @file Contains support macros for the testcase helper macros. These macros are implementation details, and thus
 * should not be used directly. Use their frontends instead.
 */

#define MKCHAIN1(name) \
   chainbase::database name ## _db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir() / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_blockchain name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this); \
   BOOST_TEST_CHECKPOINT("Created blockchain " << #name);
#define MKCHAIN2(name, id) \
   chainbase::database name ## _db(get_temp_dir(#id), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir(#id) / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_blockchain name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this); \
   BOOST_TEST_CHECKPOINT("Created blockchain " << #name);
#define MKCHAIN5(name, transaction_execution_time_sec, receive_block_execution_time_sec, create_block_execution_time_sec, rate_limits) \
   chainbase::database name ## _db(get_temp_dir(), chainbase::database::read_write, TEST_DB_SIZE); \
   block_log name ## _log(get_temp_dir() / "blocklog"); \
   fork_database name ## _fdb; \
   native_contract::native_contract_chain_initializer name ## _initializer(genesis_state()); \
   testing_blockchain name(name ## _db, name ## _fdb, name ## _log, name ## _initializer, *this, transaction_execution_time_sec, receive_block_execution_time_sec, create_block_execution_time_sec, rate_limits); \
   BOOST_TEST_CHECKPOINT("Created blockchain " << #name);
#define MKCHAINS_MACRO(x, y, name) Make_Blockchain(name)

#define MKNET1(name) testing_network name; BOOST_TEST_CHECKPOINT("Created testnet " << #name);
#define MKNET2_MACRO(x, name, chain) name.connect_blockchain(chain);
#define MKNET2(name, ...) MKNET1(name) BOOST_PP_SEQ_FOR_EACH(MKNET2_MACRO, name, __VA_ARGS__)

inline std::vector<name> sort_names( std::vector<name>&& names ) {
   std::sort( names.begin(), names.end() );
   auto itr = std::unique( names.begin(), names.end() );
   names.erase( itr, names.end() );
   return names;
}

#define Complex_Authority_macro_Key(r, data, key_bubble) \
   data.keys.emplace_back(BOOST_PP_CAT(BOOST_PP_TUPLE_ELEM(2, 0, key_bubble), _public_key), \
                          BOOST_PP_TUPLE_ELEM(2, 1, key_bubble));
#define Complex_Authority_macro_Account(r, data, account_bubble) \
   data.accounts.emplace_back(types::account_permission{BOOST_PP_TUPLE_ELEM(3, 0, account_bubble), \
                                                       BOOST_PP_TUPLE_ELEM(3, 1, account_bubble)}, \
                              BOOST_PP_TUPLE_ELEM(3, 2, account_bubble));

#define MKACCT_IMPL(chain, name, creator, active, owner, recovery, deposit) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names({ #creator, config::eos_contract_name }); \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#creator, "active"}}, \
                         "newaccount", types::newaccount{#creator, #name, owner, active, recovery, deposit}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Created account " << #name); \
   }
#define MKACCT2(chain, name) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, inita, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), \
               Account_Authority(inita), asset(100))
#define MKACCT3(chain, name, creator) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, creator, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), \
               Account_Authority(creator), asset(100))
#define MKACCT4(chain, name, creator, deposit) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, creator, Key_Authority(name ## _public_key), Key_Authority(name ## _public_key), \
               Account_Authority(creator), deposit)
#define MKACCT5(chain, name, creator, deposit, owner) \
   Make_Key(name) \
   MKACCT_IMPL(chain, name, creator, owner, Key_Authority(name ## _public_key), Account_Authority(creator), deposit)
#define MKACCT6(chain, name, creator, deposit, owner, active) \
   MKACCT_IMPL(chain, name, creator, owner, active, Account_Authority(creator), deposit)
#define MKACCT7(chain, name, creator, deposit, owner, active, recovery) \
   MKACCT_IMPL(chain, name, creator, owner, active, recovery, deposit)

#define SETCODE3(chain, acct, wast) \
   { \
      auto wasm = eosio::chain::wast_to_wasm(wast); \
      types::setcode handler; \
      handler.account = #acct; \
      handler.code.assign(wasm.begin(), wasm.end()); \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names({config::eos_contract_name, #acct}); \
      transaction_emplace_message(trx, config::eos_contract_name, vector<types::account_permission>{{#acct,"active"}}, \
                                  "setcode", handler); \
   }

#define SETAUTH5(chain, account, authname, parentname, auth) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = {#account}; \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#account,"active"}}, \
                         "updateauth", types::updateauth{#account, authname, parentname, auth}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Set " << #account << "'s " << authname << " authority."); \
   }

#define DELAUTH3(chain, account, authname) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = {#account}; \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#account,"active"}}, \
                         "deleteauth", types::deleteauth{#account, authname}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Deleted " << #account << "'s " << authname << " authority."); \
   }

#define LINKAUTH5(chain, account, authname, codeacct, messagetype) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = {#account}; \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#account,"active"}}, \
                         "linkauth", types::linkauth{#account, #codeacct, messagetype, authname}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Link " << #codeacct << "::" << messagetype << " to " << #account \
                            << "'s " << authname << " authority."); \
   }
#define LINKAUTH4(chain, account, authname, codeacct) LINKAUTH5(chain, account, authname, codeacct, "")

#define UNLINKAUTH4(chain, account, codeacct, messagetype) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = {#account}; \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#account,"active"}}, \
                         "unlinkauth", types::unlinkauth{#account, #codeacct, messagetype}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Unlink " << #codeacct << "::" << messagetype << " from " << #account); \
   }
#define LINKAUTH3(chain, account, codeacct) LINKAUTH5(chain, account, codeacct, "")

#define XFER5(chain, sender, recipient, Amount, memo) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names({#sender,#recipient}); \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{ {#sender,"active"} }, \
                         "transfer", types::transfer{#sender, #recipient, Amount.amount, memo}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Transferred " << Amount << " from " << #sender << " to " << #recipient); \
   }
#define XFER4(chain, sender, recipient, amount) XFER5(chain, sender, recipient, amount, "")

#define STAKE4(chain, sender, recipient, amount) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names( { #sender, #recipient, config::eos_contract_name } ); \
      transaction_emplace_message(trx, config::eos_contract_name, vector<types::account_permission>{{#sender, "active"}}, \
                        "lock", types::lock{#sender, #recipient, amount}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Staked " << amount << " to " << #recipient); \
   }
#define STAKE3(chain, account, amount) STAKE4(chain, account, account, amount)

#define BEGIN_UNSTAKE3(chain, account, amount) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names( { config::eos_contract_name } ); \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#account, "active"}}, \
                         "unlock", types::unlock{#account, amount}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Begin unstake " << amount << " to " << #account); \
   }

#define FINISH_UNSTAKE3(chain, account, amount) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names( { config::eos_contract_name, #account } ); \
      transaction_emplace_message(trx, config::eos_contract_name, vector<types::account_permission>{{#account, "active"}}, \
                         "claim", types::claim{#account, amount}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Finish unstake " << amount << " to " << #account); \
   }

#define MKPDCR4(chain, owner, key, cfg) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names( {#owner, config::eos_contract_name} ); \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{{#owner, "active"}}, \
                         "setproducer", types::setproducer{#owner, key, cfg}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Create producer " << #owner); \
   }
#define MKPDCR3(chain, owner, key) MKPDCR4(chain, owner, key, blockchain_configuration{});
#define MKPDCR2(chain, owner) \
   Make_Key(owner ## _producer); \
   MKPDCR4(chain, owner, owner ## _producer_public_key, blockchain_configuration{});

#define APPDCR4(chain, voter, producer, approved) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names( {#voter, config::eos_contract_name} ); \
      transaction_emplace_message(trx, config::eos_contract_name,  \
                         vector<types::account_permission>{{#voter, "active"}}, \
                         "okproducer", types::okproducer{#voter, #producer, approved? 1 : 0}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Set producer approval from " << #voter << " for " << #producer << " to " << approved); \
   }

#define UPPDCR4(chain, owner, key, cfg) \
   { \
      eosio::chain::signed_transaction trx; \
      trx.scope = sort_names( {owner, config::eos_contract_name} ); \
      transaction_emplace_message(trx, config::eos_contract_name,  \
                         vector<types::account_permission>{{owner, "active"}}, \
                         "setproducer", types::setproducer{owner, key, cfg}); \
      trx.expiration = chain.head_block_time() + 100; \
      transaction_set_reference_block(trx, chain.head_block_id()); \
      chain.push_transaction(trx); \
      BOOST_TEST_CHECKPOINT("Update producer " << owner); \
   }
#define UPPDCR3(chain, owner, key) UPPDCR4(chain, owner, key, chain.get_producer(owner).configuration)
