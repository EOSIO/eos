/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/testing/tester.hpp>

#include <eosio/utilities/key_conversion.hpp>
#include <eosio/utilities/rand.hpp>

#include <fc/io/json.hpp>

#include <boost/test/unit_test.hpp>

using namespace eosio::chain;
namespace eosio
{
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

/// Test calculation of median values of blockchain operation properties
BOOST_AUTO_TEST_CASE(median_properties_test)
{
   try
   {
      vector<chain_config> votes{
          {512 , 1024 , 256, 512 , 512 , 1024 , 4096, 512 , 1000 , 6, 3, 4096, 65536},
          {100 , 10000, 50 , 5000, 100 , 10000, 4096, 100 , 3000 , 6, 2, 4096, 65536},
          {1500, 2048 , 750, 1024, 1500, 2048 , 1000, 1500, 5000 , 6, 9, 4096, 65536},
          {25  , 100  , 12 , 50  , 25  , 100  , 1024, 25  , 10000, 6, 4, 4096, 65536},
          {1000, 1024 , 500, 512 , 10  , 1024 , 100 , 1000, 4000 , 6, 1, 4096, 65536}};

      chain_config medians{
          512, 1024, 256, 512, 100, 1024, 1024, 512, 4000, 6, 3, 4096, 65536};

      BOOST_TEST(chain_config::get_median_values(votes) == medians);

      votes.emplace_back(chain_config{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
      votes.emplace_back(chain_config{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1});
      medians = chain_config{100, 1024, 50, 512, 25, 1024, 1000, 100, 3000, 6, 2, 4096, 65536};

      BOOST_TEST(chain_config::get_median_values(votes) == medians);
      BOOST_TEST(chain_config::get_median_values({medians}) == medians);

      votes.erase(votes.begin() + 2);
      votes.erase(votes.end() - 1);
      medians = chain_config{100, 1024, 50, 512, 25, 1024, 1024, 100, 3000, 6, 2, 4096, 65536};
      BOOST_TEST(chain_config::get_median_values(votes) == medians);
   }
   FC_LOG_AND_RETHROW()
}

/// Test that our deterministic random shuffle algorithm gives the same results in all environments
BOOST_AUTO_TEST_CASE(deterministic_randomness)
{ try {
   utilities::rand::random rng(123454321);
   vector<string> words = {"infamy", "invests", "estimated", "potters", "memorizes", "hal9000"};
   rng.shuffle(words);
   BOOST_TEST(fc::json::to_string(words) ==
                     fc::json::to_string(vector<string>{"hal9000","infamy","invests","estimated","memorizes","potters"}));
   rng.shuffle(words);
   BOOST_TEST(fc::json::to_string(words) ==
                     fc::json::to_string(vector<string>{"memorizes","infamy","hal9000","potters","estimated","invests"}));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(deterministic_distributions)
{ try {
   utilities::rand::random rng(123454321);

   BOOST_TEST(rng.next() == UINT64_C(13636622732572118961));
   BOOST_TEST(rng.next() == UINT64_C(8049736256506128729));
   BOOST_TEST(rng.next() ==  UINT64_C(1224405983932261174));

   std::vector<int> nums = {0, 1, 2};

   rng.shuffle(nums);
   std::vector<int> a{2, 0, 1};
   BOOST_TEST(std::equal(nums.begin(), nums.end(), a.begin()));
   rng.shuffle(nums);
   std::vector<int> b{0, 2, 1};
   BOOST_TEST(std::equal(nums.begin(), nums.end(), b.begin()));
   rng.shuffle(nums);
   std::vector<int> c{1, 0, 2};
   BOOST_TEST(std::equal(nums.begin(), nums.end(), c.begin()));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(authority_checker)
{ try {
   testing::tester test;
   auto a = test.get_public_key("a", "active");
   auto b = test.get_public_key("b", "active");
   auto c = test.get_public_key("c", "active");

   auto GetNullAuthority = [](auto){abort(); return authority();};

   auto A = authority(2, {key_weight{a, 1}, key_weight{b, 1}});
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, b});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2);
      BOOST_TEST(checker.unused_keys().size() == 0);
   }
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, c});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 0);
      BOOST_TEST(checker.unused_keys().size() == 2);
   }
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2);
      BOOST_TEST(checker.used_keys().count(a) == 1);
      BOOST_TEST(checker.used_keys().count(b) == 1);
      BOOST_TEST(checker.unused_keys().size() == 1);
      BOOST_TEST(checker.unused_keys().count(c) == 1);
   }
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {b, c});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 0);
   }

   A = authority(3, {key_weight{a, 1}, key_weight{b, 1}, key_weight{c, 1}});
   BOOST_TEST(make_auth_checker(GetNullAuthority, 2, {c, b, a}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetNullAuthority, 2, {a, b}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetNullAuthority, 2, {a, c}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetNullAuthority, 2, {b, c}).satisfied(A));

   A = authority(1, {key_weight{a, 1}, key_weight{b, 1}});
   BOOST_TEST(make_auth_checker(GetNullAuthority, 2, {a}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetNullAuthority, 2, {b}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetNullAuthority, 2, {c}).satisfied(A));

   A = authority(1, {key_weight{a, 2}, key_weight{b, 1}});
   BOOST_TEST(make_auth_checker(GetNullAuthority, 2, {a}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetNullAuthority, 2, {b}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetNullAuthority, 2, {c}).satisfied(A));

   auto GetCAuthority = [c](auto){
      return authority(1, {key_weight{c, 1}});
   };

   A = authority(2, {key_weight{a, 2}, key_weight{b, 1}}, {permission_level_weight{{"hello",  "world"}, 1}});
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {b});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(checker.used_keys().size() == 0);
      BOOST_TEST(checker.unused_keys().size() == 1);
      BOOST_TEST(checker.unused_keys().count(b) == 1);
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {c});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(checker.used_keys().size() == 0);
      BOOST_TEST(checker.unused_keys().size() == 1);
      BOOST_TEST(checker.unused_keys().count(c) == 1);
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2);
      BOOST_TEST(checker.unused_keys().size() == 0);
      BOOST_TEST(checker.used_keys().count(b) == 1);
      BOOST_TEST(checker.used_keys().count(c) == 1);
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {b, c, a});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 1);
      BOOST_TEST(checker.used_keys().count(a) == 1);
      BOOST_TEST(checker.unused_keys().size() == 2);
      BOOST_TEST(checker.unused_keys().count(b) == 1);
      BOOST_TEST(checker.unused_keys().count(c) == 1);
   }

   A = authority(2, {key_weight{a, 1}, key_weight{b, 1}}, {permission_level_weight{{"hello",  "world"}, 1}});
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {a}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {b}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {c}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {a, b}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {b, c}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {a, c}).satisfied(A));
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2);
      BOOST_TEST(checker.unused_keys().size() == 1);
      BOOST_TEST(checker.unused_keys().count(c) == 1);
   }

   A = authority(2, {key_weight{a, 1}, key_weight{b, 1}}, {permission_level_weight{{"hello",  "world"}, 2}});
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {a, b}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {c}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {a}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {b}).satisfied(A));
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 1);
      BOOST_TEST(checker.unused_keys().size() == 2);
      BOOST_TEST(checker.used_keys().count(c) == 1);
   }

   auto d = test.get_public_key("d", "active");
   auto e = test.get_public_key("e", "active");

   auto GetAuthority = [d, e] (const permission_level& perm) {
      if (perm.actor == "top")
         return authority(2, {key_weight{d, 1}}, {permission_level_weight{{"bottom",  "bottom"}, 1}});
      return authority{1, {{e, 1}}, {}};
   };

   A = authority(5, {key_weight{a, 2}, key_weight{b, 2}, key_weight{c, 2}}, {permission_level_weight{{"top",  "top"}, 5}});
   {
      auto checker = make_auth_checker(GetAuthority, 2, {d, e});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
   }
   {
      auto checker = make_auth_checker(GetAuthority, 2, {a, b, c, d, e});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2);
      BOOST_TEST(checker.unused_keys().size() == 3);
      BOOST_TEST(checker.used_keys().count(d) == 1);
      BOOST_TEST(checker.used_keys().count(e) == 1);
   }
   {
      auto checker = make_auth_checker(GetAuthority, 2, {a, b, c, e});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 3);
      BOOST_TEST(checker.unused_keys().size() == 1);
      BOOST_TEST(checker.used_keys().count(a) == 1);
      BOOST_TEST(checker.used_keys().count(b) == 1);
      BOOST_TEST(checker.used_keys().count(c) == 1);
   }
   BOOST_TEST(make_auth_checker(GetAuthority, 1, {a, b, c}).satisfied(A));
   // Fails due to short recursion depth limit
   BOOST_TEST(!make_auth_checker(GetAuthority, 1, {d, e}).satisfied(A));

   A = authority(2, {key_weight{c, 1}, key_weight{b, 1}, key_weight{a, 1}});
   auto B =  authority(1, {key_weight{c, 1}, key_weight{b, 1}});
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, b, c});
      BOOST_TEST(validate(A));
      BOOST_TEST(validate(B));
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.satisfied(B));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.unused_keys().count(a) == 1);
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
    BOOST_TEST(tmp[i] == words[i]);
  }

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

} // namespace eosio
