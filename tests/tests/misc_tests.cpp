#include <eos/chain/BlockchainConfiguration.hpp>
#include <eos/chain/authority_checker.hpp>
#include <eos/chain/authority.hpp>

#include <eos/utilities/key_conversion.hpp>
#include <eos/utilities/rand.hpp>

#include <fc/io/json.hpp>

#include <boost/test/unit_test.hpp>

using namespace eos::chain;
#include "../common/testing_macros.hpp"

namespace eos {
using namespace chain;
using namespace std;

BOOST_AUTO_TEST_SUITE(misc_tests)

/// Test calculation of median values of blockchain operation properties
BOOST_AUTO_TEST_CASE(median_properties_test)
{ try {
      vector<BlockchainConfiguration> votes{
         {1024  , 512   , 4096  , Asset(5000   ).amount, Asset(4000   ).amount, Asset(100  ).amount, 512   , 6, 1000  , 3, 4096, 65536 },
         {10000 , 100   , 4096  , Asset(3333   ).amount, Asset(27109  ).amount, Asset(10   ).amount, 100   , 6, 3000  , 2, 4096, 65536 },
         {2048  , 1500  , 1000  , Asset(5432   ).amount, Asset(2000   ).amount, Asset(50   ).amount, 1500  , 6, 5000  , 9, 4096, 65536 },
         {100   , 25    , 1024  , Asset(90000  ).amount, Asset(0      ).amount, Asset(433  ).amount, 25    , 6, 10000 , 4, 4096, 65536 },
         {1024  , 1000  , 100   , Asset(10     ).amount, Asset(50     ).amount, Asset(200  ).amount, 1000  , 6, 4000  , 1, 4096, 65536 },
      };
      BlockchainConfiguration medians{
         1024, 512, 1024, Asset(5000).amount, Asset(2000).amount, Asset(100).amount, 512, 6, 4000, 3, 4096, 65536
      };

      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);

      votes.emplace_back(BlockchainConfiguration{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
      votes.emplace_back(BlockchainConfiguration{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
      medians = BlockchainConfiguration {1024, 100, 1000, Asset(3333).amount, Asset(50).amount, Asset(50).amount, 100, 6, 3000, 2, 4096, 65536 };

      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);
      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values({medians}), medians);

      votes.erase(votes.begin() + 2);
      votes.erase(votes.end() - 1);
      medians = BlockchainConfiguration {1024, 100, 1024, Asset(3333).amount, Asset(50).amount, Asset(100).amount, 100, 6, 3000, 2, 4096, 65536 };
      BOOST_CHECK_EQUAL(BlockchainConfiguration::get_median_values(votes), medians);
} FC_LOG_AND_RETHROW() }

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

BOOST_AUTO_TEST_CASE(authority_checker)
{ try {
   Make_Key(a);
   auto& a = a_public_key;
   Make_Key(b);
   auto& b = b_public_key;
   Make_Key(c);
   auto& c = c_public_key;

   auto GetNullAuthority = [](auto){abort(); return Authority();};

   auto A = Complex_Authority(2, ((a,1))((b,1)),);
   {
      auto checker = MakeAuthorityChecker(GetNullAuthority, 2, {a, b});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 0);
   }
   {
      auto checker = MakeAuthorityChecker(GetNullAuthority, 2, {a, c});
      BOOST_CHECK(!checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 0);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 2);
   }
   {
      auto checker = MakeAuthorityChecker(GetNullAuthority, 2, {a, b, c});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.used_keys().count(a), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(b), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().count(c), 1);
   }
   {
      auto checker = MakeAuthorityChecker(GetNullAuthority, 2, {b, c});
      BOOST_CHECK(!checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 0);
   }

