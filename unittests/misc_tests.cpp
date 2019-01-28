/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/io/json.hpp>

#include <boost/asio/thread_pool.hpp>
#include <boost/test/unit_test.hpp>

#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif

using namespace eosio::chain;
using namespace eosio::testing;

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace eosio
{
using namespace chain;
using namespace std;

static constexpr uint64_t name_suffix( uint64_t n ) {
   uint32_t remaining_bits_after_last_actual_dot = 0;
   uint32_t tmp = 0;
   for( int32_t remaining_bits = 59; remaining_bits >= 4; remaining_bits -= 5 ) { // Note: remaining_bits must remain signed integer
      // Get characters one-by-one in name in order from left to right (not including the 13th character)
      auto c = (n >> remaining_bits) & 0x1Full;
      if( !c ) { // if this character is a dot
         tmp = static_cast<uint32_t>(remaining_bits);
      } else { // if this character is not a dot
         remaining_bits_after_last_actual_dot = tmp;
      }
   }

   uint64_t thirteenth_character = n & 0x0Full;
   if( thirteenth_character ) { // if 13th character is not a dot
      remaining_bits_after_last_actual_dot = tmp;
   }

   if( remaining_bits_after_last_actual_dot == 0 ) // there is no actual dot in the name other than potentially leading dots
      return n;

   // At this point remaining_bits_after_last_actual_dot has to be within the range of 4 to 59 (and restricted to increments of 5).

   // Mask for remaining bits corresponding to characters after last actual dot, except for 4 least significant bits (corresponds to 13th character).
   uint64_t mask = (1ull << remaining_bits_after_last_actual_dot) - 16;
   uint32_t shift = 64 - remaining_bits_after_last_actual_dot;

   return ( ((n & mask) << shift) + (thirteenth_character << (shift-1)) );
}

BOOST_AUTO_TEST_SUITE(misc_tests)

BOOST_AUTO_TEST_CASE(name_suffix_tests)
{
   BOOST_CHECK_EQUAL( name{name_suffix(0)}, name{0} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abcdehijklmn))}, name{N(abcdehijklmn)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abcdehijklmn1))}, name{N(abcdehijklmn1)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abc.def))}, name{N(def)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(.abc.def))}, name{N(def)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(..abc.def))}, name{N(def)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abc..def))}, name{N(def)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abc.def.ghi))}, name{N(ghi)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(.abcdefghij))}, name{N(abcdefghij)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(.abcdefghij.1))}, name{N(1)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(a.bcdefghij))}, name{N(bcdefghij)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(a.bcdefghij.1))}, name{N(1)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(......a.b.c))}, name{N(c)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abcdefhi.123))}, name{N(123)} );
   BOOST_CHECK_EQUAL( name{name_suffix(N(abcdefhij.123))}, name{N(123)} );
}

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

