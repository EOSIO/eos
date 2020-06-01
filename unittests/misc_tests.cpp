#include <eosio/chain/asset.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/authority_checker.hpp>
#include <eosio/chain/chain_config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/thread_utils.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/io/json.hpp>
#include <fc/log/logger_config.hpp>
#include <appbase/execution_priority_queue.hpp>
#include <fc/bitutil.hpp>

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

struct base_reflect : fc::reflect_init {
   int bv = 0;
   bool base_reflect_initialized = false;
   int base_reflect_called = 0;
protected:
   friend struct fc::reflector<base_reflect>;
   friend struct fc::reflector_init_visitor<base_reflect>;
   friend struct fc::has_reflector_init<base_reflect>;
   void reflector_init() {
      BOOST_CHECK_EQUAL( bv, 42 ); // should be deserialized before called, set by test
      ++base_reflect_called;
      base_reflect_initialized = true;
   }
};

struct derived_reflect : public base_reflect {
   int dv = 0;
   bool derived_reflect_initialized = false;
   int derived_reflect_called = 0;
protected:
   friend struct fc::reflector<derived_reflect>;
   friend struct fc::reflector_init_visitor<derived_reflect>;
   friend struct fc::has_reflector_init<derived_reflect>;
   void reflector_init() {
      BOOST_CHECK_EQUAL( bv, 42 ); // should be deserialized before called, set by test
      BOOST_CHECK_EQUAL( dv, 52 ); // should be deserialized before called, set by test
      ++derived_reflect_called;
      base_reflect::reflector_init();
      derived_reflect_initialized = true;
   }
};

struct final_reflect : public derived_reflect {
   int fv = 0;
   bool final_reflect_initialized = false;
   int final_reflect_called = 0;
private:
   friend struct fc::reflector<final_reflect>;
   friend struct fc::reflector_init_visitor<final_reflect>;
   friend struct fc::has_reflector_init<final_reflect>;
   void reflector_init() {
      BOOST_CHECK_EQUAL( bv, 42 ); // should be deserialized before called, set by test
      BOOST_CHECK_EQUAL( dv, 52 ); // should be deserialized before called, set by test
      BOOST_CHECK_EQUAL( fv, 62 ); // should be deserialized before called, set by test
      ++final_reflect_called;
      derived_reflect::reflector_init();
      final_reflect_initialized = true;
   }
};

FC_REFLECT( base_reflect, (bv) )
FC_REFLECT_DERIVED( derived_reflect, (base_reflect), (dv) )
FC_REFLECT_DERIVED( final_reflect, (derived_reflect), (fv) )

