#define BOOST_TEST_MODULE key_recovery_service
#include <boost/test/unit_test.hpp>

#include <eosio/blockchain/key_recovery_service.hpp>
#include <eosio/blockchain/internal/thread_pool_policy.hpp>

using namespace eosio::blockchain;
using namespace eosio::blockchain::internal;

template<int ThreadCount>
class thread_fixture {
   public:
      typedef key_recovery_service<static_thread_pool_policy<ThreadCount>> key_recovery_service_type;
      thread_fixture()
      :krs(make_unique<key_recovery_service_type>())
      {}

      ~thread_fixture() {}

      void wait_for_completion() {
         krs = nullptr;
      }

      unique_ptr<key_recovery_service_type> krs;
};

#define CATCH_LOG_DROP( )  \
   catch( const fc::exception& er ) { \
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
      auto public_key = public_key_type(private_key.get_public_key());
      auto signatures = vector<signature_type>({private_key.sign_compact(digest)});
      vector<public_key_type> output;

      (*krs).recover_keys(digest, signatures, [&](async_result<vector<public_key_type>>&& result) -> void {
         try {
            output = result.get();
         } CATCH_LOG_DROP()
      });

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(output.size(), 1);
      BOOST_CHECK_EQUAL((std::string)output[0], (std::string)public_key);
   }

   BOOST_FIXTURE_TEST_CASE(test_bad_digest_single, thread_fixture<1>) {
      auto digest = digest_type("0123456789abcdef");
      auto bad_digest = digest_type("0123456789abcdef01234");
      auto private_key = private_key_type::generate();
      auto public_key = public_key_type(private_key.get_public_key());
      auto signatures = vector<signature_type>({private_key.sign_compact(digest)});
      vector<public_key_type> output;

      (*krs).recover_keys(bad_digest, signatures, [&](async_result<vector<public_key_type>>&& result) -> void {
         try {
            output = result.get();
         } CATCH_LOG_DROP()
      });

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(output.size(), 1);
      BOOST_CHECK_NE((std::string)output[0], (std::string)public_key);
   }

   BOOST_FIXTURE_TEST_CASE(test_unrecoverable_signature_single, thread_fixture<1>) {
      auto digest = digest_type("0123456789abcdef");
      auto private_key = private_key_type::generate();
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

   BOOST_FIXTURE_TEST_CASE(test_success_multi, thread_fixture<10>) {
      auto digests = vector<digest_type>({
         digest_type("0123456789abcdef"),
         digest_type("123456789abcdef0"),
         digest_type("23456789abcdef01"),
         digest_type("3456789abcdef012"),
         digest_type("456789abcdef0123"),
         digest_type("56789abcdef01234"),
         digest_type("6789abcdef012345"),
         digest_type("789abcdef0123456"),
         digest_type("89abcdef01234567"),
         digest_type("9abcdef012345678")
      });

      auto private_keys = vector<private_key_type>({
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
         private_key_type::generate(),
      });

      auto inputs = ([&](){
         auto result = vector<vector<signature_type>>();
         for (const auto& digest: digests) {
            auto signatures = vector<signature_type>();
            for (const auto& key: private_keys) {
               signatures.emplace_back(key.sign_compact(digest));
            }
            result.emplace_back(signatures);
         }

         return result;
      })();

      vector<vector<public_key_type>> outputs(10);

      for (int idx = 0; idx < inputs.size(); idx++) {
         const auto& input = inputs.at(idx);
         const auto& digest = digests.at(idx);
         (*krs).recover_keys(digest, input, [=,&outputs](async_result<vector<public_key_type>> &&result) -> void {
            try {
               outputs.at(idx) = result.get();
            } CATCH_LOG_DROP()
         });
      }

      wait_for_completion();

      BOOST_REQUIRE_EQUAL(outputs.size(), inputs.size());
      for (const auto& output: outputs) {
         BOOST_REQUIRE_EQUAL(output.size(), private_keys.size());
         for(int idx = 0; idx < output.size(); idx++) {
            BOOST_CHECK_EQUAL((std::string)output[idx], (std::string)(public_key_type(private_keys[idx].get_public_key())));
         }
      }
   }
BOOST_AUTO_TEST_SUITE_END()