// Test overflow handling in asset::from_string
BOOST_AUTO_TEST_CASE(asset_from_string_overflow)
{
   asset a;

   // precision = 19, magnitude < 2^61
   BOOST_CHECK_EXCEPTION( asset::from_string("0.1000000000000000000 CUR") , symbol_type_exception, [](const auto& e) {
      return expect_assert_message(e, "precision 19 should be <= 18");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-0.1000000000000000000 CUR") , symbol_type_exception, [](const auto& e) {
      return expect_assert_message(e, "precision 19 should be <= 18");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("1.0000000000000000000 CUR") , symbol_type_exception, [](const auto& e) {
      return expect_assert_message(e, "precision 19 should be <= 18");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-1.0000000000000000000 CUR") , symbol_type_exception, [](const auto& e) {
      return expect_assert_message(e, "precision 19 should be <= 18");
   });

   // precision = 18, magnitude < 2^58
   a = asset::from_string("0.100000000000000000 CUR");
   BOOST_CHECK_EQUAL(a.get_amount(), 100000000000000000L);
   a = asset::from_string("-0.100000000000000000 CUR");
   BOOST_CHECK_EQUAL(a.get_amount(), -100000000000000000L);

   // precision = 18, magnitude = 2^62
   BOOST_CHECK_EXCEPTION( asset::from_string("4.611686018427387904 CUR") , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-4.611686018427387904 CUR") , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("4611686018427387.904 CUR") , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-4611686018427387.904 CUR") , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

   // precision = 18, magnitude = 2^62-1
   a = asset::from_string("4.611686018427387903 CUR");
   BOOST_CHECK_EQUAL(a.get_amount(), 4611686018427387903L);
   a = asset::from_string("-4.611686018427387903 CUR");
   BOOST_CHECK_EQUAL(a.get_amount(), -4611686018427387903L);

   // precision = 0, magnitude = 2^62
   BOOST_CHECK_EXCEPTION( asset::from_string("4611686018427387904 CUR") , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-4611686018427387904 CUR") , asset_type_exception, [](const asset_type_exception& e) {
      return expect_assert_message(e, "magnitude of asset amount must be less than 2^62");
   });

   // precision = 0, magnitude = 2^62-1
   a = asset::from_string("4611686018427387903 CUR");
   BOOST_CHECK_EQUAL(a.get_amount(), 4611686018427387903L);
   a = asset::from_string("-4611686018427387903 CUR");
   BOOST_CHECK_EQUAL(a.get_amount(), -4611686018427387903L);

   // precision = 18, magnitude = 2^65
   BOOST_CHECK_EXCEPTION( asset::from_string("36.893488147419103232 CUR") , overflow_exception, [](const overflow_exception& e) {
      return true;
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-36.893488147419103232 CUR") , underflow_exception, [](const underflow_exception& e) {
      return true;
   });

   // precision = 14, magnitude > 2^76
   BOOST_CHECK_EXCEPTION( asset::from_string("1000000000.00000000000000 CUR") , overflow_exception, [](const overflow_exception& e) {
      return true;
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-1000000000.00000000000000 CUR") , underflow_exception, [](const underflow_exception& e) {
      return true;
   });

   // precision = 0, magnitude > 2^76
   BOOST_CHECK_EXCEPTION( asset::from_string("100000000000000000000000 CUR") , parse_error_exception, [](const parse_error_exception& e) {
      return expect_assert_message(e, "Couldn't parse int64_t");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-100000000000000000000000 CUR") , parse_error_exception, [](const parse_error_exception& e) {
      return expect_assert_message(e, "Couldn't parse int64_t");
   });

   // precision = 20, magnitude > 2^142
   BOOST_CHECK_EXCEPTION( asset::from_string("100000000000000000000000.00000000000000000000 CUR") , symbol_type_exception, [](const auto& e) {
      return expect_assert_message(e, "precision 20 should be <= 18");
   });
   BOOST_CHECK_EXCEPTION( asset::from_string("-100000000000000000000000.00000000000000000000 CUR") , symbol_type_exception, [](const auto& e) {
      return expect_assert_message(e, "precision 20 should be <= 18");
   });
}

struct permission_visitor {
   std::vector<permission_level> permissions;
   std::vector<size_t> size_stack;
   bool _log;

   permission_visitor(bool log = false) : _log(log) {}

   void operator()(const permission_level& permission) {
      permissions.push_back(permission);
   }

   void operator()(const permission_level& permission, bool repeat ) {}

   void push_undo() {
      if( _log )
         ilog("push_undo called");
      size_stack.push_back(permissions.size());
   }

   void pop_undo() {
      if( _log )
         ilog("pop_undo called");
      FC_ASSERT( size_stack.back() <= permissions.size() && size_stack.size() >= 1,
                 "invariant failure in test permission_visitor" );
      permissions.erase( permissions.begin() + size_stack.back(), permissions.end() );
      size_stack.pop_back();
   }

   void squash_undo() {
      if( _log )
         ilog("squash_undo called");
      FC_ASSERT( size_stack.size() >= 1, "invariant failure in test permission_visitor" );
      size_stack.pop_back();
   }

};

