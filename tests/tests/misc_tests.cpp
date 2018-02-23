/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/authority.hpp>

#include <eosio/utilities/key_conversion.hpp>
#include <eosio/utilities/rand.hpp>

#include <fc/io/json.hpp>

#include <boost/test/unit_test.hpp>

using namespace eosio::chain;


namespace eosio {
using namespace chain;
using namespace std;

BOOST_AUTO_TEST_SUITE(misc_tests)

/// Test processing of unbalanced strings
BOOST_AUTO_TEST_CASE(json_from_string_test)
{
  bool exc_found = false;
  try {
    auto val = fc::json::from_string("{\"}");
  } catch(...) {
    exc_found = true;
  }
  BOOST_CHECK_EQUAL(exc_found, true);

  exc_found = false;
  try {
    auto val = fc::json::from_string("{\"block_num_or_id\":5");
  } catch(...) {
    exc_found = true;
  }
  BOOST_CHECK_EQUAL(exc_found, true);
}

/* Disabling test. It may no longer be relevant
/// Test calculation of median values of blockchain operation properties
BOOST_AUTO_TEST_CASE(median_properties_test)
{ try {
      vector<blockchain_configuration> votes{
         {1024  , 512   , 4096  , asset(5000   ).amount, asset(4000   ).amount, asset(100  ).amount, 512   , 6, 1000  , 3, 4096, 65536 },
         {10000 , 100   , 4096  , asset(3333   ).amount, asset(27109  ).amount, asset(10   ).amount, 100   , 6, 3000  , 2, 4096, 65536 },
         {2048  , 1500  , 1000  , asset(5432   ).amount, asset(2000   ).amount, asset(50   ).amount, 1500  , 6, 5000  , 9, 4096, 65536 },
         {100   , 25    , 1024  , asset(90000  ).amount, asset(0      ).amount, asset(433  ).amount, 25    , 6, 10000 , 4, 4096, 65536 },
         {1024  , 1000  , 100   , asset(10     ).amount, asset(50     ).amount, asset(200  ).amount, 1000  , 6, 4000  , 1, 4096, 65536 },
      };
      blockchain_configuration medians{
         1024, 512, 1024, asset(5000).amount, asset(2000).amount, asset(100).amount, 512, 6, 4000, 3, 4096, 65536
      };

      BOOST_CHECK_EQUAL(blockchain_configuration::get_median_values(votes), medians);

      votes.emplace_back(blockchain_configuration{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
      votes.emplace_back(blockchain_configuration{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
      medians = blockchain_configuration {1024, 100, 1000, asset(3333).amount, asset(50).amount, asset(50).amount, 100, 6, 3000, 2, 4096, 65536 };

      BOOST_CHECK_EQUAL(blockchain_configuration::get_median_values(votes), medians);
      BOOST_CHECK_EQUAL(blockchain_configuration::get_median_values({medians}), medians);

      votes.erase(votes.begin() + 2);
      votes.erase(votes.end() - 1);
      medians = blockchain_configuration {1024, 100, 1024, asset(3333).amount, asset(50).amount, asset(100).amount, 100, 6, 3000, 2, 4096, 65536 };
      BOOST_CHECK_EQUAL(blockchain_configuration::get_median_values(votes), medians);
} FC_LOG_AND_RETHROW() }
*/

/// Test that our deterministic random shuffle algorithm gives the same results in all environments
BOOST_AUTO_TEST_CASE(deterministic_randomness)
{ try {
   utilities::rand::random rng(123454321);
   vector<string> words = {"infamy", "invests", "estimated", "potters", "memorizes", "hal9000"};
   rng.shuffle(words);
   BOOST_CHECK_EQUAL(fc::json::to_string(words),
                     fc::json::to_string(vector<string>{"hal9000","infamy","invests","estimated","memorizes","potters"}));
   rng.shuffle(words);
   BOOST_CHECK_EQUAL(fc::json::to_string(words), 
                     fc::json::to_string(vector<string>{"memorizes","infamy","hal9000","potters","estimated","invests"}));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deterministic_distributions)
{ try {
   utilities::rand::random rng(123454321);

   BOOST_CHECK_EQUAL(rng.next(), UINT64_C(13636622732572118961));
   BOOST_CHECK_EQUAL(rng.next(), UINT64_C(8049736256506128729));
   BOOST_CHECK_EQUAL(rng.next(), UINT64_C(1224405983932261174));

   std::vector<int> nums = {0, 1, 2};

   rng.shuffle(nums);
   std::vector<int> a{2, 0, 1};
   BOOST_CHECK(std::equal(nums.begin(), nums.end(), a.begin()));
   rng.shuffle(nums);
   std::vector<int> b{0, 2, 1};
   BOOST_CHECK(std::equal(nums.begin(), nums.end(), b.begin()));
   rng.shuffle(nums);
   std::vector<int> c{1, 0, 2};
   BOOST_CHECK(std::equal(nums.begin(), nums.end(), c.begin()));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(alphabetic_sort)
{ try {

  vector<string> words = {
    "com.o",
    "te",
    "a.....5",
    "a...4",
    ".va.ku",
    "gh",
    "1ho.la",
    "g1",
    "g",
    "a....2",
    "gg",
    "va",
    "lale.....12b",
    "a....3",
    "a....1",
    "..g",
    ".g",
    "....g",
    "a....y",
    "...g",
    "lale.....333",
  };
  
  std::sort(words.begin(), words.end(), std::less<string>());

  vector<uint64_t> uwords;
  for(const auto w: words) {
    auto n = name(w.c_str());
    uwords.push_back(n.value);
  }

  std::sort(uwords.begin(), uwords.end(), std::less<uint64_t>());

  vector<string> tmp;
  for(const auto uw: uwords) {
    auto str = name(uw).to_string();
    tmp.push_back(str);
  }

  for(int i = 0; i < words.size(); ++i ) {
    BOOST_CHECK_EQUAL(tmp[i], words[i]);
  }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

} // namespace eosio
