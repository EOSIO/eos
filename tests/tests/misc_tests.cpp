#include <eos/chain/BlockchainConfiguration.hpp>

#include <eos/utilities/randutils.hpp>
#include <eos/utilities/pcg-random/pcg_random.hpp>

#include <fc/io/json.hpp>

#include <boost/test/unit_test.hpp>

namespace eos {
using namespace chain;

BOOST_AUTO_TEST_SUITE(misc_tests)

/// Test calculation of median values of blockchain operation properties
BOOST_AUTO_TEST_CASE(median_properties_test)
{ try {
      vector<BlockchainConfiguration> votes{
         {1024  , 512   , 4096  , Asset(5000   ).amount, Asset(4000   ).amount, Asset(100  ).amount, 512   },
         {10000 , 100   , 4096  , Asset(3333   ).amount, Asset(27109  ).amount, Asset(10   ).amount, 100   },
         {2048  , 1500  , 1000  , Asset(5432   ).amount, Asset(2000   ).amount, Asset(50   ).amount, 1500  },
         {100   , 25    , 1024  , Asset(90000  ).amount, Asset(0      ).amount, Asset(433  ).amount, 25    },
         {1024  , 1000  , 100   , Asset(10     ).amount, Asset(50     ).amount, Asset(200  ).amount, 1000  },
      };
      BlockchainConfiguration medians{
         1024, 512, 1024, Asset(5000).amount, Asset(2000).amount, Asset(100).amount, 512
      };

      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);

      votes.emplace_back(BlockchainConfiguration{1, 1, 1, 1, 1, 1, 1});
      votes.emplace_back(BlockchainConfiguration{1, 1, 1, 1, 1, 1, 1});
      medians = BlockchainConfiguration {1024, 100, 1000, Asset(3333).amount, Asset(50).amount, Asset(50).amount, 100};

      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);
      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values({medians}), medians);

      votes.erase(votes.begin() + 2);
      votes.erase(votes.end() - 1);
      medians = BlockchainConfiguration {1024, 100, 1024, Asset(3333).amount, Asset(50).amount, Asset(100).amount, 100};
      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);
} FC_LOG_AND_RETHROW() }

/// Test that our deterministic random shuffle algorithm gives the same results in all environments
BOOST_AUTO_TEST_CASE(deterministic_randomness)
{ try {
   randutils::seed_seq_fe<1> seed({123454321});
   randutils::random_generator<pcg32_fast> rng(seed);
   vector<string> words = {"infamy", "invests", "estimated", "potters", "memorizes", "hal9000"};
   rng.shuffle(words);
   BOOST_CHECK_EQUAL(fc::json::to_string(words),
                     fc::json::to_string(vector<string>{"hal9000","infamy","estimated","memorizes","invests","potters"}));
   rng.shuffle(words);
   BOOST_CHECK_EQUAL(fc::json::to_string(words), 
                     fc::json::to_string(vector<string>{"hal9000","estimated","infamy","memorizes","potters","invests"}));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deterministic_distributions)
{ try {
   randutils::seed_seq_fe<1> seed({123454321});
   randutils::random_generator<pcg32_fast> rng(seed);

   std::vector<int> nums = {0, 1, 2};

   BOOST_CHECK_EQUAL(rng.uniform(0.0, 1.0), 0.52802440151572227);
   BOOST_CHECK_EQUAL(rng.uniform(0.0, 1.0), 0.36562641779892147);
   BOOST_CHECK_EQUAL(rng.uniform(0.0, 1.0), 0.44247416267171502);

   BOOST_CHECK_EQUAL(rng.pick(nums), 2);
   BOOST_CHECK_EQUAL(rng.pick(nums), 0);
   BOOST_CHECK_EQUAL(rng.pick(nums), 2);

   rng.shuffle(nums);
   std::vector<int> a{0, 1, 2};
   BOOST_CHECK(std::equal(nums.begin(), nums.end(), a.begin()));
   rng.shuffle(nums);
   std::vector<int> b{0, 2, 1};
   BOOST_CHECK(std::equal(nums.begin(), nums.end(), b.begin()));
   rng.shuffle(nums);
   std::vector<int> c{1, 0, 2};
   BOOST_CHECK(std::equal(nums.begin(), nums.end(), c.begin()));

   BOOST_CHECK_EQUAL(rng.uniform(0, 9), 2);
   BOOST_CHECK_EQUAL(rng.uniform(0, 9), 9);
   BOOST_CHECK_EQUAL(rng.uniform(0, 9), 0);
} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()

} // namespace eos
