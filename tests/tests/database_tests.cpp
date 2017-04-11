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

#include <eos/chain/database.hpp>
#include <eos/chain/account_object.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

using namespace eos::chain;

// Simple tests of undo infrastructure
BOOST_FIXTURE_TEST_CASE(undo_test, testing_fixture)
{ try {
      testing_database db(*this);
      db.open();
      auto ses = db.start_undo_session(true);

      // Create an account
      db.create<account_object>([](account_object& a) {
          a.name = "Billy the Kid";
      });

      // Make sure we can retrieve that account by name
      auto ptr = db.find<account_object, by_name, std::string>("Billy the Kid");
      BOOST_CHECK(ptr != nullptr);

      // Undo creation of the account
      ses.undo();

      // Make sure we can no longer find the account
      ptr = db.find<account_object, by_name, std::string>("Billy the Kid");
      BOOST_CHECK(ptr == nullptr);
} FC_LOG_AND_RETHROW() }

// Test the block fetching methods on database, get_block_id_for_num, fetch_bock_by_id, and fetch_block_by_num
BOOST_FIXTURE_TEST_CASE(get_blocks, testing_fixture)
{ try {
      MKDB(db)

      // I apologize if this proves fragile in the future. Any changes to how block IDs are calculated, or producer
      // scheduling, etc. will break this, but hopefully it should be pretty easy to just swap in the new IDs... :]
      block_id_type block_ids[] = {
         block_id_type("0000000130e605ebe8ba3da0d5d1ed556ccfde3b"),
         block_id_type("00000002a23c51a99d8d32b671012ebc221a4098"),
         block_id_type("00000003eaf03b2aca76f64db5e640325c0e7b3f"),
         block_id_type("000000041e533056ead792fd5935d23c84311b30"),
         block_id_type("000000058aa9a6436929741ffee02fed1f2b4290"),
         block_id_type("000000063037551d2353ffb0ab07a9b5e582e66b"),
         block_id_type("00000007f3960fb247bd95feb9280565b4a0eb2e"),
         block_id_type("0000000893bd630f901ff78bd5d5a28574698f45"),
         block_id_type("0000000928143087ee97d5429be8282b1ce2d323"),
         block_id_type("0000000a3b9c0c3a596069f36f678f0e69a6a7a4"),
         block_id_type("0000000bf05985672af3481f1f1c780ac40b1299"),
         block_id_type("0000000c6665f984a6e994c7732255aaab25dee2"),
         block_id_type("0000000d3cf47c047f681451508efd07d418574c"),
         block_id_type("0000000e62d6764b69c4d2bb72047676bfff5e79"),
         block_id_type("0000000f85aa69d6f5d900e3e26daf5f19d91d91"),
         block_id_type("000000106c1e81538bf6ce21e9d000d7c9beb20e"),
         block_id_type("00000011cd6a0d410addfdf8c7816c168e24af6e"),
         block_id_type("000000120b333f3264d605127b50e28b9679279f"),
         block_id_type("000000138983847a10b1dc66503c556e9e66f461"),
         block_id_type("0000001420541bed52c10949a82c5d0b311fb489")
      };

      // Produce 20 blocks and check their IDs should match the above
      db.produce_blocks(20);
      for (int i = 0; i < 20; ++i) {
         BOOST_CHECK(db.is_known_block(block_ids[i]));
         BOOST_CHECK_EQUAL(db.get_block_id_for_num(i+1), block_ids[i]);
         BOOST_CHECK_EQUAL(db.fetch_block_by_id(block_ids[i])->block_num(), i+1);
         BOOST_CHECK_EQUAL(db.fetch_block_by_number(i+1)->id(), block_ids[i]);
      }

      // Check the last irreversible block number is set correctly
      BOOST_CHECK_EQUAL(db.last_irreversible_block_num(), 13);
      // Check that block 21 cannot be found (only 20 blocks exist)
      BOOST_CHECK_THROW(db.get_block_id_for_num(21), eos::chain::unknown_block_exception);

      db.produce_blocks();
      // Check the last irreversible block number is updated correctly
      BOOST_CHECK_EQUAL(db.last_irreversible_block_num(), 14);
      // Check that block 21 can now be found
      BOOST_CHECK_EQUAL(db.get_block_id_for_num(21), db.head_block_id());
} FC_LOG_AND_RETHROW() }