BOOST_AUTO_TEST_CASE(authority_checker)
{ try {
   testing::TESTER test;
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

   A = authority(3, {key_weight{a, 2}, key_weight{b, 1}}, {permission_level_weight{{"hello",  "world"}, 3}});
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 1);
      BOOST_TEST(checker.used_keys().count(c) == 1);
      BOOST_TEST(checker.unused_keys().size() == 2);
      BOOST_TEST(checker.unused_keys().count(a) == 1);
      BOOST_TEST(checker.unused_keys().count(b) == 1);
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

   BOOST_TEST(b < a);
   BOOST_TEST(b < c);
   BOOST_TEST(a < c);
   {
      // valid key order: b < a < c
      A = authority(2, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}});
      // valid key order: b < c
      auto B = authority(1, {key_weight{b, 1}, key_weight{c, 1}});
      // invalid key order: c > b
      auto C = authority(1, {key_weight{b, 1}, key_weight{c, 1}, key_weight{b, 1}});
      // invalid key order: duplicate c
      auto D = authority(1, {key_weight{b, 1}, key_weight{c, 1}, key_weight{c, 1}});
      // invalid key order: duplicate b
      auto E = authority(1, {key_weight{b, 1}, key_weight{b, 1}, key_weight{c, 1}});
      // unvalid: insufficient weight
      auto F = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}});

      auto checker = make_auth_checker(GetNullAuthority, 2, {a, b, c});
      BOOST_TEST(validate(A));
      BOOST_TEST(validate(B));
      BOOST_TEST(!validate(C));
      BOOST_TEST(!validate(D));
      BOOST_TEST(!validate(E));
      BOOST_TEST(!validate(F));

      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.unused_keys().count(b) == 1);
      BOOST_TEST(checker.unused_keys().count(a) == 1);
      BOOST_TEST(checker.unused_keys().count(c) == 1);
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.satisfied(B));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.unused_keys().count(b) == 0);
      BOOST_TEST(checker.unused_keys().count(a) == 0);
      BOOST_TEST(checker.unused_keys().count(c) == 1);
   }
   {
      auto A2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{"a",  "world"},     1},
                            permission_level_weight{{"hello",  "world"}, 1},
                            permission_level_weight{{"hi",  "world"},    1}
                          });
      auto B2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          {permission_level_weight{{"hello",  "world"}, 1}
                          });
      auto C2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{"hello",  "there"}, 1},
                            permission_level_weight{{"hello",  "world"}, 1}
                          });
      // invalid: duplicate
      auto D2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{"hello",  "world"}, 1},
                            permission_level_weight{{"hello",  "world"}, 2}
                          });
      // invalid: wrong order
      auto E2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{"hello",  "world"}, 2},
                            permission_level_weight{{"hello",  "there"}, 1}
                          });
      // invalid: wrong order
      auto F2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{"hi",  "world"}, 2},
                            permission_level_weight{{"hello",  "world"}, 1}
                          });

      // invalid: insufficient weight
      auto G2 = authority(7, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                             { permission_level_weight{{"a",  "world"},     1},
                               permission_level_weight{{"hello",  "world"}, 1},
                               permission_level_weight{{"hi",  "world"},    1}
                             });

      BOOST_TEST(validate(A2));
      BOOST_TEST(validate(B2));
      BOOST_TEST(validate(C2));
      BOOST_TEST(!validate(D2));
      BOOST_TEST(!validate(E2));
      BOOST_TEST(!validate(F2));
      BOOST_TEST(!validate(G2));
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