namespace eosio
{
using namespace chain;
using namespace std;

static constexpr uint64_t name_suffix( name nv ) {
   uint64_t n = nv.to_uint64_t();
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

BOOST_AUTO_TEST_CASE(reverse_endian_tests)
{
    BOOST_CHECK_EQUAL( endian_reverse_u64(0x0123456789abcdef), 0xefcdab8967452301 );
    BOOST_CHECK_EQUAL( endian_reverse_u64(0x0102030405060708), 0x0807060504030201 );
    BOOST_CHECK_EQUAL( endian_reverse_u32(0x01234567), 0x67452301 );
    BOOST_CHECK_EQUAL( endian_reverse_u32(0x01020304), 0x04030201 );
}

BOOST_AUTO_TEST_CASE(name_suffix_tests)
{
   BOOST_CHECK_EQUAL( name{name_suffix(name(0))}, name{0} );
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

BOOST_AUTO_TEST_CASE(variant_format_string_limited)
{
   const string format = "${a} ${b} ${c}";
   {
      fc::mutable_variant_object mu;
      mu( "a", string( 1024, 'a' ) );
      mu( "b", string( 1024, 'b' ) );
      mu( "c", string( 1024, 'c' ) );
      string result = fc::format_string( format, mu, true );
      BOOST_CHECK_LT(0, mu.size());
      BOOST_CHECK_LT(format.size(), 1024);
      const size_t target_size = (1024 -  format.size()) / mu.size();
      BOOST_CHECK_EQUAL( result, string( target_size, 'a' ) + "... " + string( target_size, 'b' ) + "... " + string( target_size, 'c' ) + "..." );
   }
   {
      fc::mutable_variant_object mu;
      signed_block a;
      blob b;
      for( int i = 0; i < 1024; ++i)
         b.data.push_back('b');
      variants c;
      c.push_back(variant(a));
      mu( "a", a );
      mu( "b", b );
      mu( "c", c );
      string result = fc::format_string( format, mu, true );
      BOOST_CHECK_EQUAL( result, "${a} ${b} ${c}");
   }
   {
      // test cases for issue #8741, short version, all fields being displayed
      flat_set <permission_level> provided_permissions;
      for(char ch = 'a'; ch < 'c'; ++ch) {
         provided_permissions.insert( {name(std::string_view(string(4, ch))), name(std::string_view(string(4, ch + 1)))});
      }
      flat_set <public_key_type> provided_keys;
      auto fill_keys = [](const flat_set <permission_level>& provided_permissions, flat_set <public_key_type>& provided_keys) {
         std::string digest = "1234567";
         for (auto &permission : provided_permissions) {
            digest += "1";
            const std::string key_name_str = permission.actor.to_string() + permission.permission.to_string();
            auto sig_digest = digest_type::hash(std::make_pair("1234", "abcd"));
            const fc::crypto::signature sig = private_key_type::regenerate<fc::ecc::private_key_shim>(
                  fc::sha256::hash(key_name_str + "active")).sign(sig_digest);
            provided_keys.insert(public_key_type{sig, fc::sha256{digest}, true});
         }
      };
      fill_keys(provided_permissions, provided_keys);
      const string format = "transaction declares authority '${auth}', provided permissions ${provided_permissions}, provided keys ${provided_keys}";
      fc::mutable_variant_object mu;
      mu("auth", *provided_permissions.begin());
      mu("provided_permissions", provided_permissions);
      mu("provided_keys", provided_keys);
      BOOST_CHECK_LT(0, mu.size());
      const auto arg_limit_size = (1024 - format.size()) / mu.size();
      const string result = fc::format_string(format, mu, true);
      BOOST_CHECK(provided_permissions.begin() != provided_permissions.end());
      const string auth_str = fc::json::to_string(*provided_permissions.begin(), fc::time_point::max());
      string target_str = "transaction declares authority '" + fc::json::to_string(*provided_permissions.begin(), fc::time_point::max());
      target_str += "', provided permissions " + fc::json::to_string(provided_permissions, fc::time_point::max());
      target_str += ", provided keys " + fc::json::to_string(provided_keys, fc::time_point::max()).substr(0, arg_limit_size);
      BOOST_CHECK_EQUAL(result, target_str);
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());

      // test cases for issue #8741, longer version, permission and keys field being folded
      provided_permissions.clear();
      provided_keys.clear();
      for(char ch = 'c'; ch < 'z'; ++ch) {
         provided_permissions.insert( {name(std::string_view(string(5, ch))), name(std::string_view(string(5, ch + 1)))});
      }
      fill_keys(provided_permissions, provided_keys);
      fc::mutable_variant_object mu_fold;
      mu_fold("auth", *provided_permissions.begin());
      mu_fold("provided_permissions", provided_permissions);
      mu_fold("provided_keys", provided_keys);
      BOOST_CHECK_LT(0, mu_fold.size());
      string target_fold_str = "transaction declares authority '" + fc::json::to_string(*provided_permissions.begin(), fc::time_point::max());
      target_fold_str += "', provided permissions ${provided_permissions}";
      target_fold_str += ", provided keys ${provided_keys}";
      const string result_fold = fc::format_string(format, mu_fold, true);
      BOOST_CHECK_EQUAL(result_fold, target_fold_str);
      BOOST_CHECK_LT(result_fold.size(), 1024 + 3 * mu.size());
   }
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
   auto a = test.get_public_key(name("a"), "active");
   auto b = test.get_public_key(name("b"), "active");
   auto c = test.get_public_key(name("c"), "active");

   auto GetNullAuthority = [](auto){abort(); return authority();};

   auto A = authority(2, {key_weight{a, 1}, key_weight{b, 1}});
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, b});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2u);
      BOOST_TEST(checker.unused_keys().size() == 0u);
   }
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, c});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 0u);
      BOOST_TEST(checker.unused_keys().size() == 2u);
   }
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2u);
      BOOST_TEST(checker.used_keys().count(a) == 1u);
      BOOST_TEST(checker.used_keys().count(b) == 1u);
      BOOST_TEST(checker.unused_keys().size() == 1u);
      BOOST_TEST(checker.unused_keys().count(c) == 1u);
   }
   {
      auto checker = make_auth_checker(GetNullAuthority, 2, {b, c});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 0u);
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

   A = authority(2, {key_weight{a, 2}, key_weight{b, 1}}, {permission_level_weight{{name("hello"), name("world")}, 1}});
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {b});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(checker.used_keys().size() == 0u);
      BOOST_TEST(checker.unused_keys().size() == 1u);
      BOOST_TEST(checker.unused_keys().count(b) == 1u);
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {c});
      BOOST_TEST(!checker.satisfied(A));
      BOOST_TEST(checker.used_keys().size() == 0u);
      BOOST_TEST(checker.unused_keys().size() == 1u);
      BOOST_TEST(checker.unused_keys().count(c) == 1u);
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2u);
      BOOST_TEST(checker.unused_keys().size() == 0u);
      BOOST_TEST(checker.used_keys().count(b) == 1u);
      BOOST_TEST(checker.used_keys().count(c) == 1u);
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {b, c, a});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 1u);
      BOOST_TEST(checker.used_keys().count(a) == 1u);
      BOOST_TEST(checker.unused_keys().size() == 2u);
      BOOST_TEST(checker.unused_keys().count(b) == 1u);
      BOOST_TEST(checker.unused_keys().count(c) == 1u);
   }

   A = authority(3, {key_weight{a, 2}, key_weight{b, 1}}, {permission_level_weight{{name("hello"), name("world")}, 3}});
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
   }
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 1u);
      BOOST_TEST(checker.used_keys().count(c) == 1u);
      BOOST_TEST(checker.unused_keys().size() == 2u);
      BOOST_TEST(checker.unused_keys().count(a) == 1u);
      BOOST_TEST(checker.unused_keys().count(b) == 1u);
   }

   A = authority(2, {key_weight{a, 1}, key_weight{b, 1}}, {permission_level_weight{{name("hello"), name("world")}, 1}});
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
      BOOST_TEST(checker.used_keys().size() == 2u);
      BOOST_TEST(checker.unused_keys().size() == 1u);
      BOOST_TEST(checker.unused_keys().count(c) == 1u);
   }

   A = authority(2, {key_weight{a, 1}, key_weight{b, 1}}, {permission_level_weight{{name("hello"), name("world")}, 2}});
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {a, b}).satisfied(A));
   BOOST_TEST(make_auth_checker(GetCAuthority, 2, {c}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {a}).satisfied(A));
   BOOST_TEST(!make_auth_checker(GetCAuthority, 2, {b}).satisfied(A));
   {
      auto checker = make_auth_checker(GetCAuthority, 2, {a, b, c});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 1u);
      BOOST_TEST(checker.unused_keys().size() == 2u);
      BOOST_TEST(checker.used_keys().count(c) == 1u);
   }

   auto d = test.get_public_key(name("d"), "active");
   auto e = test.get_public_key(name("e"), "active");

   auto GetAuthority = [d, e] (const permission_level& perm) {
      if (perm.actor == name("top"))
         return authority(2, {key_weight{d, 1}}, {permission_level_weight{{name("bottom"), name("bottom")}, 1}});
      return authority{1, {{e, 1}}, {}};
   };

   A = authority(5, {key_weight{a, 2}, key_weight{b, 2}, key_weight{c, 2}}, {permission_level_weight{{name("top"), name("top")}, 5}});
   {
      auto checker = make_auth_checker(GetAuthority, 2, {d, e});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.all_keys_used());
   }
   {
      auto checker = make_auth_checker(GetAuthority, 2, {a, b, c, d, e});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 2u);
      BOOST_TEST(checker.unused_keys().size() == 3u);
      BOOST_TEST(checker.used_keys().count(d) == 1u);
      BOOST_TEST(checker.used_keys().count(e) == 1u);
   }
   {
      auto checker = make_auth_checker(GetAuthority, 2, {a, b, c, e});
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.used_keys().size() == 3u);
      BOOST_TEST(checker.unused_keys().size() == 1u);
      BOOST_TEST(checker.used_keys().count(a) == 1u);
      BOOST_TEST(checker.used_keys().count(b) == 1u);
      BOOST_TEST(checker.used_keys().count(c) == 1u);
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
      BOOST_TEST(checker.unused_keys().count(b) == 1u);
      BOOST_TEST(checker.unused_keys().count(a) == 1u);
      BOOST_TEST(checker.unused_keys().count(c) == 1u);
      BOOST_TEST(checker.satisfied(A));
      BOOST_TEST(checker.satisfied(B));
      BOOST_TEST(!checker.all_keys_used());
      BOOST_TEST(checker.unused_keys().count(b) == 0u);
      BOOST_TEST(checker.unused_keys().count(a) == 0u);
      BOOST_TEST(checker.unused_keys().count(c) == 1u);
   }
   {
      auto A2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{name("a"), name("world")},     1},
                            permission_level_weight{{name("hello"), name("world")}, 1},
                            permission_level_weight{{name("hi"), name("world")},    1}
                          });
      auto B2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          {permission_level_weight{{name("hello"), name("world")}, 1}
                          });
      auto C2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{name("hello"), name("there")}, 1},
                            permission_level_weight{{name("hello"), name("world")}, 1}
                          });
      // invalid: duplicate
      auto D2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{name("hello"), name("world")}, 1},
                            permission_level_weight{{name("hello"), name("world")}, 2}
                          });
      // invalid: wrong order
      auto E2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{name("hello"), name("world")}, 2},
                            permission_level_weight{{name("hello"), name("there")}, 1}
                          });
      // invalid: wrong order
      auto F2 = authority(4, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                          { permission_level_weight{{name("hi"), name("world")}, 2},
                            permission_level_weight{{name("hello"), name("world")}, 1}
                          });

      // invalid: insufficient weight
      auto G2 = authority(7, {key_weight{b, 1}, key_weight{a, 1}, key_weight{c, 1}},
                             { permission_level_weight{{name("a"), name("world")},     1},
                               permission_level_weight{{name("hello"), name("world")}, 1},
                               permission_level_weight{{name("hi"), name("world")},    1}
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
    uwords.push_back(n.to_uint64_t());
  }

  std::sort(uwords.begin(), uwords.end(), std::less<uint64_t>());

  vector<string> tmp;
  for(const auto uw: uwords) {
    auto str = name(uw).to_string();
    tmp.push_back(str);
  }

  for(size_t i = 0; i < words.size(); ++i ) {
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
      )
      ("context_free_data", vector<bytes>{{'d','u','m','m','y',' ','d','a','t','a'}});

   abi_serializer::from_variant(pretty_trx, trx, test.get_resolver(), abi_serializer::create_yield_function( test.abi_serializer_max_time ));

   test.set_transaction_headers(trx);

   trx.expiration = fc::now<fc::seconds>();
   trx.validate();
   BOOST_CHECK_EQUAL(0u, trx.signatures.size());
   ((const signed_transaction &)trx).sign( test.get_private_key( config::system_account_name, "active" ), test.control->get_chain_id());
   BOOST_CHECK_EQUAL(0u, trx.signatures.size());
   auto private_key = test.get_private_key( config::system_account_name, "active" );
   auto public_key = private_key.get_public_key();
   trx.sign( private_key, test.control->get_chain_id()  );
   BOOST_CHECK_EQUAL(1u, trx.signatures.size());
   trx.validate();

   packed_transaction pkt(signed_transaction(trx), true, packed_transaction::compression_type::none);

   packed_transaction pkt2(signed_transaction(trx), true, packed_transaction::compression_type::zlib);

   BOOST_CHECK_EQUAL(true, trx.expiration ==  pkt.expiration());
   BOOST_CHECK_EQUAL(true, trx.expiration == pkt2.expiration());

   BOOST_CHECK_EQUAL(trx.id(), pkt.id());
   BOOST_CHECK_EQUAL(trx.id(), pkt2.id());

   BOOST_CHECK_EQUAL(pkt.to_packed_transaction_v0()->get_signed_transaction().id(), pkt2.to_packed_transaction_v0()->get_signed_transaction().id());
   BOOST_CHECK_EQUAL(pkt.to_packed_transaction_v0()->get_transaction().id(), pkt2.id());
   BOOST_CHECK_EQUAL(pkt.get_transaction().id(), pkt2.id());
   BOOST_CHECK_EQUAL(pkt.to_packed_transaction_v0()->get_prunable_size(), pkt.get_prunable_size());
   BOOST_CHECK_EQUAL(pkt.to_packed_transaction_v0()->get_unprunable_size(), pkt.get_unprunable_size());

   flat_set<public_key_type> keys;
   auto cpu_time1 = pkt.to_packed_transaction_v0()->get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::max(), keys);
   BOOST_CHECK_EQUAL(1u, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());
   keys.clear();
   auto cpu_time2 = pkt.to_packed_transaction_v0()->get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::max(), keys);
   BOOST_CHECK_EQUAL(1u, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());

   BOOST_CHECK(cpu_time1 > fc::microseconds(0));
   BOOST_CHECK(cpu_time2 > fc::microseconds(0));

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
   // to/from variant
   fc::variant pkt_v( pkt3 );
   packed_transaction pkt5;
   fc::from_variant(pkt_v, pkt5);

   BOOST_CHECK_EQUAL(pkt.get_transaction().id(), pkt3.get_transaction().id());
   BOOST_CHECK_EQUAL(pkt.get_transaction().id(), pkt4.get_transaction().id());
   BOOST_CHECK_EQUAL(pkt.get_transaction().id(), pkt5.get_transaction().id()); // failure indicates reflector_init not working
   BOOST_CHECK_EQUAL(pkt.id(), pkt4.get_transaction().id());
   BOOST_CHECK_EQUAL(true, trx.expiration == pkt4.expiration());
   BOOST_CHECK_EQUAL(true, trx.expiration == pkt4.get_transaction().expiration);
   BOOST_REQUIRE(pkt.get_context_free_data() != nullptr);
   BOOST_REQUIRE(pkt4.get_context_free_data() != nullptr);
   BOOST_REQUIRE_EQUAL(pkt.get_context_free_data()->size(), pkt4.get_context_free_data()->size());
   BOOST_CHECK(std::equal(pkt.get_context_free_data()->begin(), pkt.get_context_free_data()->end(), pkt4.get_context_free_data()->begin()));
   keys.clear();
   pkt4.to_packed_transaction_v0()->get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::max(), keys);
   BOOST_CHECK_EQUAL(1u, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());

   packed_transaction pkt6(*pkt.to_packed_transaction_v0(), true);
   BOOST_CHECK_EQUAL(pkt.id(), pkt6.id());
   BOOST_CHECK(pkt6.get_estimated_size() > 0);
   keys.clear();
   pkt6.to_packed_transaction_v0()->get_signed_transaction().get_signature_keys(test.control->get_chain_id(), fc::time_point::max(), keys);
   BOOST_CHECK_EQUAL(1u, keys.size());
   BOOST_CHECK_EQUAL(public_key, *keys.begin());

} FC_LOG_AND_RETHROW() }


