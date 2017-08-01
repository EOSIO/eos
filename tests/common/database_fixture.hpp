/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/exceptions.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/native_contract_chain_administrator.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/range/algorithm/sort.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

#include <iostream>

using namespace eos::chain;

extern uint32_t EOS_TESTING_GENESIS_TIMESTAMP;

#define TEST_DB_SIZE (1024*1024*1000)

#define EOS_REQUIRE_THROW( expr, exc_type )          \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "EOS_REQUIRE_THROW begin "             \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "EOS_REQUIRE_THROW end "               \
         << req_throw_info << std::endl;                  \
}

#define EOS_CHECK_THROW( expr, exc_type )            \
{                                                         \
   std::string req_throw_info = fc::json::to_string(      \
      fc::mutable_variant_object()                        \
      ("source_file", __FILE__)                           \
      ("source_lineno", __LINE__)                         \
      ("expr", #expr)                                     \
      ("exc_type", #exc_type)                             \
      );                                                  \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "EOS_CHECK_THROW begin "               \
         << req_throw_info << std::endl;                  \
   BOOST_CHECK_THROW( expr, exc_type );                   \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "EOS_CHECK_THROW end "                 \
         << req_throw_info << std::endl;                  \
}

namespace eos { namespace chain {
FC_DECLARE_EXCEPTION(testing_exception, 6000000, "test framework exception")
FC_DECLARE_DERIVED_EXCEPTION(missing_key_exception, eos::chain::testing_exception, 6010000, "key could not be found")

/**
 * @brief The testing_fixture class provides various services relevant to testing the blockchain.
 */
class testing_fixture {
public:
   testing_fixture();

   /**
    * @brief Get a temporary directory to store data in during this test
    *
    * @param id Identifier for the temporary directory. All requests for directories with the same ID will receive the
    * same path. If id is empty, a unique directory will be returned.
    *
    * @return Path to the temporary directory
    *
    * This method makes it easy to get temporary directories for the duration of a test. All returned directories will
    * be automatically deleted when the testing_fixture is destroyed.
    *
    * If multiple calls to this function are made with the same id, the same path will be returned. Multiple calls with
    * distinct ids will not return the same path. If called with an empty id, a unique path will be returned which will
    * not be returned from any subsequent call.
    */
   fc::path get_temp_dir(std::string id = std::string());

   const native_contract::genesis_state_type& genesis_state() const;
   native_contract::genesis_state_type& genesis_state();

   void store_private_key(const private_key_type& key);
   private_key_type get_private_key(const public_key_type& public_key) const;

protected:
   std::vector<fc::temp_directory> anonymous_temp_dirs;
   std::map<std::string, fc::temp_directory> named_temp_dirs;
   std::map<public_key_type, private_key_type> key_ring;
   native_contract::genesis_state_type default_genesis_state;
};

/**
 * @brief The testing_blockchain class wraps chain_controller and eliminates some of the boilerplate for common 
 * operations on the blockchain during testing.
 *
 * testing_blockchains have an optional ID, which is passed to the constructor. If two testing_blockchains are created 
 * with the same ID, they will have the same data directory. If no ID, or an empty ID, is provided, the database will 
 * have a unique data directory which no subsequent testing_blockchain will be assigned.
 *
 * testing_blockchain helps with producing blocks, or missing blocks, via the @ref produce_blocks and @ref miss_blocks
 * methods. To produce N blocks, simply call produce_blocks(N); to miss N blocks, call miss_blocks(N). Note that missing
 * blocks has no effect on the blockchain until the next block, following the missed blocks, is produced.
 */
class testing_blockchain : public chain_controller {
public:
    testing_blockchain(chainbase::database& db, fork_database& fork_db, block_log& blocklog,
                     chain_initializer_interface& initializer, testing_fixture& fixture);

   /**
    * @brief Produce new blocks, adding them to the blockchain, optionally following a gap of missed blocks
    * @param count Number of blocks to produce
    * @param blocks_to_miss Number of block intervals to miss a production before producing the next block
    *
    * Creates and adds  @ref count new blocks to the blockchain, after going @ref blocks_to_miss intervals without
    * producing a block.
    */
   void produce_blocks(uint32_t count = 1, uint32_t blocks_to_miss = 0);

   /**
    * @brief Sync this blockchain with other
    * @param other Blockchain to sync with
    *
    * To sync the blockchains, all blocks from one blockchain which are unknown to the other are pushed to the other, 
    * then the same thing in reverse. Whichever blockchain has more blocks will have its blocks sent to the other 
    * first.
    *
    * Blocks not on the main fork are ignored.
    */
   void sync_with(testing_blockchain& other);

   /// @brief Get the liquid balance belonging to the named account
   Asset get_liquid_balance(const types::AccountName& account);
   /// @brief Get the staked balance belonging to the named account
   Asset get_staked_balance(const types::AccountName& account);
   /// @brief Get the unstaking balance belonging to the named account
   Asset get_unstaking_balance(const types::AccountName& account);

   /// @brief Get the set of producers approved by the named account
   std::set<AccountName> get_approved_producers(const AccountName& account);
   /// @brief Get the specified block producer's signing key
   PublicKey get_block_signing_key(const AccountName& producerName);

protected:
   testing_fixture& fixture;
};

using boost::signals2::scoped_connection;

/**
 * @brief The testing_network class provides a simplistic virtual P2P network connecting testing_blockchains together.
 *
 * A testing_blockchain may be connected to zero or more testing_networks at any given time. When a new 
 * testing_blockchain joins the network, it will be synced with all other blockchains already in the network (blocks 
 * known only to the new chain will be pushed to the prior network members and vice versa, ignoring blocks not on the 
 * main fork). After this, whenever any blockchain in the network gets a new block, that block will be pushed to all 
 * other blockchains in the network as well.
 */
class testing_network {
public:
   /**
    * @brief Add a new database to the network
    * @param new_blockchain The blockchain to add
    */
   void connect_blockchain(testing_blockchain& new_database);
   /**
    * @brief Remove a database from the network
    * @param leaving_blockchain The database to remove
    */
   void disconnect_database(testing_blockchain& leaving_blockchain);
   /**
    * @brief Disconnect all blockchains from the network
    */
   void disconnect_all();

   /**
    * @brief Send a block to all blockchains in this network
    * @param block The block to send
    */
   void propagate_block(const signed_block& block, const testing_blockchain& skip_db);

protected:
   std::map<testing_blockchain*, scoped_connection> blockchains;
};

/// Some helpful macros to reduce boilerplate when making testcases
/// @{
#include "macro_support.hpp"

/**
 * @brief Create/Open a testing_blockchain, optionally with an ID
 *
 * Creates and opens a testing_blockchain with the first argument as its name, and, if present, the second argument as
 * its ID. The ID should be provided without quotes.
 *
 * Example:
 * @code{.cpp}
 * // Create testing_blockchain chain1
 * Make_Blockchain(chain1)
 *
 * // The above creates the following objects:
 * chainbase::database chain1_db;
 * block_log chain1_log;
 * fork_database chain1_fdb;
 * native_contract::native_contract_chain_initializer chain1_initializer;
 * testing_blockchain chain1;
 * @endcode
 */
#define Make_Blockchain(...) BOOST_PP_OVERLOAD(MKCHAIN, __VA_ARGS__)(__VA_ARGS__)
/**
 * @brief Similar to @ref Make_Blockchain, but works with several chains at once
 *
 * Creates and opens several testing_blockchains
 *
 * Example:
 * @code{.cpp}
 * // Create testing_blockchains chain1 and chain2, with chain2 having ID "id2"
 * Make_Blockchains((chain1)(chain2, id2))
 * @endcode
 */
#define Make_Blockchains(...) BOOST_PP_SEQ_FOR_EACH(MKCHAINS_MACRO, _, __VA_ARGS__)

/**
 * @brief Make_Network is a shorthand way to create a testing_network and connect some testing_blockchains to it.
 *
 * Example usage:
 * @code{.cpp}
 * // Create and open testing_blockchains named alice, bob, and charlie
 * MKDBS((alice)(bob)(charlie))
 * // Create a testing_network named net and connect alice and bob to it
 * Make_Network(net, (alice)(bob))
 *
 * // Connect charlie to net, then disconnect alice
 * net.connect_blockchain(charlie);
 * net.disconnect_blockchain(alice);
 *
 * // Create a testing_network named net2 with no blockchains connected
 * Make_Network(net2)
 * @endcode
 */
#define Make_Network(...) BOOST_PP_OVERLOAD(MKNET, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Make_Key is a shorthand way to create a keypair
 *
 * @code{.cpp}
 * // This line:
 * Make_Key(a_key)
 * // ...defines these objects:
 * private_key_type a_key_private_key;
 * PublicKey a_key_public_key;
 * // The private key is generated off of the sha256 hash of "a_key_private_key", so it should be unique from all
 * // other keys created with MKKEY in the same scope.
 * @endcode
 */
#define Make_Key(name) auto name ## _private_key = private_key_type::regenerate(fc::digest(#name "_private_key")); \
   store_private_key(name ## _private_key); \
   PublicKey name ## _public_key = name ## _private_key.get_public_key(); \
   BOOST_TEST_CHECKPOINT("Created key " #name "_public_key");

/**
 * @brief Key_Authority is a shorthand way to create an inline Authority based on a key
 *
 * Invoke Key_Authority passing the name of a public key in the current scope, and AUTHK will resolve inline to an authority
 * which can be satisfied by a signature generated by the corresponding private key.
 */
#define Key_Authority(pubkey) (Authority{1, {{pubkey, 1}}, {}})
/**
 * @brief Account_Authority is a shorthand way to create an inline Authority based on an account
 *
 * Invoke Account_Authority passing the name of an account, and AUTHA will resolve inline to an authority which can be satisfied by
 * the provided account's active authority.
 */
#define Account_Authority(account) (Authority{1, {}, {{{#account, "active"}, 1}}})

/**
 * @brief Make_Account is a shorthand way to create an account
 *
 * Use Make_Account to create an account, including keys. The changes will be applied via a transaction applied to the
 * provided blockchain object. The changes will not be incorporated into a block; they will be left in the pending 
 * state.
 *
 * Unless overridden, new accounts are created with a balance of Asset(100)
 *
 * Example:
 * @code{.cpp}
 * Make_Account(chain, joe)
 * // ... creates these objects:
 * private_key_type joe_private_key;
 * PublicKey joe_public_key;
 * // ...and also registers the account joe with owner and active authorities satisfied by these keys, created by
 * // init0, with init0's active authority as joe's recovery authority, and initially endowed with Asset(100)
 * @endcode
 *
 * You may specify a third argument for the creating account:
 * @code{.cpp}
 * // Same as MKACCT(chain, joe) except that sam will create joe's account instead of init0
 * Make_Account(chain, joe, sam)
 * @endcode
 *
 * You may specify a fourth argument for the amount to transfer in account creation:
 * @code{.cpp}
 * // Same as MKACCT(chain, joe, sam) except that sam will send joe ASSET(100) during creation
 * Make_Account(chain, joe, sam, Asset(100))
 * @endcode
 *
 * You may specify a fifth argument, which will be used as the owner authority (must be an Authority, NOT a key!).
 *
 * You may specify a sixth argument, which will be used as the active authority. If six or more arguments are provided,
 * the default keypair will NOT be created or put into scope.
 *
 * You may specify a seventh argument, which will be used as the recovery authority.
 */
#define Make_Account(...) BOOST_PP_OVERLOAD(MKACCT, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to transfer funds
 *
 * Use Transfer_Asset to send funds from one account to another:
 * @code{.cpp}
 * // Send 10 EOS from alice to bob
 * Transfer_Asset(chain, alice, bob, Asset(10));
 *
 * // Send 10 EOS from alice to bob with memo "Thanks for all the fish!"
 * Transfer_Asset(chain, alice, bob, Asset(10), "Thanks for all the fish!");
 * @endcode
 *
 * The changes will be applied via a transaction applied to the provided blockchain object. The changes will not be
 * incorporated into a block; they will be left in the pending state.
 */
#define Transfer_Asset(...) BOOST_PP_OVERLOAD(XFER, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to convert liquid funds to staked funds
 *
 * Use Stake_Asset to stake liquid funds:
 * @code{.cpp}
 * // Convert 10 of bob's EOS from liquid to staked
 * Stake_Asset(chain, bob, Asset(10).amount);
 *
 * // Stake and transfer 10 EOS from alice to bob (alice pays liquid EOS and bob receives stake)
 * Stake_Asset(chain, alice, bob, Asset(10).amount);
 * @endcode
 */
#define Stake_Asset(...) BOOST_PP_OVERLOAD(STAKE, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to begin conversion from staked funds to liquid funds
 *
 * Use Unstake_Asset to begin unstaking funds:
 * @code{.cpp}
 * // Begin unstaking 10 of bob's EOS
 * Unstake_Asset(chain, bob, Asset(10).amount);
 * @endcode
 *
 * This can also be used to cancel an unstaking in progress, by passing Asset(0) as the amount.
 */
#define Begin_Unstake_Asset(...) BOOST_PP_OVERLOAD(BEGIN_UNSTAKE, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to claim unstaked EOS as liquid
 *
 * Use Finish_Unstake_Asset to liquidate unstaked funds:
 * @code{.cpp}
 * // Reclaim as liquid 10 of bob's unstaked EOS
 * Unstake_Asset(chain, bob, Asset(10).amount);
 * @endcode
 */
#define Finish_Unstake_Asset(...) BOOST_PP_OVERLOAD(FINISH_UNSTAKE, __VA_ARGS__)(__VA_ARGS__)


/**
 * @brief Shorthand way to set voting proxy
 *
 * Use Set_Proxy to set what account a stakeholding account proxies its voting power to
 * @code{.cpp}
 * // Proxy sam's votes to bob
 * Set_Proxy(chain, sam, bob);
 *
 * // Unproxy sam's votes
 * Set_Proxy(chain, sam, sam);
 * @endcode
 */
#define Set_Proxy(chain, stakeholder, proxy) \
{ \
   eos::chain::SignedTransaction trx; \
   if (std::string(#stakeholder) != std::string(#proxy)) \
      trx.emplaceMessage(config::EosContractName, \
                         vector<types::AccountPermission>{ {#stakeholder,"active"} }, "setproxy", types::setproxy{#stakeholder, #proxy}); \
   else \
      trx.emplaceMessage(config::EosContractName, \
                         vector<types::AccountPermission>{ {#stakeholder,"active"} }, "setproxy", types::setproxy{#stakeholder, #proxy}); \
   trx.expiration = chain.head_block_time() + 100; \
   trx.set_reference_block(chain.head_block_id()); \
   chain.push_transaction(trx); \
}

/**
 * @brief Shorthand way to create a block producer
 *
 * Use Make_Producer to create a block producer:
 * @code{.cpp}
 * // Create a block producer belonging to joe using signing_key as the block signing key and config as the producer's
 * // vote for a @ref BlockchainConfiguration:
 * Make_Producer(chain, joe, signing_key, config);
 *
 * // Create a block producer belonging to joe using signing_key as the block signing key:
 * Make_Producer(chain, joe, signing_key);
 *
 * // Create a block producer belonging to joe, using a new key as the block signing key:
 * Make_Producer(chain, joe);
 * // ... creates the objects:
 * private_key_type joe_producer_private_key;
 * PublicKey joe_producer_public_key;
 * @endcode
 */
#define Make_Producer(...) BOOST_PP_OVERLOAD(MKPDCR, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to set approval of a block producer
 *
 * Use Approve_Producer to change an account's approval of a block producer:
 * @code{.cpp}
 * // Set joe's approval for pete's block producer to Approve
 * Approve_Producer(chain, joe, pete, true);
 * // Set joe's approval for pete's block producer to Disapprove
 * Approve_Producer(chain, joe, pete, false);
 * @endcode
 */
#define Approve_Producer(...) BOOST_PP_OVERLOAD(APPDCR, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to update a block producer
 *
 * @note Unlike with the Make_* macros, the Update_* macros take an expression as the owner/name field, so be sure to
 * wrap names like this in quotes. You may also pass a normal C++ expression to be evaulated here instead. The reason
 * for this discrepancy is that the Make_* macros add identifiers to the current scope based on the owner/name field;
 * moreover, which can't be done with C++ expressions; however, the Update_* macros do not add anything to the scope,
 * and it's more likely that these will be used in a loop or other context where it is inconvenient to know the
 * owner/name at compile time.
 *
 * Use Update_Producer to update a block producer:
 * @code{.cpp}
 * // Update a block producer belonging to joe using signing_key as the new block signing key, and config as the
 * // producer's new vote for a @ref BlockchainConfiguration:
 * Update_Producer(chain, "joe", signing_key, config)
 *
 * // Update a block producer belonging to joe using signing_key as the new block signing key:
 * Update_Producer(chain, "joe", signing_key)
 * @endcode
 */
#define Update_Producer(...) BOOST_PP_OVERLOAD(UPPDCR, __VA_ARGS__)(__VA_ARGS__)
/// @}
} }
