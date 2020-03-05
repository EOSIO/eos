#define BOOST_TEST_MODULE trace_data_logging
#include <boost/test/included/unit_test.hpp>

#include <eosio/trace_api_plugin/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api_plugin;
using namespace eosio::trace_api_plugin::test_common;

namespace {

};

struct logfile_test_fixture {
   /**
    * TODO: initialize logfile implementation here
    * OPTIONAL use `mock_filesystem_provider` as template param to store things in RAM?
    */
   logfile_test_fixture()
   // : logfile_impl()
   {
   }


   /**
    * Writing API
    */

   /**
    * append an entry to the end of the data log
    *
    * @param entry : the entry to append
    * @return the offset in the log where that entry is written
    */
   uint64_t append_data_log( const data_log_entry& entry ) {
      // TODO: route to logfile subsystem
      // return logfile_impl.append_data_log(std::move(entry));
      return 0;
   }

   /**
    * Append an entry to the metadata log
    *
    * @param entry
    * @return
    */
   uint64_t append_metadata_log( const metadata_log_entry& entry ) {
      // TODO: route to logfile subsystem
      // return logfile_impl.append_data_log(std::move(entry));
      return 0;
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
      // TODO: route to logfile subsystem
      return {};
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
      // TODO: route to logfile subsystem
      return offset;
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

   // TODO: declare extraction implementation here with `mock_logfile_provider` as template param
   // logfile_impl_type logfile_impl;
};

BOOST_AUTO_TEST_SUITE(logfile_access)
   BOOST_FIXTURE_TEST_CASE(single_slice_write, logfile_test_fixture)
   {
//      auto block_1_data_entry = data_log_entry {
//         block_trace_v0 {
//            "b000000000000000000000000000000000000000000000000000000000000001"_h,
//            1,
//            "0000000000000000000000000000000000000000000000000000000000000000"_h,
//            chain::block_timestamp_type(1),
//            "bp.one"_n,
//            {}
//         }
//      };
//
//      // write the data
//      auto block_1_offset = append_data_log(block_1_data_entry);
//
//      auto block_1_metadata_entry = metadata_log_entry {
//         block_entry_v0 {
//            "b000000000000000000000000000000000000000000000000000000000000001"_h,
//            1,
//            block_1_offset
//         }
//      };
//
//      // write the metadata
//      //
//      append_metadata_log(block_1_metadata_entry);
//
//      auto expected_metadata_log = std::vector<metadata_log_entry> { block_1_metadata_entry };
//      auto actual_metadata_log = inspect_metadata_log(1);
//      BOOST_REQUIRE_EQUAL_COLLECTIONS(actual_metadata_log.begin(), actual_metadata_log.end(), expected_metadata_log.begin(), expected_metadata_log.end());
//
//      auto actual_block_1_data_entry_opt = read_data_log(1, block_1_offset);
//      BOOST_REQUIRE(actual_block_1_data_entry_opt);
//      BOOST_REQUIRE_EQUAL(*actual_block_1_data_entry_opt, block_1_data_entry);
   }

BOOST_AUTO_TEST_SUITE_END()