BOOST_AUTO_TEST_CASE(signed_int_test) { try {
    char buf[32];
    fc::datastream<char*> ds(buf,32);
    signed_int a(47), b((1<<30)+2), c(-47), d(-(1<<30)-2); //small +, big +, small -, big -
    signed_int ee;
    fc::raw::pack(ds,a);
    ds.seekp(0);
    fc::raw::unpack(ds,ee);
    ds.seekp(0);
    BOOST_CHECK_EQUAL(a,ee);
    fc::raw::pack(ds,b);
    ds.seekp(0);
    fc::raw::unpack(ds,ee);
    ds.seekp(0);
    BOOST_CHECK_EQUAL(b,ee);
    fc::raw::pack(ds,c);
    ds.seekp(0);
    fc::raw::unpack(ds,ee);
    ds.seekp(0);
    BOOST_CHECK_EQUAL(c,ee);
    fc::raw::pack(ds,d);
    ds.seekp(0);
    fc::raw::unpack(ds,ee);
    ds.seekp(0);
    BOOST_CHECK_EQUAL(d,ee);

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
      )
      ("context_free_data", vector<bytes>{{'d','u','m','m','y',' ','d','a','t','a'}});

      abi_serializer::from_variant(pretty_trx, trx, test.get_resolver(), abi_serializer::create_yield_function( test.abi_serializer_max_time ));

      test.set_transaction_headers(trx);
      trx.expiration = fc::now<fc::seconds>();

      auto private_key = test.get_private_key( config::system_account_name, "active" );
      auto public_key = private_key.get_public_key();
      trx.sign( private_key, test.control->get_chain_id()  );
      BOOST_CHECK_EQUAL(1u, trx.signatures.size());

      packed_transaction pkt(signed_transaction(trx), true, packed_transaction::compression_type::none);
      packed_transaction pkt2(signed_transaction(trx), true, packed_transaction::compression_type::zlib);

      packed_transaction_ptr ptrx = std::make_shared<packed_transaction>( signed_transaction(trx), true, packed_transaction::compression_type::none);
      packed_transaction_ptr ptrx2 = std::make_shared<packed_transaction>( signed_transaction(trx), true, packed_transaction::compression_type::zlib);

      BOOST_CHECK_EQUAL(trx.id(), pkt.id());
      BOOST_CHECK_EQUAL(trx.id(), pkt2.id());
      BOOST_CHECK_EQUAL(trx.id(), ptrx->id());
      BOOST_CHECK_EQUAL(trx.id(), ptrx2->id());

      named_thread_pool thread_pool( "misc", 5 );

      auto fut = transaction_metadata::start_recover_keys( ptrx, thread_pool.get_executor(), test.control->get_chain_id(), fc::microseconds::max() );
      auto fut2 = transaction_metadata::start_recover_keys( ptrx2, thread_pool.get_executor(), test.control->get_chain_id(), fc::microseconds::max() );

      // start another key reovery on same packed_transaction, creates a new future with transaction_metadata, should not interfere with above
      transaction_metadata::start_recover_keys( ptrx, thread_pool.get_executor(), test.control->get_chain_id(), fc::microseconds::max() );
      transaction_metadata::start_recover_keys( ptrx2, thread_pool.get_executor(), test.control->get_chain_id(), fc::microseconds::max() );

      auto mtrx = fut.get();
      const auto& keys = mtrx->recovered_keys();
      BOOST_CHECK_EQUAL(1u, keys.size());
      BOOST_CHECK_EQUAL(public_key, *keys.begin());

      // again, can be called multiple times, current implementation it is just an attribute of transaction_metadata
      const auto& keys2 = mtrx->recovered_keys();
      BOOST_CHECK_EQUAL(1u, keys2.size());
      BOOST_CHECK_EQUAL(public_key, *keys2.begin());

      auto mtrx2 = fut2.get();
      const auto& keys3 = mtrx2->recovered_keys();
      BOOST_CHECK_EQUAL(1u, keys3.size());
      BOOST_CHECK_EQUAL(public_key, *keys3.begin());

      thread_pool.stop();

} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_CASE(prunable_transaction_data_test) {
   {
      packed_transaction::prunable_data_type basic{packed_transaction::prunable_data_type::full_legacy{{}, {}}};
      packed_transaction::prunable_data_type pruned = packed_transaction::prunable_data_type( basic).prune_all();
      BOOST_TEST( basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none) >=
                  pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(basic) <= basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(pruned) <= pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(basic.digest().str() == pruned.digest().str());
   }

   bytes large_bytes(48);
   {
      packed_transaction::prunable_data_type basic{packed_transaction::prunable_data_type::full_legacy{{}, fc::raw::pack( std::vector( 4, large_bytes))}};
      packed_transaction::prunable_data_type pruned = packed_transaction::prunable_data_type(basic).prune_all();
      BOOST_TEST( basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none) >=
                  pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(basic) <= basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(pruned) <= pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(basic.digest().str() == pruned.digest().str());
   }

   {
      packed_transaction::prunable_data_type basic{packed_transaction::prunable_data_type::full_legacy{std::vector( 4, signature_type()), fc::raw::pack( std::vector( 4, large_bytes))}};
      packed_transaction::prunable_data_type pruned = packed_transaction::prunable_data_type(basic).prune_all();
      BOOST_TEST( basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none) >=
                  pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(basic) <= basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(pruned) <= pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(basic.digest().str() == pruned.digest().str());
   }

   {
      packed_transaction::prunable_data_type basic{packed_transaction::prunable_data_type::full_legacy{std::vector( 4, signature_type()), {}}};
      packed_transaction::prunable_data_type pruned = packed_transaction::prunable_data_type(basic).prune_all();
      BOOST_TEST( basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none) >=
                  pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(basic) <= basic.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(fc::raw::pack_size(pruned) <= pruned.maximum_pruned_pack_size( packed_transaction::prunable_data_type::compression_type::none));
      BOOST_TEST(basic.digest().str() == pruned.digest().str());
   }
}

