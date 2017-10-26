#define BOOST_TEST_MODULE key_recovery_service
#include <boost/test/unit_test.hpp>

#include <eosio/blockchain/internal/key_recovery_service.hpp>
#include <eosio/blockchain/internal/shared_dispatch_policy.hpp>
#include <eosio/blockchain/internal/thread_pool_policy.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <fc/optional.hpp>

using namespace eosio;
using namespace blockchain;
using namespace internal;
using namespace fc;

template<int ThreadCount>
class thread_fixture {
public:
   typedef key_recovery_service<static_thread_pool_policy<ThreadCount>> key_recovery_service_type;
   thread_fixture()
      :krs(std::make_unique<key_recovery_service_type>())
   {
   }

   ~thread_fixture() {
   }

   void wait_for_completion() {
      krs = nullptr;
   }

   std::unique_ptr<key_recovery_service_type> krs;
};

#define CATCH_LOG_DROP( )  \
   catch( fc::exception& er ) { \
      wlog( "${details}", ("details",er.to_detail_string()) ); \
   } catch( const std::exception& e ) {  \
      fc::exception fce( \
                FC_LOG_MESSAGE( warn, "rethrow ${what}: ",("what",e.what()) ), \
                fc::std_exception_code,\
                typeid(e).name(), \
                e.what() ) ; \
      wlog( "${details}", ("details",fce.to_detail_string()) ); \
   } catch( ... ) {  \
      fc::unhandled_exception e( \
                FC_LOG_MESSAGE( warn, "rethrow"), \
                std::current_exception() ); \
      wlog( "${details}", ("details",e.to_detail_string()) ); \
   }


BOOST_AUTO_TEST_SUITE(key_recovery_service_tests)
   BOOST_FIXTURE_TEST_CASE(test_success_single, thread_fixture<1>) {
      auto digest = digest_type("0123456789abcdef");
      auto private_key = private_key_type::generate();
      auto public_key = private_key.get_public_key();
      auto signatures = vector<signature_type>({private_key.sign_compact(digest)});
      vector<public_key_type> output;

      (*krs).recover_keys(digest, signatures, [&](async_result<vector<public_key_type>>&& result) -> void {
         try {
            output = result.get();
         } CATCH_LOG_DROP()
      });

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(output.size(), 1);
      BOOST_CHECK_EQUAL(output[0].to_base58(), public_key.to_base58());
   }

   BOOST_FIXTURE_TEST_CASE(test_bad_digest_single, thread_fixture<1>) {
      auto digest = digest_type("0123456789abcdef");
      auto bad_digest = digest_type("0123456789abcdef01234");
      auto private_key = private_key_type::generate();
      auto public_key = private_key.get_public_key();
      auto signatures = vector<signature_type>({private_key.sign_compact(digest)});
      vector<public_key_type> output;

      (*krs).recover_keys(bad_digest, signatures, [&](async_result<vector<public_key_type>>&& result) -> void {
         try {
            output = result.get();
         } CATCH_LOG_DROP()
      });

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(output.size(), 1);
      BOOST_CHECK_NE(output[0].to_base58(),public_key.to_base58());
   }

   BOOST_FIXTURE_TEST_CASE(test_unrecoverable_signature_single, thread_fixture<1>) {
      auto digest = digest_type("0123456789abcdef");
      auto private_key = private_key_type::generate();
      auto public_key = private_key.get_public_key();
      auto signatures = vector<signature_type>({private_key.sign_compact(digest)});

      // create error
      signatures.at(0).data[0] = 0;

      vector<public_key_type> output;

      (*krs).recover_keys(digest, signatures, [&](async_result<vector<public_key_type>>&& result) -> void {
         try {
            output = result.get();
         } CATCH_LOG_DROP()
      });

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(output.size(), 0);
   }

   BOOST_FIXTURE_TEST_CASE(test_non_canonical_signature_single, thread_fixture<1>) {
      auto digest = digest_type("0123456789abcdef");
      auto private_key = private_key_type::generate();
      auto public_key = private_key.get_public_key();
      auto signatures = vector<signature_type>({private_key.sign_compact(digest)});

      // create error
      signatures.at(0).data[1] |= 0x80;

      vector<public_key_type> output;

      (*krs).recover_keys(digest, signatures, [&](async_result<vector<public_key_type>>&& result) -> void {
         try {
            output = result.get();
         } CATCH_LOG_DROP()
      });

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(output.size(), 0);
   }


BOOST_AUTO_TEST_SUITE_END()