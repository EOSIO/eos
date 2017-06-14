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

#include <eos/chain/chain_controller.hpp>
#include <eos/chain/account_object.hpp>
#include <chainbase/chainbase.hpp>

#include <fc/crypto/digest.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

namespace eos {
using namespace chain;
namespace bfs = boost::filesystem;

// Simple tests of undo infrastructure
BOOST_FIXTURE_TEST_CASE(undo_test, testing_fixture)
{ try {
      auto db = database(get_temp_dir(), database::read_write, 8*1024*1024);
      db.add_index<account_index>();
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

// Test the block fetching methods on database, get_block_id_for_num, fetch_bock_by_id, and fetch_block_by_number
BOOST_FIXTURE_TEST_CASE(get_blocks, testing_fixture)
{ try {
      Make_Database(db)
      vector<block_id_type> block_ids;

      // Produce 20 blocks and check their IDs should match the above
      db.produce_blocks(20);
      for (int i = 0; i < 20; ++i) {
         block_ids.emplace_back(db.fetch_block_by_number(i+1)->id());
         BOOST_CHECK_EQUAL(block_header::num_from_id(block_ids.back()), i+1);
         BOOST_CHECK_EQUAL(db.get_block_id_for_num(i+1), block_ids.back());
         BOOST_CHECK_EQUAL(db.fetch_block_by_number(i+1)->id(), block_ids.back());
      }

      // Check the last irreversible block number is set correctly
      BOOST_CHECK_EQUAL(db.last_irreversible_block_num(), 6);
      // Check that block 21 cannot be found (only 20 blocks exist)
      BOOST_CHECK_THROW(db.get_block_id_for_num(21), eos::chain::unknown_block_exception);

      db.produce_blocks();
      // Check the last irreversible block number is updated correctly
      BOOST_CHECK_EQUAL(db.last_irreversible_block_num(), 7);
      // Check that block 21 can now be found
      BOOST_CHECK_EQUAL(db.get_block_id_for_num(21), db.head_block_id());
} FC_LOG_AND_RETHROW() }

// Test producer votes on blockchain parameters in full blockchain context
BOOST_FIXTURE_TEST_CASE(producer_voting_parameters, testing_fixture)
{ try {
      Make_Database(db)
      db.produce_blocks(21);

      vector<BlockchainConfiguration> votes{
         {1024  , 512   , 4096  , Asset(5000   ).amount, Asset(4000   ).amount, Asset(100  ).amount, 512   },
         {10000 , 100   , 4096  , Asset(3333   ).amount, Asset(27109  ).amount, Asset(10   ).amount, 100   },
         {2048  , 1500  , 1000  , Asset(5432   ).amount, Asset(2000   ).amount, Asset(50   ).amount, 1500  },
         {100   , 25    , 1024  , Asset(90000  ).amount, Asset(0      ).amount, Asset(433  ).amount, 25    },
         {1024  , 1000  , 100   , Asset(10     ).amount, Asset(50     ).amount, Asset(200  ).amount, 1000  },
         {420   , 400   , 2710  , Asset(27599  ).amount, Asset(1177   ).amount, Asset(27720).amount, 400   },
         {271   , 200   , 66629 , Asset(2666   ).amount, Asset(99991  ).amount, Asset(277  ).amount, 200   },
         {1057  , 1000  , 2770  , Asset(972    ).amount, Asset(302716 ).amount, Asset(578  ).amount, 1000  },
         {9926  , 27    , 990   , Asset(99999  ).amount, Asset(39651  ).amount, Asset(4402 ).amount, 27    },
         {1005  , 1000  , 1917  , Asset(937111 ).amount, Asset(2734   ).amount, Asset(1    ).amount, 1000  },
         {80    , 70    , 5726  , Asset(63920  ).amount, Asset(231561 ).amount, Asset(27100).amount, 70    },
         {471617, 333333, 100   , Asset(2666   ).amount, Asset(2650   ).amount, Asset(2772 ).amount, 333333},
         {2222  , 1000  , 100   , Asset(33619  ).amount, Asset(1046   ).amount, Asset(10577).amount, 1000  },
         {8     , 7     , 100   , Asset(5757267).amount, Asset(2257   ).amount, Asset(2888 ).amount, 7     },
         {2717  , 2000  , 57797 , Asset(3366   ).amount, Asset(205    ).amount, Asset(4472 ).amount, 2000  },
         {9997  , 5000  , 27700 , Asset(29199  ).amount, Asset(100    ).amount, Asset(221  ).amount, 5000  },
         {163900, 200   , 882   , Asset(100    ).amount, Asset(5720233).amount, Asset(105  ).amount, 200   },
         {728   , 80    , 27100 , Asset(28888  ).amount, Asset(6205   ).amount, Asset(5011 ).amount, 80    },
         {91937 , 44444 , 652589, Asset(87612  ).amount, Asset(123    ).amount, Asset(2044 ).amount, 44444 },
         {171   , 96    , 123456, Asset(8402   ).amount, Asset(321    ).amount, Asset(816  ).amount, 96    },
         {17177 , 6767  , 654321, Asset(9926   ).amount, Asset(9264   ).amount, Asset(8196 ).amount, 6767  },
      };
      BlockchainConfiguration medians =
         {1057, 512, 2770, Asset(9926).amount, Asset(2650).amount, Asset(816).amount, 512};
      // If this fails, the medians variable probably needs to be updated to have the medians of the votes above
      BOOST_REQUIRE_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);

      for (int i = 0; i < votes.size(); ++i) {
         auto name = std::string("init") + fc::to_string(i);
         Update_Producer(db, name, db.get_producer(name).signing_key, votes[i]);
      }

      BOOST_CHECK_NE(db.get_global_properties().configuration, medians);
      db.produce_blocks(20);
      BOOST_CHECK_NE(db.get_global_properties().configuration, medians);
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.get_global_properties().configuration, medians);
} FC_LOG_AND_RETHROW() }

// Test producer votes on blockchain parameters in full blockchain context, with missed blocks
BOOST_FIXTURE_TEST_CASE(producer_voting_parameters_2, testing_fixture)
{ try {
      Make_Database(db)
      db.produce_blocks(21);

      vector<BlockchainConfiguration> votes{
         {1024  , 512   , 4096  , Asset(5000   ).amount, Asset(4000   ).amount, Asset(100  ).amount, 512   },
         {10000 , 100   , 4096  , Asset(3333   ).amount, Asset(27109  ).amount, Asset(10   ).amount, 100   },
         {2048  , 1500  , 1000  , Asset(5432   ).amount, Asset(2000   ).amount, Asset(50   ).amount, 1500  },
         {100   , 25    , 1024  , Asset(90000  ).amount, Asset(0      ).amount, Asset(433  ).amount, 25    },
         {1024  , 1000  , 100   , Asset(10     ).amount, Asset(50     ).amount, Asset(200  ).amount, 1000  },
         {420   , 400   , 2710  , Asset(27599  ).amount, Asset(1177   ).amount, Asset(27720).amount, 400   },
         {271   , 200   , 66629 , Asset(2666   ).amount, Asset(99991  ).amount, Asset(277  ).amount, 200   },
         {1057  , 1000  , 2770  , Asset(972    ).amount, Asset(302716 ).amount, Asset(578  ).amount, 1000  },
         {9926  , 27    , 990   , Asset(99999  ).amount, Asset(39651  ).amount, Asset(4402 ).amount, 27    },
         {1005  , 1000  , 1917  , Asset(937111 ).amount, Asset(2734   ).amount, Asset(1    ).amount, 1000  },
         {80    , 70    , 5726  , Asset(63920  ).amount, Asset(231561 ).amount, Asset(27100).amount, 70    },
         {471617, 333333, 100   , Asset(2666   ).amount, Asset(2650   ).amount, Asset(2772 ).amount, 333333},
         {2222  , 1000  , 100   , Asset(33619  ).amount, Asset(1046   ).amount, Asset(10577).amount, 1000  },
         {8     , 7     , 100   , Asset(5757267).amount, Asset(2257   ).amount, Asset(2888 ).amount, 7     },
         {2717  , 2000  , 57797 , Asset(3366   ).amount, Asset(205    ).amount, Asset(4472 ).amount, 2000  },
         {9997  , 5000  , 27700 , Asset(29199  ).amount, Asset(100    ).amount, Asset(221  ).amount, 5000  },
         {163900, 200   , 882   , Asset(100    ).amount, Asset(5720233).amount, Asset(105  ).amount, 200   },
         {728   , 80    , 27100 , Asset(28888  ).amount, Asset(6205   ).amount, Asset(5011 ).amount, 80    },
         {91937 , 44444 , 652589, Asset(87612  ).amount, Asset(123    ).amount, Asset(2044 ).amount, 44444 },
         {171   , 96    , 123456, Asset(8402   ).amount, Asset(321    ).amount, Asset(816  ).amount, 96    },
         {17177 , 6767  , 654321, Asset(9926   ).amount, Asset(9264   ).amount, Asset(8196 ).amount, 6767  },
      };
      BlockchainConfiguration medians =
         {1057, 512, 2770, Asset(9926).amount, Asset(2650).amount, Asset(816).amount, 512};
      // If this fails, the medians variable probably needs to be updated to have the medians of the votes above
      BOOST_REQUIRE_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);

      for (int i = 0; i < votes.size(); ++i) {
         auto name = std::string("init") + fc::to_string(i);
         Update_Producer(db, name, db.get_producer(name).signing_key, votes[i]);
      }

      BOOST_CHECK_NE(db.get_global_properties().configuration, medians);
      db.produce_blocks(2);
      db.produce_blocks(18, 5);
      BOOST_CHECK_NE(db.get_global_properties().configuration, medians);
      db.produce_blocks();
      BOOST_CHECK_EQUAL(db.get_global_properties().configuration, medians);
} FC_LOG_AND_RETHROW() }

} // namespace eos
