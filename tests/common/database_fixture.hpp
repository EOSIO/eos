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

#include <eos/app/application.hpp>
#include <eos/chain/database.hpp>
#include <eos/chain/producer_object.hpp>
#include <eos/utilities/tempdir.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>

#include <iostream>

using namespace eos::chain;

extern uint32_t EOS_TESTING_GENESIS_TIMESTAMP;

namespace eos { namespace chain {
FC_DECLARE_EXCEPTION(testing_exception, 6000000, "test framework exception")
} }

#define TEST_DB_SIZE (1024*1024*10)

#define PUSH_TX \
   eos::chain::test::_push_transaction

#define PUSH_BLOCK \
   eos::chain::test::_push_block

// See below
#define REQUIRE_OP_VALIDATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   op.validate(); \
   op.field = temp; \
}
#define REQUIRE_OP_EVALUATION_SUCCESS( op, field, value ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = temp; \
   db.push_transaction( trx, ~0 ); \
}

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
      std::cout << "EOS_REQUIRE_THROW begin "        \
         << req_throw_info << std::endl;                  \
   BOOST_REQUIRE_THROW( expr, exc_type );                 \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "EOS_REQUIRE_THROW end "          \
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
      std::cout << "EOS_CHECK_THROW begin "          \
         << req_throw_info << std::endl;                  \
   BOOST_CHECK_THROW( expr, exc_type );                   \
   if( fc::enable_record_assert_trip )                    \
      std::cout << "EOS_CHECK_THROW end "            \
         << req_throw_info << std::endl;                  \
}

#define REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, exc_type ) \
{ \
   const auto temp = op.field; \
   op.field = value; \
   EOS_REQUIRE_THROW( op.validate(), exc_type ); \
   op.field = temp; \
}
#define REQUIRE_OP_VALIDATION_FAILURE( op, field, value ) \
   REQUIRE_OP_VALIDATION_FAILURE_2( op, field, value, fc::exception )

#define REQUIRE_THROW_WITH_VALUE_2(op, field, value, exc_type) \
{ \
   auto bak = op.field; \
   op.field = value; \
   trx.operations.back() = op; \
   op.field = bak; \
   EOS_REQUIRE_THROW(db.push_transaction(trx, ~0), exc_type); \
}

#define REQUIRE_THROW_WITH_VALUE( op, field, value ) \
   REQUIRE_THROW_WITH_VALUE_2( op, field, value, fc::exception )

///This simply resets v back to its default-constructed value. Requires v to have a working assingment operator and
/// default constructor.
#define RESET(v) v = decltype(v)()
///This allows me to build consecutive test cases. It's pretty ugly, but it works well enough for unit tests.
/// i.e. This allows a test on update_account to begin with the database at the end state of create_account.
#define INVOKE(test) ((struct test*)this)->test_method(); trx.clear()

#define PREP_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name));   \
   public_key_type name ## _public_key = name ## _private_key.get_public_key();