BOOST_AUTO_TEST_CASE(pruned_transaction_test) {
   tester t;
   signed_transaction trx;
   trx.context_free_actions.push_back({ {}, N(eosio), N(), bytes() });
   trx.context_free_data.push_back(bytes());
   t.set_transaction_headers(trx);
   trx.sign( t.get_private_key( N(eosio), "active" ), t.control->get_chain_id() );

   packed_transaction_v0 packed(trx);
   packed_transaction pruned(std::move(trx), true);
   BOOST_TEST(packed.packed_digest().str() == pruned.packed_digest().str());
   BOOST_REQUIRE(pruned.get_context_free_data() != nullptr);
   BOOST_TEST(*pruned.get_context_free_data() == packed.get_context_free_data());
   BOOST_REQUIRE(pruned.get_context_free_data(0) != nullptr);
   BOOST_TEST(*pruned.get_context_free_data(0) == bytes());
   BOOST_TEST(pruned.get_context_free_data(1) == nullptr);
   BOOST_REQUIRE(pruned.get_signatures() != nullptr);
   BOOST_TEST(*pruned.get_signatures() == packed.get_signatures());
   BOOST_TEST(pruned.get_prunable_size() == packed.get_prunable_size());
   BOOST_TEST(pruned.get_unprunable_size() == packed.get_unprunable_size());
   std::size_t max_size = pruned.maximum_pruned_pack_size(packed_transaction::cf_compression_type::none);
   BOOST_TEST(fc::raw::pack_size(pruned) <= max_size);
   BOOST_TEST(fc::raw::pack_size(pruned) < pruned.get_estimated_size());

   pruned.prune_all();
   BOOST_TEST(packed.packed_digest().str() == pruned.packed_digest().str());
   BOOST_TEST(pruned.get_context_free_data() == nullptr);
   BOOST_TEST(pruned.get_context_free_data(0) == nullptr);
   BOOST_TEST(pruned.get_context_free_data(1) == nullptr);
   BOOST_TEST(pruned.get_signatures() == nullptr);

   BOOST_TEST(fc::raw::pack_size(pruned) <= max_size);
   BOOST_CHECK_THROW(pruned.get_prunable_size(), tx_prune_exception);
}


