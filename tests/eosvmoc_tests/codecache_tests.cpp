#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/testing/tester.hpp>
#include <contracts.hpp>

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

using namespace eosio::chain;
namespace data = boost::unit_test::data;

//always return same fixed wasm to populate code cache. simple and works for this testing purpose
static std::string_view get_some_wasm(const digest_type&, uint8_t) {
   static auto noop_vector = eosio::testing::contracts::noop_wasm();
   return std::string_view((const char*)noop_vector.data(), noop_vector.size());
}

static chainbase::pinnable_mapped_file::map_mode mapped_and_heap[] = {chainbase::pinnable_mapped_file::map_mode::mapped,
                                                                      chainbase::pinnable_mapped_file::map_mode::heap};

BOOST_AUTO_TEST_SUITE(eosvmoc_cc_tests)

//create a code cache in one mode, close it and open it in a second mode ensuring the populated data from the first
// go around is the same, then close it and open it yet again in the orginal mode
BOOST_DATA_TEST_CASE(there_and_back_again, data::make(mapped_and_heap) * data::make(mapped_and_heap), first, second) { try {
   fc::temp_directory tmp_code_cache;

   const eosvmoc::code_descriptor* first_desc = nullptr;

   eosvmoc::config first_eosvmoc_config = {32u*1024u*1024u, 1u, first};
   eosvmoc::config second_eosvmoc_config = {32u*1024u*1024u, 1u, second};

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), first_eosvmoc_config, get_some_wasm);
      first_desc = cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0));
      BOOST_REQUIRE(first_desc);
   }

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), second_eosvmoc_config, get_some_wasm);
      const eosvmoc::code_descriptor* const second_desc = cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0));
      BOOST_REQUIRE(second_desc);
      BOOST_REQUIRE_EQUAL(first_desc->code_begin, second_desc->code_begin);
   }

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), first_eosvmoc_config, get_some_wasm);
      const eosvmoc::code_descriptor* const desc = cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0));
      BOOST_REQUIRE(desc);
      BOOST_REQUIRE_EQUAL(first_desc->code_begin, desc->code_begin);
   }
} FC_LOG_AND_RETHROW() }

//try to open the code cache while a first instance is already open -- should fail. Then open it again just cause.
BOOST_DATA_TEST_CASE(dirty_check, data::make(mapped_and_heap) * data::make(mapped_and_heap), first, second) { try {
   fc::temp_directory tmp_code_cache;

   eosvmoc::config first_eosvmoc_config = {32u*1024u*1024u, 1u, first};
   eosvmoc::config second_eosvmoc_config = {32u*1024u*1024u, 1u, second};

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), first_eosvmoc_config, get_some_wasm);

      BOOST_REQUIRE_EXCEPTION(eosvmoc::code_cache_sync(tmp_code_cache.path(), second_eosvmoc_config, get_some_wasm), database_exception,
                              eosio::testing::fc_exception_message_is("code cache is dirty") );
   }

   eosvmoc::code_cache_sync cc(tmp_code_cache.path(), first_eosvmoc_config, get_some_wasm);
} FC_LOG_AND_RETHROW() }

//test that growing the code cache behaves as expected
BOOST_DATA_TEST_CASE(growit, data::make(mapped_and_heap) * data::make(mapped_and_heap) * data::make(mapped_and_heap), first, second, third) { try {
   fc::temp_directory tmp_code_cache;

   eosvmoc::config first_eosvmoc_config = {32u*1024u*1024u, 1u, first};
   eosvmoc::config second_eosvmoc_config = {256u*1024u*1024u, 1u, second};
   eosvmoc::config third_eosvmoc_config = {32u*1024u*1024u, 1u, third};

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), first_eosvmoc_config, get_some_wasm);
      BOOST_REQUIRE(cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0)));
      BOOST_REQUIRE(cc.number_entries());
      BOOST_REQUIRE(cc.free_bytes());
      BOOST_REQUIRE(*cc.free_bytes() < first_eosvmoc_config.cache_size);
      cc.free_code(digest_type(), UINT8_C(0)); //so free_bytes() can be populated next time
   }

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), second_eosvmoc_config, get_some_wasm);
      BOOST_REQUIRE(cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0)));
      BOOST_REQUIRE(cc.number_entries());
      BOOST_REQUIRE(cc.free_bytes());
      BOOST_REQUIRE(*cc.free_bytes() > first_eosvmoc_config.cache_size*2);
      cc.free_code(digest_type(), UINT8_C(0)); //so free_bytes() can be populated next time
   }

   //eos vm oc's code cache size behaves similar to that as chainbase: reducing the config does not shrink the cache
   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), third_eosvmoc_config, get_some_wasm);
      BOOST_REQUIRE(cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0)));
      BOOST_REQUIRE(cc.number_entries());
      BOOST_REQUIRE(cc.free_bytes());
      BOOST_REQUIRE(*cc.free_bytes() > first_eosvmoc_config.cache_size*2);
   }
} FC_LOG_AND_RETHROW() }

//try creating a code cache with a size not a multiple of page size
BOOST_DATA_TEST_CASE(oddball_size, data::make(mapped_and_heap), mode) { try {
   fc::temp_directory tmp_code_cache;

   eosvmoc::config eosvmoc_config = {32u*1024u*1024u+708u, 1u, mode};

   {
      eosvmoc::code_cache_sync cc(tmp_code_cache.path(), eosvmoc_config, get_some_wasm);
      BOOST_REQUIRE(cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0)));
   }
   BOOST_REQUIRE_EQUAL(boost::filesystem::file_size(tmp_code_cache.path() / "code_cache.bin"), eosvmoc_config.cache_size);

   eosvmoc::code_cache_sync cc(tmp_code_cache.path(), eosvmoc_config, get_some_wasm);
   BOOST_REQUIRE(cc.get_descriptor_for_code_sync(digest_type(), UINT8_C(0)));
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()