   A = Complex_Authority(3, ((a,1))((b,1))((c,1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, 2, {c, b, a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, 2, {a, b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, 2, {a, c}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, 2, {b, c}).satisfied(A));

   A = Complex_Authority(1, ((a, 1))((b, 1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, 2, {a}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, 2, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, 2, {c}).satisfied(A));

   A = Complex_Authority(1, ((a, 2))((b, 1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, 2, {a}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, 2, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, 2, {c}).satisfied(A));

   auto GetCAuthority = [c_public_key](auto){return Complex_Authority(1, ((c, 1)),);};

   A = Complex_Authority(2, ((a, 2))((b, 1)), (("hello", "world", 1)));
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {a});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(checker.all_keys_used());
   }
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {b});
      BOOST_CHECK(!checker.satisfied(A));
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 0);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().count(b), 1);
   }
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {c});
      BOOST_CHECK(!checker.satisfied(A));
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 0);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().count(c), 1);
   }
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {b,c});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 0);
      BOOST_CHECK_EQUAL(checker.used_keys().count(b), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(c), 1);
   }
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {b,c,a});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(a), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.unused_keys().count(b), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().count(c), 1);
   }

   A = Complex_Authority(2, ((a, 1))((b, 1)), (("hello", "world", 1)));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, 2, {a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, 2, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, 2, {c}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, 2, {a,b}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, 2, {b,c}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, 2, {a,c}).satisfied(A));
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {a,b,c});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().count(c), 1);
   }

   A = Complex_Authority(2, ((a, 1))((b, 1)), (("hello", "world", 2)));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, 2, {a,b}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, 2, {c}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, 2, {a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, 2, {b}).satisfied(A));
   {
      auto checker = MakeAuthorityChecker(GetCAuthority, 2, {a,b,c});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.used_keys().count(c), 1);
   }

   Make_Key(d);
   auto& d = d_public_key;
   Make_Key(e);
   auto& e = e_public_key;

   auto GetAuthority = [d_public_key, e] (const types::AccountPermission& perm) {
      if (perm.account == "top")
         return Complex_Authority(2, ((d, 1)), (("bottom", "bottom", 1)));
      return Key_Authority(e);
   };

   A = Complex_Authority(5, ((a, 2))((b, 2))((c, 2)), (("top", "top", 5)));
   {
      auto checker = MakeAuthorityChecker(GetAuthority, 2, {d, e});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(checker.all_keys_used());
   }
   {
      auto checker = MakeAuthorityChecker(GetAuthority, 2, {a,b,c,d,e});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 2);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 3);
      BOOST_CHECK_EQUAL(checker.used_keys().count(d), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(e), 1);
   }
   {
      auto checker = MakeAuthorityChecker(GetAuthority, 2, {a,b,c,e});
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.used_keys().size(), 3);
      BOOST_CHECK_EQUAL(checker.unused_keys().size(), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(a), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(b), 1);
      BOOST_CHECK_EQUAL(checker.used_keys().count(c), 1);
   }
   BOOST_CHECK(MakeAuthorityChecker(GetAuthority, 1, {a,b,c}).satisfied(A));
   // Fails due to short recursion depth limit
   BOOST_CHECK(!MakeAuthorityChecker(GetAuthority, 1, {d,e}).satisfied(A));

   A = Complex_Authority(2, ((a, 1))((b, 1))((c, 1)),);
   auto B = Complex_Authority(1, ((b, 1))((c, 1)),);
   {
      auto checker = MakeAuthorityChecker(GetNullAuthority, 2, {a,b,c});
      BOOST_CHECK(validate(A));
      BOOST_CHECK(validate(B));
      BOOST_CHECK(checker.satisfied(A));
      BOOST_CHECK(checker.satisfied(B));
      BOOST_CHECK(!checker.all_keys_used());
      BOOST_CHECK_EQUAL(checker.unused_keys().count(c), 1);
   }
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
    auto n = Name(w.c_str());
    uwords.push_back(n.value);
  }

  std::sort(uwords.begin(), uwords.end(), std::less<uint64_t>());

  vector<string> tmp;
  for(const auto uw: uwords) {
    auto str = Name(uw).toString();
    tmp.push_back(str);
  }

  for(int i = 0; i < words.size(); ++i ) {
    BOOST_CHECK_EQUAL(tmp[i], words[i]);
  }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

} // namespace eos