#define ACTOR(name) \
   PREP_ACTOR(name) \
   const auto& name = create_account(BOOST_PP_STRINGIZE(name), name ## _public_key); \
   account_id_type name ## _id = name.id; (void)name ## _id;

#define GET_ACTOR(name) \
   fc::ecc::private_key name ## _private_key = generate_private_key(BOOST_PP_STRINGIZE(name)); \
   const account_object& name = get_account(BOOST_PP_STRINGIZE(name)); \
   account_id_type name ## _id = name.id; \
   (void)name ##_id

#define ACTORS_IMPL(r, data, elem) ACTOR(elem)
#define ACTORS(names) BOOST_PP_SEQ_FOR_EACH(ACTORS_IMPL, ~, names)

namespace eos { namespace chain {

struct database_fixture {
   // the reason we use an app is to exercise the indexes of built-in plugins
   eos::app::application app;
   genesis_state_type genesis_state;
   eos::chain::database &db;
   signed_transaction trx;
   fc::ecc::private_key private_key = fc::ecc::private_key::generate();
   fc::ecc::private_key init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(string("null_key")) );
   public_key_type init_account_pub_key;

   optional<fc::temp_directory> data_dir;
   bool skip_key_index_test = false;
   uint32_t anon_acct_count;

   database_fixture();
   ~database_fixture();

   static fc::ecc::private_key generate_private_key(string seed);
   string generate_anon_acct_name();
   static void verify_asset_supplies( const database& db );
   void verify_account_history_plugin_index( )const;
   void open_database();
   signed_block generate_block(uint32_t skip = ~0,
                               const fc::ecc::private_key& key = generate_private_key("null_key"),
                               int miss_blocks = 0);

   /**
    * @brief Generates block_count blocks
    * @param block_count number of blocks to generate
    */
   void generate_blocks(uint32_t block_count);

   /**
    * @brief Generates blocks until the head block time matches or exceeds timestamp
    * @param timestamp target time to generate blocks until
    */
   void generate_blocks(fc::time_point_sec timestamp, bool miss_intermediate_blocks = true, uint32_t skip = ~0);
};

namespace test {
/// set a reasonable expiration time for the transaction
void set_expiration( const database& db, transaction& tx );

bool _push_block( database& db, const signed_block& b, uint32_t skip_flags = 0 );
signed_transaction _push_transaction( database& db, const signed_transaction& tx, uint32_t skip_flags = 0 );
}

class testing_fixture {
public:
   testing_fixture() {
      default_genesis_state.initial_timestamp = fc::time_point_sec(EOS_TESTING_GENESIS_TIMESTAMP);
      default_genesis_state.initial_producer_count = EOS_DEFAULT_MIN_PRODUCER_COUNT;
      for (int i = 0; i < default_genesis_state.initial_producer_count; ++i) {
         auto name = std::string("init") + fc::to_string(i);
         auto private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name));
         public_key_type public_key = private_key.get_public_key();
         default_genesis_state.initial_accounts.emplace_back(name, public_key, public_key);
         key_ring[public_key] = private_key;

         private_key = fc::ecc::private_key::regenerate(fc::sha256::hash(name + ".producer"));
         public_key = private_key.get_public_key();
         default_genesis_state.initial_producers.emplace_back(name, public_key);
         idump((name)(public_key));
         key_ring[public_key] = private_key;
      }
   }

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
   fc::path get_temp_dir(std::string id = std::string()) {
      if (id.empty())
         return anonymous_temp_dirs.emplace().first->path();
      if (named_temp_dirs.count(id))
         return named_temp_dirs[id].path();
      return named_temp_dirs.emplace(std::make_pair(id, fc::temp_directory())).first->second.path();
   }

   const genesis_state_type& genesis_state() const {
      return default_genesis_state;
   }
   genesis_state_type& genesis_state() {
      return default_genesis_state;
   }

   private_key_type get_private_key(const public_key_type& public_key) const {
      auto itr = key_ring.find(public_key);
      EOS_ASSERT(itr != key_ring.end(), testing_exception,
                 "Private key corresponding to public key ${k} not known.", ("k", public_key));
      return itr->second;
   }

protected:
   std::set<fc::temp_directory> anonymous_temp_dirs;
   std::map<std::string, fc::temp_directory> named_temp_dirs;
   std::map<public_key_type, private_key_type> key_ring;
   genesis_state_type default_genesis_state;
};

/**
 * @brief The testing_database class wraps database and eliminates some of the boilerplate for common operations on the
 * database during testing.
 *
 * testing_databases have an optional ID, which is passed to the constructor. If two testing_databases are created with
 * the same ID, they will have the same data directory. If no ID, or an empty ID, is provided, the database will have a
 * unique data directory which no subsequent testing_database will be assigned.
 *
 * testing_database helps with producing blocks, or missing blocks, via the @ref produce_blocks and @ref miss_blocks
 * methods. To produce N blocks, simply call produce_blocks(N); to miss N blocks, call miss_blocks(N). Note that missing
 * blocks has no effect on the database until the next block, following the missed blocks, is produced.
 */
class testing_database : public database {
public:
   testing_database(testing_fixture& fixture, std::string id = std::string(),
                    fc::optional<genesis_state_type> override_genesis_state = {})
      : genesis_state(override_genesis_state? *override_genesis_state : fixture.genesis_state()),
        fixture(fixture) {
      data_dir = fixture.get_temp_dir(id);
   }

   /**
    * @brief Open the database using the boilerplate testing database settings
    */
   void open() {
      database::open(data_dir, TEST_DB_SIZE, [this]{return genesis_state;});
   }

   /**
    * @brief Produce new blocks, adding them to the database
    * @param count Number of blocks to produce
    */
   void produce_blocks(uint32_t count = 1) {
      if (count == 0)
         return;

      for (int i = 0; i < count; ++i) {
         auto slot = get_slot_at_time(next_block_time);
         auto producer_id = get_scheduled_producer(slot);
         const auto& producer = get(producer_id);
         auto private_key = fixture.get_private_key(producer.signing_key);
         ilog("Producing block at time ${t} with producer ${p}, signing key ${k}",
              ("t", next_block_time)("p", producer.owner_name.c_str())("k", public_key_type(private_key.get_public_key())));
         generate_block(next_block_time, producer_id, private_key, 0);
         next_block_time = get_slot_time(1);
      }
   }

   /**
    * @brief Advance the clock so that when the next block is produced, count blocks will have been missed
    * @param count Number of blocks to miss
    *
    * Note that this function has no effect on the database directly, as the passage of time doesn't affect the
    * database until a block is produced, and this function does not produce blocks. Instead, the next time @ref
    * produce_blocks is called, it will produce blocks following a gap of count missed blocks.
    */
   void miss_blocks(uint32_t count = 1) {
      auto next_slot = get_slot_at_time(next_block_time);
      next_slot += count;
      next_block_time = get_slot_time(next_slot);
   }

protected:
   fc::path data_dir;
   genesis_state_type genesis_state;
   fc::time_point_sec next_block_time = genesis_state.initial_timestamp +
                                        genesis_state.initial_parameters.block_interval;

   testing_fixture& fixture;
};

} }
