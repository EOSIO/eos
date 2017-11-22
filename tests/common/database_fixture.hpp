/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/chain/exceptions.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>

#include <eos/native_contract/native_contract_chain_initializer.hpp>
#include <eos/native_contract/native_contract_chain_administrator.hpp>

#include <eos/utilities/tempdir.hpp>

#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <boost/range/algorithm/sort.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

#include <iostream>

using namespace eosio::chain;

extern uint32_t EOS_TESTING_GENESIS_TIMESTAMP;

#define HAVE_DATABASE_FIXTURE
#include "testing_macros.hpp"

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

namespace eosio { namespace chain {
FC_DECLARE_EXCEPTION(testing_exception, 6000000, "test framework exception")
FC_DECLARE_DERIVED_EXCEPTION(missing_key_exception, eosio::chain::testing_exception, 6010000, "key could not be found")

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
   flat_set<public_key_type> available_keys() const;

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

   testing_blockchain(chainbase::database& db, fork_database& fork_db, block_log& blocklog,
                      chain_initializer_interface& initializer, testing_fixture& fixture,
                      uint32_t transaction_execution_time_msec,
                      uint32_t received_block_execution_time_msec,
                      uint32_t create_block_execution_time_msec,
                      const chain_controller::txn_msg_limits& rate_limits = chain_controller::txn_msg_limits());

   /**
    * @brief Publish the provided contract to the blockchain, owned by owner
    * @param owner The account to publish the contract under
    * @param contract_wast The WAST of the contract
    */
   void set_contract(account_name owner, const char* contract_wast);

   /**
    * @brief Produce new blocks, adding them to the blockchain, optionally following a gap of missed blocks
    * @param count Number of blocks to produce
    * @param blocks_to_miss Number of block intervals to miss a production before producing the next block
    *
    * Creates and adds @ref count new blocks to the blockchain, after going @ref blocks_to_miss intervals without
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
   asset get_liquid_balance(const types::account_name& account);
   /// @brief Get the staked balance belonging to the named account
   asset get_staked_balance(const types::account_name& account);
   /// @brief Get the unstaking balance belonging to the named account
   asset get_unstaking_balance(const types::account_name& account);

   /// @brief Get the set of producers approved by the named account
   std::set<account_name> get_approved_producers(const account_name& account);
   /// @brief Get the specified block producer's signing key
   public_key get_block_signing_key(const account_name& producerName);

   /// @brief Attempt to sign the provided transaction using the keys available to the testing_fixture
   void sign_transaction(signed_transaction& trx) const;

   /// @brief Override push_transaction to apply testing policies
   /// If transactions are being held for review, transaction will be held after testing policies are applied
   fc::optional<processed_transaction> push_transaction(signed_transaction trx, uint32_t skip_flags = 0);
   /// @brief Review and optionally push last held transaction
   /// @tparam F A callable with signature `bool f(SignedTransaction&, uint32_t&)`
   /// @param reviewer Callable which inspects and potentially alters the held transaction and skip flags, and returns
   /// whether it should be pushed or not
   template<typename F>
   fc::optional<processed_transaction> review_transaction(F&& reviewer) {
      if (reviewer(review_storage.first, review_storage.second))
         return chain_controller::push_transaction(review_storage.first, review_storage.second);
      return {};
   }

   /// @brief Set whether testing_blockchain::push_transaction checks signatures by default
   /// @param skip_sigs If true, push_transaction will skip signature checks; otherwise, no changes will be made
   void set_skip_transaction_signature_checking(bool skip_sigs) {
      skip_trx_sigs = skip_sigs;
   }
   /// @brief Set whether testing_blockchain::push_transaction attempts to sign transactions or not
   void set_auto_sign_transactions(bool auto_sign) {
      auto_sign_trxs = auto_sign;
   }
   /// @brief Set whether testing_blockchain::push_transaction holds transactions for review or not
   void set_hold_transactions_for_review(bool hold_trxs) {
      hold_for_review = hold_trxs;
   }

   static std::vector<uint8_t> assemble_wast(const std::string& wast);

protected:
   chainbase::database& db;
   testing_fixture& fixture;
   std::pair<signed_transaction, uint32_t> review_storage;
   bool skip_trx_sigs = true;
   bool auto_sign_trxs = false;
   bool hold_for_review = false;
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

} }