BOOST_AUTO_TEST_CASE(transaction_test) { try {

   testing::TESTER test;
   signed_transaction trx;

   variant pretty_trx = fc::mutable_variant_object()
      ("actions", fc::variants({
         fc::mutable_variant_object()
            ("account", "eosio")
            ("name", "reqauth")
            ("authorization", fc::variants({
               fc::mutable_variant_object()
                  ("actor", "eosio")
                  ("permission", "active")
            }))
            ("data", fc::mutable_variant_object()
               ("from", "eosio")
            )
         })
      )
      // lets also push a context free action, the multi chain test will then also include a context free action
      ("context_free_actions", fc::variants({
         fc::mutable_variant_object()
            ("account", "eosio")
            ("name", "nonce")
            ("data", fc::raw::pack(std::string("dummy")))
         })
      );

   abi_serializer::from_variant(pretty_trx, trx, test.get_resolver(), test.abi_serializer_max_time);

   test.set_transaction_headers(trx);

   trx.expiration = fc::time_point::now();
   trx.validate();
   BOOST_CHECK_EQUAL(0, trx.signatures.size());
   ((const signed_transaction &)trx).sign( test.get_private_key( config::system_account_name, "active" ), test.control->get_chain_id());
   BOOST_CHECK_EQUAL(0, trx.signatures.size());
   auto private_key = test.get_private_key( config::system_account_name, "active" );
   auto public_key = private_key.get_public_key();
   trx.sign( private_key, test.control->get_chain_id()  );
   BOOST_CHECK_EQUAL(1, trx.signatures.size());
   trx.validate();

   packed_transaction pkt(trx, packed_transaction::none);

   packed_transaction pkt2(trx, packed_transaction::zlib);

   BOOST_CHECK_EQUAL(true, trx.expiration ==  pkt.expiration());
   BOOST_CHECK_EQUAL(true, trx.expiration == pkt2.expiration());

   BOOST_CHECK_EQUAL(trx.id(), pkt.id());
   BOOST_CHECK_EQUAL(trx.id(), pkt2.id());

   bytes raw = pkt.get_raw_transaction();
   bytes raw2 = pkt2.get_raw_transaction();
   BOOST_CHECK_EQUAL(raw.size(), raw2.size());
   BOOST_CHECK_EQUAL(true, std::equal(raw.begin(), raw.end(), raw2.begin()));

   BOOST_CHECK_EQUAL(pkt.get_signed_transaction().id(), pkt2.get_signed_transaction().id());
   BOOST_CHECK_EQUAL(pkt.get_signed_transaction().id(), pkt2.id());

   flat_set<public_key_type> keys;
   auto cpu_time1 = pkt.get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);
   BOOST_CHECK_EQUAL(1, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());
   keys.clear();
   auto cpu_time2 = pkt.get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);
   BOOST_CHECK_EQUAL(1, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());

   BOOST_CHECK(cpu_time1 > fc::microseconds(0));
   BOOST_CHECK(cpu_time2 > fc::microseconds(0));

   // verify that hitting cache still indicates same billable time
   // if we remove cache so that the second is a real time calculation then remove this check
   BOOST_CHECK(cpu_time1 == cpu_time2);

   // pack
   uint32_t pack_size = fc::raw::pack_size( pkt );
   vector<char> buf(pack_size);
   fc::datastream<char*> ds(buf.data(), pack_size);
   fc::raw::pack( ds, pkt );
   // unpack
   ds.seekp(0);
   packed_transaction pkt3;
   fc::raw::unpack(ds, pkt3);
   // pack again
   pack_size = fc::raw::pack_size( pkt3 );
   fc::datastream<char*> ds2(buf.data(), pack_size);
   fc::raw::pack( ds2, pkt3 );
   // unpack
   ds2.seekp(0);
   packed_transaction pkt4;
   fc::raw::unpack(ds2, pkt4);

   bytes raw3 = pkt3.get_raw_transaction();
   bytes raw4 = pkt4.get_raw_transaction();
   BOOST_CHECK_EQUAL(raw.size(), raw3.size());
   BOOST_CHECK_EQUAL(raw3.size(), raw4.size());
   BOOST_CHECK_EQUAL(true, std::equal(raw.begin(), raw.end(), raw3.begin()));
   BOOST_CHECK_EQUAL(true, std::equal(raw.begin(), raw.end(), raw4.begin()));
   BOOST_CHECK_EQUAL(pkt.get_signed_transaction().id(), pkt3.get_signed_transaction().id());
   BOOST_CHECK_EQUAL(pkt.get_signed_transaction().id(), pkt4.get_signed_transaction().id());
   BOOST_CHECK_EQUAL(pkt.id(), pkt4.get_signed_transaction().id());
   BOOST_CHECK_EQUAL(true, trx.expiration == pkt4.expiration());
   BOOST_CHECK_EQUAL(true, trx.expiration == pkt4.get_signed_transaction().expiration);
   keys.clear();
   auto cpu_time3 = pkt4.get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::maximum(), keys);
   BOOST_CHECK_EQUAL(1, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(transaction_metadata_test) { try {

   testing::TESTER test;
   signed_transaction trx;

   variant pretty_trx = fc::mutable_variant_object()
      ("actions", fc::variants({
         fc::mutable_variant_object()
            ("account", "eosio")
            ("name", "reqauth")
            ("authorization", fc::variants({
               fc::mutable_variant_object()
                  ("actor", "eosio")
                  ("permission", "active")
            }))
            ("data", fc::mutable_variant_object()
               ("from", "eosio")
            )
         })
      )
      ("context_free_actions", fc::variants({
         fc::mutable_variant_object()
            ("account", "eosio")
            ("name", "nonce")
            ("data", fc::raw::pack(std::string("dummy data")))
         })
      );

      abi_serializer::from_variant(pretty_trx, trx, test.get_resolver(), test.abi_serializer_max_time);

      test.set_transaction_headers(trx);
      trx.expiration = fc::time_point::now();

      auto private_key = test.get_private_key( config::system_account_name, "active" );
      auto public_key = private_key.get_public_key();
      trx.sign( private_key, test.control->get_chain_id()  );
      BOOST_CHECK_EQUAL(1, trx.signatures.size());

      packed_transaction pkt(trx, packed_transaction::none);
      packed_transaction pkt2(trx, packed_transaction::zlib);

      transaction_metadata_ptr mtrx = std::make_shared<transaction_metadata>( std::make_shared<packed_transaction>( trx, packed_transaction::none) );
      transaction_metadata_ptr mtrx2 = std::make_shared<transaction_metadata>( std::make_shared<packed_transaction>( trx, packed_transaction::zlib) );

      BOOST_CHECK_EQUAL(trx.id(), pkt.id());
      BOOST_CHECK_EQUAL(trx.id(), pkt2.id());
      BOOST_CHECK_EQUAL(trx.id(), mtrx->id);
      BOOST_CHECK_EQUAL(trx.id(), mtrx2->id);

      boost::asio::thread_pool thread_pool(5);

      BOOST_CHECK( !mtrx->signing_keys_future.valid() );
      BOOST_CHECK( !mtrx2->signing_keys_future.valid() );

      transaction_metadata::create_signing_keys_future( mtrx, thread_pool, test.control->get_chain_id(), fc::microseconds::maximum() );
      transaction_metadata::create_signing_keys_future( mtrx2, thread_pool, test.control->get_chain_id(), fc::microseconds::maximum() );

      BOOST_CHECK( mtrx->signing_keys_future.valid() );
      BOOST_CHECK( mtrx2->signing_keys_future.valid() );

      // no-op
      transaction_metadata::create_signing_keys_future( mtrx, thread_pool, test.control->get_chain_id(), fc::microseconds::maximum() );
      transaction_metadata::create_signing_keys_future( mtrx2, thread_pool, test.control->get_chain_id(), fc::microseconds::maximum() );

      auto keys = mtrx->recover_keys( test.control->get_chain_id() );
      BOOST_CHECK_EQUAL(1, keys.size());
      BOOST_CHECK_EQUAL(public_key, *keys.begin());

      // again
      keys = mtrx->recover_keys( test.control->get_chain_id() );
      BOOST_CHECK_EQUAL(1, keys.size());
      BOOST_CHECK_EQUAL(public_key, *keys.begin());

      auto keys2 = mtrx2->recover_keys( test.control->get_chain_id() );
      BOOST_CHECK_EQUAL(1, keys.size());
      BOOST_CHECK_EQUAL(public_key, *keys.begin());


} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_SUITE_END()

} // namespace eosio
