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
#include <eos/chain/exceptions.hpp>
#include <eos/utilities/tempdir.hpp>
#include <fc/io/json.hpp>
#include <fc/smart_ref_impl.hpp>
#include <fc/signals.hpp>

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

#include <iostream>

using namespace eos::chain;

extern uint32_t EOS_TESTING_GENESIS_TIMESTAMP;

#define TEST_DB_SIZE (1024*1024*10)

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

namespace eos { namespace chain {
FC_DECLARE_EXCEPTION(testing_exception, 6000000, "test framework exception")
FC_DECLARE_DERIVED_EXCEPTION(missing_key_exception, eos::chain::testing_exception, 6010000, "key could not be found")

/**
 * @brief The testing_fixture class provides various services relevant to testing the database.
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

   const genesis_state_type& genesis_state() const;
   genesis_state_type& genesis_state();

   private_key_type get_private_key(const public_key_type& public_key) const;

protected:
   std::vector<fc::temp_directory> anonymous_temp_dirs;
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
                    fc::optional<genesis_state_type> override_genesis_state = {});

   /**
    * @brief Open the database using the boilerplate testing database settings
    */
   void open();
   /**
    * @brief Reindex the database using the boilerplate testing database settings
    */
   void reindex();
   /**
    * @brief Wipe the database using the boilerplate testing database settings
    * @param include_blocks If true, the blocks will be removed as well; otherwise, only the database will be wiped and
    * can then be rebuilt from the local blocks
    */
   void wipe(bool include_blocks = true);

   /**
    * @brief Produce new blocks, adding them to the database, optionally following a gap of missed blocks
    * @param count Number of blocks to produce
    * @param blocks_to_miss Number of block intervals to miss a production before producing the next block
    *
    * Creates and adds  @ref count new blocks to the database, after going @ref blocks_to_miss intervals without
    * producing a block.
    */
   void produce_blocks(uint32_t count = 1, uint32_t blocks_to_miss = 0);

   /**
    * @brief Sync this database with other
    * @param other Database to sync with
    *
    * To sync the databases, all blocks from one database which are unknown to the other are pushed to the other, then
    * the same thing in reverse. Whichever database has more blocks will have its blocks sent to the other first.
    *
    * Blocks not on the main fork are ignored.
    */
   void sync_with(testing_database& other);

protected:
   fc::path data_dir;
   genesis_state_type genesis_state;

   testing_fixture& fixture;
};

/// Some helpful macros to reduce boilerplate when making testing_databases @{
#define MKDB1(name) testing_database name(*this); name.open();
#define MKDB2(name, id) testing_database name(*this, #id); name.open();
/**
 * @brief Create/Open a testing_database, optionally with an ID
 *
 * Creates and opens a testing_database with the first argument as its name, and, if present, the second argument as
 * its ID. The ID should be provided without quotes.
 *
 * Example:
 * @code{.cpp}
 * // Create testing_databases db1 and db2, with db2 having ID "id2"
 * MKDB(db1)
 * MKDB(db2, id2)
 * @endcode
 */
#define MKDB(...) BOOST_PP_OVERLOAD(MKDB, __VA_ARGS__)(__VA_ARGS__)
#define MKDBS_MACRO(x, y, name) MKDB(name)
/**
 * @brief Similar to @ref MKDB, but works with several databases at once
 *
 * Creates and opens several testing_databases
 *
 * Example:
 * @code{.cpp}
 * // Create testing_databases db1 and db2, with db2 having ID "id2"
 * MKDBS((db1)(db2, id2))
 * @endcode
 */
#define MKDBS(...) BOOST_PP_SEQ_FOR_EACH(MKDBS_MACRO, _, __VA_ARGS__)
/// @}

/**
 * @brief The testing_network class provides a simplistic virtual P2P network connecting testing_databases together.
 *
 * A testing_database may be connected to zero or more testing_networks at any given time. When a new testing_database
 * joins the network, it will be synced with all other databases already in the network (blocks known only to the new
 * database will be pushed to the prior network members and vice versa, ignoring blocks not on the main fork). After
 * this, whenever any database in the network gets a new block, that block will be pushed to all other databases in the
 * network as well.
 */
class testing_network {
public:
   /**
    * @brief Add a new database to the network
    * @param new_database The database to add
    */
   void connect_database(testing_database& new_database);
   /**
    * @brief Remove a database from the network
    * @param leaving_database The database to remove
    */
   void disconnect_database(testing_database& leaving_database);
   /**
    * @brief Disconnect all databases from the network
    */
   void disconnect_all();

   /**
    * @brief Send a block to all databases in this network
    * @param block The block to send
    */
   void propagate_block(const signed_block& block);

protected:
   std::map<testing_database*, fc::scoped_connection> databases;
   bool currently_propagating_block = false;
};

/// Some helpful macros to reduce boilerplate when making a testing_network and connecting some testing_databases @{
#define MKNET1(name) testing_network name;
#define MKNET2_MACRO(x, name, db) name.connect_database(db);
#define MKNET2(name, ...) MKNET1(name) BOOST_PP_SEQ_FOR_EACH(MKNET2_MACRO, name, __VA_ARGS__)
/**
 * @brief MKNET is a shorthand way to create a testing_network and connect some testing_databases to it.
 *
 * Example usage:
 * @code{.cpp}
 * // Create and open testing_databases named alice, bob, and charlie
 * MKDBS((alice)(bob)(charlie))
 * // Create a testing_network named net and connect alice and bob to it
 * MKNET(net, (alice)(bob))
 *
 * // Connect charlie to net, then disconnect alice
 * net.connect_database(charlie);
 * net.disconnect_database(alice);
 *
 * // Create a testing_network named net2 with no databases connected
 * MKNET(net2)
 * @endcode
 */
#define MKNET(...) BOOST_PP_OVERLOAD(MKNET, __VA_ARGS__)(__VA_ARGS__)
/// @}

} }
