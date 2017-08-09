#include <eos/chain/BlockchainConfiguration.hpp>
#include <eos/chain/authority.hpp>

#include <eos/utilities/key_conversion.hpp>
#include <eos/utilities/rand.hpp>
#include <eos/wallet/wallet.hpp>

#include <fc/io/json.hpp>

#include <boost/test/unit_test.hpp>

#include "../common/testing_macros.hpp"

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

   auto GetNullAuthority = [](auto){return Authority();};

   auto A = Complex_Authority(2, ((a,1))((b,1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {a, b}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {a, b, c}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {a, c}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {b, c}).satisfied(A));

   A = Complex_Authority(3, ((a,1))((b,1))((c,1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {c, b, a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {a, b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {a, c}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {b, c}).satisfied(A));

   A = Complex_Authority(1, ((a, 1))((b, 1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {a}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {c}).satisfied(A));

   A = Complex_Authority(1, ((a, 2))((b, 1)),);
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {a}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetNullAuthority, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetNullAuthority, {c}).satisfied(A));

   auto GetCAuthority = [c_public_key](auto){return Complex_Authority(1, ((c, 1)),);};

   A = Complex_Authority(2, ((a, 2))((b, 1)), (("hello", "world", 1)));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {c}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {b,c}).satisfied(A));

   A = Complex_Authority(2, ((a, 1))((b, 1)), (("hello", "world", 1)));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {b}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {c}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {a,b}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {b,c}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {a,c}).satisfied(A));

   A = Complex_Authority(2, ((a, 1))((b, 1)), (("hello", "world", 2)));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {a,b}).satisfied(A));
   BOOST_CHECK(MakeAuthorityChecker(GetCAuthority, {c}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {a}).satisfied(A));
   BOOST_CHECK(!MakeAuthorityChecker(GetCAuthority, {b}).satisfied(A));
} FC_LOG_AND_RETHROW() }

/// Test creating the wallet
BOOST_AUTO_TEST_CASE(wallet_test)
{ try {
   using namespace eos::wallet;
   using namespace eos::utilities;

   wallet_data d;
   d.ws_server = "test_server";
   d.ws_port = 99;
   d.ws_user = "bob";
   d.ws_password = "user_pwd";

   wallet_api wallet(d);
   BOOST_CHECK(wallet.is_locked());

   wallet.set_password("pass");
   BOOST_CHECK(wallet.is_locked());

   wallet.unlock("pass");
   BOOST_CHECK(!wallet.is_locked());

   BOOST_CHECK_EQUAL(0, wallet.list_keys().size());

   auto priv = fc::ecc::private_key::generate();
   auto pub = public_key_type( priv.get_public_key() );
   auto wif = key_to_wif(priv.get_secret());
   wallet.import_key(wif);
   BOOST_CHECK_EQUAL(1, wallet.list_keys().size());

   auto privCopy = wallet.get_private_key(pub);
   BOOST_CHECK_EQUAL(wif, privCopy);

   wallet.lock();
   BOOST_CHECK(wallet.is_locked());
   wallet.unlock("pass");
   BOOST_CHECK_EQUAL(1, wallet.list_keys().size());
   wallet.save_wallet_file("wallet_test.json");

   wallet_data d2;
   wallet_api wallet2(d2);

   BOOST_CHECK(wallet2.is_locked());
   wallet2.load_wallet_file("wallet_test.json");
   BOOST_CHECK(wallet2.is_locked());

   wallet2.unlock("pass");
   BOOST_CHECK_EQUAL(1, wallet2.list_keys().size());

   auto privCopy2 = wallet2.get_private_key(pub);
   BOOST_CHECK_EQUAL(wif, privCopy2);
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()

} // namespace eos
