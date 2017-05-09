#include <eos/chain/BlockchainConfiguration.hpp>

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
      medians = {1024, 100, 1000, Asset(3333).amount, Asset(50).amount, Asset(50).amount, 100};

      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);
      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values({medians}), medians);

      votes.erase(votes.begin() + 2);
      votes.erase(votes.end() - 1);
      medians = {1024, 100, 1024, Asset(3333).amount, Asset(50).amount, Asset(100).amount, 100};
      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

} // namespace eos