static checksum256_type calculate_trx_merkle( const deque<transaction_receipt>& trxs ) {
   deque<digest_type> trx_digests;
   for( const auto& a : trxs )
     trx_digests.emplace_back( a.digest() );
   return merkle( move( trx_digests ) );
}

BOOST_AUTO_TEST_CASE(pruned_block_test) {
   tester t;
   signed_transaction trx;
   trx.actions.push_back({ { permission_level{ N(eosio), N(active) } }, N(eosio), N(), bytes() } );
   trx.context_free_actions.push_back({ {}, N(eosio), N(), bytes() });
   trx.context_free_data.push_back(bytes());
   t.set_transaction_headers(trx);
   trx.sign( t.get_private_key( N(eosio), "active" ), t.control->get_chain_id() );

   t.push_transaction(trx);
   signed_block_ptr produced = t.produce_block();
   auto original = produced->to_signed_block_v0();
   BOOST_REQUIRE(original);
   signed_block basic(*original, true);

   BOOST_TEST(basic.transaction_mroot.str() == original->transaction_mroot.str());
   BOOST_TEST(basic.transaction_mroot.str() == calculate_trx_merkle(basic.transactions).str());

   auto recovered = basic.to_signed_block_v0();
   BOOST_REQUIRE(recovered);
   BOOST_TEST(fc::raw::pack(*original) == fc::raw::pack(*recovered));

   fc::datastream<std::size_t> size_stream;
   std::size_t padded_size = basic.pack(size_stream, packed_transaction::cf_compression_type::none);
   BOOST_TEST(size_stream.tellp() <= padded_size);
   std::vector<char> buffer(padded_size);
   fc::datastream<char*> stream(buffer.data(), buffer.size());
   basic.pack(stream, packed_transaction::cf_compression_type::none);
   signed_block deserialized;
   fc::datastream<const char*> in(buffer.data(), buffer.size());
   deserialized.unpack(in, packed_transaction::cf_compression_type::none);
   std::size_t unpacked_size = padded_size;
   BOOST_TEST(in.tellp() <= buffer.size());
   BOOST_TEST(deserialized.transaction_mroot.str() == original->transaction_mroot.str());
   BOOST_TEST(deserialized.transaction_mroot.str() == calculate_trx_merkle(deserialized.transactions).str());
   deserialized.transactions.back().trx.get<packed_transaction>().prune_all();
   deserialized.prune_state = signed_block::prune_state_type::incomplete;
   BOOST_TEST(deserialized.transaction_mroot.str() == calculate_trx_merkle(deserialized.transactions).str());
   fc::datastream<char*> out(buffer.data(), buffer.size());
   deserialized.pack(out, packed_transaction::cf_compression_type::none);
   BOOST_TEST(out.tellp() <= buffer.size());

   BOOST_TEST(!deserialized.to_signed_block_v0());
}

