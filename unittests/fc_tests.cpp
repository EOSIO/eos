#include <eosio/chain/types.hpp>
#include <fc/log/logger_config.hpp>
#include <fc/bitutil.hpp>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/action.hpp>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE(fc_tests)

BOOST_AUTO_TEST_CASE(variant_format_string_limited)
{
   {
      const string format = "${a} ${b} ${c}";
      fc::mutable_variant_object mu;
      mu( "a", string( 1024, 'a' ) );
      mu( "b", string( 1024, 'b' ) );
      mu( "c", string( 1024, 'c' ) );
      const string result = fc::format_string( format, mu, true );
      const auto arg_limit_size = (1024 - format.size()) / mu.size();
      BOOST_CHECK_EQUAL( result, string(arg_limit_size, 'a' ) + "... " + string(arg_limit_size, 'b' ) + "... " + string(arg_limit_size, 'c' ) + "..." );
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
   {
      const string format = "${a} ${b} ${c} ${d} ${e}";
      fc::mutable_variant_object mu;
      signed_block a;
      for(size_t idx=0; idx<10; ++idx) {
         a.transactions.push_back(transaction_receipt());
      }
      fc::blob b;
      for( int i = 0; i < 1024; ++i)
         b.data.push_back('b');
      fc::variant b_vairant;
      to_variant(b, b_vairant);
      fc::variants c;
      c.push_back(variant(a));
      mu( "a", a );
      mu( "b", b );
      mu( "c", c );
      mu( "d", string( 1024, 'd' ) );
      mu( "e", string( 1024, 'e' ) );
      const string result = fc::format_string( format, mu, true );
      const auto arg_limit_size = (1024 - format.size()) / mu.size();
      string target_str = fc::json::to_string( a, fc::time_point::maximum()).substr(0, arg_limit_size) + "...} ";
      target_str += b_vairant.as_string().substr(0, arg_limit_size) + "... ";
      target_str += fc::json::to_string( c, fc::time_point::maximum()).substr(0, arg_limit_size) + "...} ";
      target_str += string(arg_limit_size, 'd' ) + "... ";
      target_str += string(arg_limit_size, 'e' ) + "...";
      BOOST_CHECK_EQUAL( result, target_str);
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
   {
      // test case for long format string
      const string format_prefix(1024, 'a');
      const string format = format_prefix + " ${a} ${b} ${c}";
      fc::mutable_variant_object mu;
      signed_block a;
      for(size_t idx=0; idx<10; ++idx) {
         a.transactions.push_back(transaction_receipt());
      }
      fc::blob b;
      for( int i = 0; i < 1024; ++i)
         b.data.push_back('b');
      fc::variant b_vairant;
      to_variant(b, b_vairant);
      fc::variants c;
      c.push_back(variant(a));
      mu( "a", a );
      mu( "b", b );
      mu( "c", c );
      const string result = fc::format_string( format, mu, true );
      const string target_str = format_prefix + "...";
      BOOST_CHECK_EQUAL( result, target_str);
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
   {
      // test cases for issue #8741
      flat_set <permission_level> provided_permissions;
      for (char ch = 'a'; ch < 'z'; ++ch) {
         for (size_t idx = 1; idx < 10; ++idx) {
            provided_permissions.insert(
                  {name(std::string_view(string(idx, ch))), name(std::string_view(string(idx, ch + 1)))});
         }
      }
      flat_set <public_key_type> provided_keys;
      std::string digest = "1234567";
      for (auto &permission : provided_permissions) {
         digest += "1";
         const std::string key_name_str = permission.actor.to_string() + permission.permission.to_string();
         auto sig_digest = digest_type::hash(std::make_pair("1234", "abcd"));
         const fc::crypto::signature sig = private_key_type::regenerate<fc::ecc::private_key_shim>(
               fc::sha256::hash(key_name_str + "active")).sign(sig_digest);
         provided_keys.insert(public_key_type{sig, fc::sha256{digest}, true});
      }
      const string format = "transaction declares authority '${auth}', provided permissions ${provided_permissions}, provided keys ${provided_keys}";
      fc::mutable_variant_object mu;
      mu("auth", *provided_permissions.begin());
      mu("provided_permissions", provided_permissions);
      mu("provided_keys", provided_keys);
      const auto arg_limit_size = (1024 - format.size()) / mu.size();
      const string result = fc::format_string(format, mu, true);
      string target_str = "transaction declares authority '" +
                          fc::json::to_string(*provided_permissions.begin(), fc::time_point::maximum()).substr(0, arg_limit_size);
      target_str += "', provided permissions " +
                    fc::json::to_string(provided_permissions, fc::time_point::maximum()).substr(0, arg_limit_size);
      target_str += "...}, provided keys " +
                    fc::json::to_string(provided_keys, fc::time_point::maximum()).substr(0, arg_limit_size) + "...}";

      BOOST_CHECK_EQUAL(result, target_str);
      BOOST_CHECK_LT(result.size(), 1024 + 3 * mu.size());
   }
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

BOOST_AUTO_TEST_SUITE_END()

} // namespace eosio
