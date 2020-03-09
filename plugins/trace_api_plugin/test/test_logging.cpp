#define BOOST_TEST_MODULE trace_data_logging
#include <boost/test/included/unit_test.hpp>
#include <eosio/trace_api/store_provider.hpp>

#include <eosio/trace_api/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api;
using namespace eosio::trace_api::test_common;

namespace {

};

struct logfile_test_fixture {
   /**
    * TODO: initialize logfile implementation here
    * OPTIONAL use `mock_filesystem_provider` as template param to store things in RAM?
    */
   logfile_test_fixture()
   : store_impl(tempdir.path(), 100)
   {
   }


   /**
    * Writing API
    */

   /**
    * append an entry to the end of the data log
    *
    * @param bt : block trace to append
    */
   void append_data_log( const block_trace_v0& bt ) {
      store_impl.append(bt);
   }

   /**
    * Reading API
    */

   /**
    * Read from the data log
    * @param block_height : the block_height of the data being read
    * @param offset : the offset in the datalog to read
    * @return empty optional if the data log does not exist, data otherwise
    * @throws std::exception : when the data is not the correct type or if the log is corrupt in some way
    *
    */
   std::optional<data_log_entry> read_data_log( uint32_t block_height, uint64_t offset ) {
      return store_impl.read_data_log(block_height, offset);
   }

   /**
    * Read the metadata log font-to-back starting at an offset passing each entry to a provided functor/lambda
    *
    * @tparam Fn : type of the functor/lambda
    * @param block_height : height of the requested data
    * @param offset : initial offset to read from
    * @param fn : the functor/lambda
    * @return the highest offset read during this scan
    */
   template<typename Fn>
   uint64_t scan_metadata_log_from( uint32_t block_height, uint64_t offset, Fn&& fn ) {
      return store_impl.scan_metadata_log_from(block_height, offset, fn);
   }

   /**
    * Test helpers
    */
    std::vector<metadata_log_entry> inspect_metadata_log( uint32_t block_height ) {
       std::vector<metadata_log_entry> result;
       scan_metadata_log_from(block_height, 0, [&result](const metadata_log_entry& e) -> bool {
          result.emplace_back(e);
          return true;
       });

       return result;
    }

   fc::temp_directory tempdir;
   store_provider store_impl;
};

BOOST_AUTO_TEST_SUITE(logfile_access)
   BOOST_FIXTURE_TEST_CASE(single_slice_write, logfile_test_fixture)
   {
      auto block_1_data_entry =
         block_trace_v0 {
            "b000000000000000000000000000000000000000000000000000000000000001"_h,
            1,
            "0000000000000000000000000000000000000000000000000000000000000000"_h,
            chain::block_timestamp_type(1),
            "bp.one"_n,
            {}
         };

      // write the data
      append_data_log(block_1_data_entry);
      uint64_t block_1_offset = 0;

      auto expected_block_entry =
         block_entry_v0 {
            "b000000000000000000000000000000000000000000000000000000000000001"_h,
            1,
            block_1_offset
         };

      auto actual_metadata_log = inspect_metadata_log(1);
      BOOST_REQUIRE_EQUAL(actual_metadata_log.size(), 1);
      BOOST_REQUIRE(actual_metadata_log[0].contains<block_entry_v0>());
      const auto actual_block_entry = actual_metadata_log[0].get<block_entry_v0>();
      BOOST_REQUIRE( actual_block_entry == expected_block_entry );

      auto actual_block_1_data_entry_opt = read_data_log(1, block_1_offset);
      BOOST_REQUIRE(actual_block_1_data_entry_opt);
      BOOST_REQUIRE(actual_block_1_data_entry_opt->get<block_trace_v0>() == block_1_data_entry);
   }

BOOST_AUTO_TEST_SUITE_END()