BOOST_AUTO_TEST_CASE(reflector_init_test) {
   try {

      base_reflect br;
      br.bv = 42;
      derived_reflect dr;
      dr.bv = 42;
      dr.dv = 52;
      final_reflect fr;
      fr.bv = 42;
      fr.dv = 52;
      fr.fv = 62;
      BOOST_CHECK_EQUAL( br.base_reflect_initialized, false );
      BOOST_CHECK_EQUAL( dr.derived_reflect_initialized, false );

      { // base
         // pack
         uint32_t pack_size = fc::raw::pack_size( br );
         vector<char> buf( pack_size );
         fc::datastream<char*> ds( buf.data(), pack_size );

         fc::raw::pack( ds, br );
         // unpack
         ds.seekp( 0 );
         base_reflect br2;
         fc::raw::unpack( ds, br2 );
         // pack again
         pack_size = fc::raw::pack_size( br2 );
         fc::datastream<char*> ds2( buf.data(), pack_size );
         fc::raw::pack( ds2, br2 );
         // unpack
         ds2.seekp( 0 );
         base_reflect br3;
         fc::raw::unpack( ds2, br3 );
         // to/from variant
         fc::variant v( br3 );
         base_reflect br4;
         fc::from_variant( v, br4 );

         BOOST_CHECK_EQUAL( br2.bv, 42 );
         BOOST_CHECK_EQUAL( br2.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br2.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( br3.bv, 42 );
         BOOST_CHECK_EQUAL( br3.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br3.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( br4.bv, 42 );
         BOOST_CHECK_EQUAL( br4.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br4.base_reflect_called, 1 );
      }
      { // derived
         // pack
         uint32_t pack_size = fc::raw::pack_size( dr );
         vector<char> buf( pack_size );
         fc::datastream<char*> ds( buf.data(), pack_size );

         fc::raw::pack( ds, dr );
         // unpack
         ds.seekp( 0 );
         derived_reflect dr2;
         fc::raw::unpack( ds, dr2 );
         // pack again
         pack_size = fc::raw::pack_size( dr2 );
         fc::datastream<char*> ds2( buf.data(), pack_size );
         fc::raw::pack( ds2, dr2 );
         // unpack
         ds2.seekp( 0 );
         derived_reflect dr3;
         fc::raw::unpack( ds2, dr3 );
         // to/from variant
         fc::variant v( dr3 );
         derived_reflect dr4;
         fc::from_variant( v, dr4 );

         BOOST_CHECK_EQUAL( dr2.bv, 42 );
         BOOST_CHECK_EQUAL( dr2.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr2.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( dr3.bv, 42 );
         BOOST_CHECK_EQUAL( dr3.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr3.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( dr4.bv, 42 );
         BOOST_CHECK_EQUAL( dr4.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr4.base_reflect_called, 1 );

         BOOST_CHECK_EQUAL( dr2.dv, 52 );
         BOOST_CHECK_EQUAL( dr2.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr2.derived_reflect_called, 1 );
         BOOST_CHECK_EQUAL( dr3.dv, 52 );
         BOOST_CHECK_EQUAL( dr3.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr3.derived_reflect_called, 1 );
         BOOST_CHECK_EQUAL( dr4.dv, 52 );
         BOOST_CHECK_EQUAL( dr4.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr4.derived_reflect_called, 1 );

         base_reflect br5;
         ds2.seekp( 0 );
         fc::raw::unpack( ds2, br5 );
         base_reflect br6;
         fc::from_variant( v, br6 );

         BOOST_CHECK_EQUAL( br5.bv, 42 );
         BOOST_CHECK_EQUAL( br5.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br5.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( br6.bv, 42 );
         BOOST_CHECK_EQUAL( br6.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br6.base_reflect_called, 1 );
      }
      { // final
         // pack
         uint32_t pack_size = fc::raw::pack_size( fr );
         vector<char> buf( pack_size );
         fc::datastream<char*> ds( buf.data(), pack_size );

         fc::raw::pack( ds, fr );
         // unpack
         ds.seekp( 0 );
         final_reflect fr2;
         fc::raw::unpack( ds, fr2 );
         // pack again
         pack_size = fc::raw::pack_size( fr2 );
         fc::datastream<char*> ds2( buf.data(), pack_size );
         fc::raw::pack( ds2, fr2 );
         // unpack
         ds2.seekp( 0 );
         final_reflect fr3;
         fc::raw::unpack( ds2, fr3 );
         // to/from variant
         fc::variant v( fr3 );
         final_reflect fr4;
         fc::from_variant( v, fr4 );

         BOOST_CHECK_EQUAL( fr2.bv, 42 );
         BOOST_CHECK_EQUAL( fr2.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr2.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( fr3.bv, 42 );
         BOOST_CHECK_EQUAL( fr3.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr3.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( fr4.bv, 42 );
         BOOST_CHECK_EQUAL( fr4.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr4.base_reflect_called, 1 );

         BOOST_CHECK_EQUAL( fr2.dv, 52 );
         BOOST_CHECK_EQUAL( fr2.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr2.derived_reflect_called, 1 );
         BOOST_CHECK_EQUAL( fr3.dv, 52 );
         BOOST_CHECK_EQUAL( fr3.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr3.derived_reflect_called, 1 );
         BOOST_CHECK_EQUAL( fr4.dv, 52 );
         BOOST_CHECK_EQUAL( fr4.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr4.derived_reflect_called, 1 );

         BOOST_CHECK_EQUAL( fr2.fv, 62 );
         BOOST_CHECK_EQUAL( fr2.final_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr2.final_reflect_called, 1 );
         BOOST_CHECK_EQUAL( fr3.fv, 62 );
         BOOST_CHECK_EQUAL( fr3.final_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr3.final_reflect_called, 1 );
         BOOST_CHECK_EQUAL( fr4.fv, 62 );
         BOOST_CHECK_EQUAL( fr4.final_reflect_initialized, true );
         BOOST_CHECK_EQUAL( fr4.final_reflect_called, 1 );

         base_reflect br5;
         ds2.seekp( 0 );
         fc::raw::unpack( ds2, br5 );
         base_reflect br6;
         fc::from_variant( v, br6 );

         BOOST_CHECK_EQUAL( br5.bv, 42 );
         BOOST_CHECK_EQUAL( br5.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br5.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( br6.bv, 42 );
         BOOST_CHECK_EQUAL( br6.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( br6.base_reflect_called, 1 );

         derived_reflect dr7;
         ds2.seekp( 0 );
         fc::raw::unpack( ds2, dr7 );
         derived_reflect dr8;
         fc::from_variant( v, dr8 );

         BOOST_CHECK_EQUAL( dr7.bv, 42 );
         BOOST_CHECK_EQUAL( dr7.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr7.base_reflect_called, 1 );
         BOOST_CHECK_EQUAL( dr8.bv, 42 );
         BOOST_CHECK_EQUAL( dr8.base_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr8.base_reflect_called, 1 );

         BOOST_CHECK_EQUAL( dr7.dv, 52 );
         BOOST_CHECK_EQUAL( dr7.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr7.derived_reflect_called, 1 );
         BOOST_CHECK_EQUAL( dr8.dv, 52 );
         BOOST_CHECK_EQUAL( dr8.derived_reflect_initialized, true );
         BOOST_CHECK_EQUAL( dr8.derived_reflect_called, 1 );
      }

   } FC_LOG_AND_RETHROW()
}

// Verify appbase::execution_priority_queue uses a stable priority queue so that jobs are executed
// in order, FIFO, as submitted.
BOOST_AUTO_TEST_CASE(stable_priority_queue_test) {
  try {
     using namespace std::chrono_literals;

     appbase::execution_priority_queue pri_queue;
     auto io_serv = std::make_shared<boost::asio::io_service>();
     auto work_ptr = std::make_unique<boost::asio::io_service::work>(*io_serv);
     std::atomic<int> posted{0};

     std::thread t( [io_serv, &pri_queue, &posted]() {
        while( posted < 100 && io_serv->run_one() ) {
           ++posted;
        }
        bool more = true;
        while( more || io_serv->run_one() ) {
           while( io_serv->poll_one() ) {}
           // execute the highest priority item
           more = pri_queue.execute_highest();
        }
     } );
     std::atomic<int> ran{0};
     std::mutex mx;
     std::vector<int> results;
     for( int i = 0; i < 50; ++i ) {
        boost::asio::post(*io_serv, pri_queue.wrap(appbase::priority::low, [io_serv, &mx, &ran, &results, i](){
           std::lock_guard<std::mutex> g(mx);
           results.push_back( 50 + i );
           ++ran;
        }));
        boost::asio::post(*io_serv, pri_queue.wrap(appbase::priority::high, [io_serv, &mx, &ran, &results, i](){
           std::lock_guard<std::mutex> g(mx);
           results.push_back( i );
           ++ran;
        }));
     }

     while( ran < 100 ) std::this_thread::sleep_for( 5us );

     work_ptr.reset();
     io_serv->stop();
     t.join();

     std::lock_guard<std::mutex> g(mx);
     BOOST_CHECK_EQUAL( 100, results.size() );
     for( int i = 0; i < 100; ++i ) {
        BOOST_CHECK_EQUAL( i, results.at( i ) );
     }

  } FC_LOG_AND_RETHROW()
}

// test that std::bad_alloc is being thrown
BOOST_AUTO_TEST_CASE(bad_alloc_test) {
   tester t; // force a controller to be constructed and set the new_handler
   int* ptr = nullptr;
   const auto fail = [&]() {
      ptr = new int[std::numeric_limits<int64_t>::max()/16];
   };
   BOOST_CHECK_THROW( fail(), std::bad_alloc );
   BOOST_CHECK( ptr == nullptr );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace eosio
