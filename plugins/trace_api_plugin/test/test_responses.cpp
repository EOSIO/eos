#define BOOST_TEST_MODULE trace_data_responses
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/trace_api_plugin/request_handler.hpp>
#include <eosio/trace_api_plugin/base64_data_handler.hpp>
#include <eosio/trace_api_plugin/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api_plugin;
using namespace eosio::trace_api_plugin::test_common;

struct response_test_fixture {

   /**
    * MOCK implementation of the logfile input API
    */
   struct mock_logfile_provider {
      mock_logfile_provider(response_test_fixture& fixture)
      :fixture(fixture)
      {}

      /**
       * Read from the data log
       * @param block_height : the block_height of the data being read
       * @param offset : the offset in the datalog to read
       * @return empty optional if the data log does not exist, data otherwise
       * @throws std::exception : when the data is not the correct type or if the log is corrupt in some way
       *
       */
      std::optional<data_log_entry> read_data_log( uint32_t block_height, uint64_t offset ) {
         if (fixture.data_log.count(offset) == 0) {
            return {};
         } else {
            const auto& res = fixture.data_log.at(offset);
            if (res.contains<std::exception_ptr>()) {
               std::rethrow_exception(res.get<std::exception_ptr>());
            } else {
               return res.get<data_log_entry>();
            }
         }
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
         uint64_t cur = offset;
         for (; cur < fixture.metadata_log.size(); cur++) {
            const auto& entry = fixture.metadata_log.at(cur);
            if (entry.contains<std::exception_ptr>()) {
               std::rethrow_exception(entry.get<std::exception_ptr>());
            }

            if (entry.contains<metadata_log_entry>()) {
               if (!fn(entry)) {
                  break;
               }
            }
         }

         return cur;
      }


      response_test_fixture& fixture;
   };

   struct mock_data_handler_provider {
      fc::variant process_data(const action_trace_v0& action, const fc::time_point&) {
         return fc::to_hex(action.data.data(), action.data.size());
      }
   };

   using response_impl_type = request_handler<mock_logfile_provider, mock_data_handler_provider>;
   /**
    * TODO: initialize extraction implementation here with `mock_logfile_provider` as template param
    */
   response_test_fixture()
   : response_impl(mock_logfile_provider(*this), mock_data_handler_provider(), [this]()->fc::time_point { return mock_now(); })
   {

   }

   fc::variant get_block_trace( uint32_t block_height, const fc::time_point& deadline = fc::time_point::maximum() ) {
      return response_impl.get_block_trace( block_height, deadline );
   }

   // fixture data and methods
   std::vector<fc::static_variant<std::exception_ptr, metadata_log_entry>> metadata_log = {};
   std::map<uint64_t, fc::static_variant<std::exception_ptr, data_log_entry>> data_log = {};
   std::function<fc::time_point()> mock_now = []() -> fc::time_point { return fc::time_point::now(); };

   response_impl_type response_impl;

};

BOOST_AUTO_TEST_SUITE(trace_responses)
   BOOST_FIXTURE_TEST_CASE(basic_empty_block_response, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry{ block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      data_log = decltype(data_log) {
         {
            0,
            data_log_entry{ block_trace_v0 {
               "b000000000000000000000000000000000000000000000000000000000000001"_h,
               1,
               "0000000000000000000000000000000000000000000000000000000000000000"_h,
               chain::block_timestamp_type(0),
               "bp.one"_n,
               {}
            }}
         }
      };

      fc::variant expected_response = fc::mutable_variant_object()
         ("id", "b000000000000000000000000000000000000000000000000000000000000001")
         ("number", 1)
         ("previous_id", "0000000000000000000000000000000000000000000000000000000000000000")
         ("status", "pending")
         ("timestamp", "2000-01-01T00:00:00.000Z")
         ("producer", "bp.one")
         ("transactions", fc::variants() )
      ;

      fc::variant actual_response = get_block_trace( 1 );

      BOOST_TEST(to_kv(expected_response) == to_kv(actual_response), boost::test_tools::per_element());
   }

   BOOST_FIXTURE_TEST_CASE(basic_block_response, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      data_log = decltype(data_log) {
         {
            0,
            data_log_entry{ block_trace_v0 {
               "b000000000000000000000000000000000000000000000000000000000000001"_h,
               1,
               "0000000000000000000000000000000000000000000000000000000000000000"_h,
               chain::block_timestamp_type(0),
               "bp.one"_n,
               {
                  {
                     "0000000000000000000000000000000000000000000000000000000000000001"_h,
                     chain::transaction_receipt_header::executed,
                     {
                        {
                           0,
                           "receiver"_n, "contract"_n, "action"_n,
                           {{ "alice"_n, "active"_n }},
                           { 0x00, 0x01, 0x02, 0x03 }
                        }
                     }
                  }
               }
            }}
         }
      };

      fc::variant expected_response = fc::mutable_variant_object()
         ("id", "b000000000000000000000000000000000000000000000000000000000000001")
         ("number", 1)
         ("previous_id", "0000000000000000000000000000000000000000000000000000000000000000")
         ("status", "pending")
         ("timestamp", "2000-01-01T00:00:00.000Z")
         ("producer", "bp.one")
         ("transactions", fc::variants({
            fc::mutable_variant_object()
               ("id", "0000000000000000000000000000000000000000000000000000000000000001")
               ("status", "executed")
               ("actions", fc::variants({
                  fc::mutable_variant_object()
                     ("receiver", "receiver")
                     ("account", "contract")
                     ("action", "action")
                     ("authorization", fc::variants({
                        fc::mutable_variant_object()
                           ("account", "alice")
                           ("permission", "active")
                     }))
                     ("data", "00010203")
               }))
         }))
      ;

      fc::variant actual_response = get_block_trace( 1 );

      BOOST_TEST(to_kv(expected_response) == to_kv(actual_response), boost::test_tools::per_element());
   }

   BOOST_FIXTURE_TEST_CASE(lib_edge_response, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      data_log = decltype(data_log) {
         {
            0,
            data_log_entry{ block_trace_v0 {
               "b000000000000000000000000000000000000000000000000000000000000001"_h,
               1,
               "0000000000000000000000000000000000000000000000000000000000000000"_h,
               chain::block_timestamp_type(0),
               "bp.one"_n,
               {}
            }}
         }
      };

      fc::variant expected_pending_response = fc::mutable_variant_object()
         ("id", "b000000000000000000000000000000000000000000000000000000000000001")
         ("number", 1)
         ("previous_id", "0000000000000000000000000000000000000000000000000000000000000000")
         ("status", "pending")
         ("timestamp", "2000-01-01T00:00:00.000Z")
         ("producer", "bp.one")
         ("transactions", fc::variants() )
      ;

      fc::variant expected_irreversible_response = fc::mutable_variant_object()
         ("id", "b000000000000000000000000000000000000000000000000000000000000001")
         ("number", 1)
         ("previous_id", "0000000000000000000000000000000000000000000000000000000000000000")
         ("status", "irreversible")
         ("timestamp", "2000-01-01T00:00:00.000Z")
         ("producer", "bp.one")
         ("transactions", fc::variants() )
      ;

      fc::variant pending_response = get_block_trace( 1 );
      BOOST_TEST(to_kv(expected_pending_response) == to_kv(pending_response), boost::test_tools::per_element());

      // push an entry to the metadata log that marks that block as LIB
      metadata_log.emplace_back( metadata_log_entry { lib_entry_v0{ 1 } } );

      fc::variant irreversible_response = get_block_trace( 1 );
      BOOST_TEST(to_kv(expected_irreversible_response) == to_kv(irreversible_response), boost::test_tools::per_element());

   }

   BOOST_FIXTURE_TEST_CASE(better_block_edge_response, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      data_log = decltype(data_log) {
         {
            0,
            data_log_entry{ block_trace_v0 {
               "b000000000000000000000000000000000000000000000000000000000000001"_h,
               1,
               "0000000000000000000000000000000000000000000000000000000000000000"_h,
               chain::block_timestamp_type(0),
               "bp.one"_n,
               {}
            }}
         }
      };

      fc::variant expected_original_response = fc::mutable_variant_object()
         ("id", "b000000000000000000000000000000000000000000000000000000000000001")
         ("number", 1)
         ("previous_id", "0000000000000000000000000000000000000000000000000000000000000000")
         ("status", "pending")
         ("timestamp", "2000-01-01T00:00:00.000Z")
         ("producer", "bp.one")
         ("transactions", fc::variants() )
      ;

      fc::variant expected_updated_response = fc::mutable_variant_object()
         ("id", "b000000000000000000000000000000000000000000000000000000000000002")
         ("number", 1)
         ("previous_id", "0000000000000000000000000000000000000000000000000000000000000000")
         ("status", "pending")
         ("timestamp", "2000-01-01T00:00:00.500Z")
         ("producer", "bp.two")
         ("transactions", fc::variants() )
      ;

      fc::variant original_response = get_block_trace( 1 );
      BOOST_TEST(to_kv(expected_original_response) == to_kv(original_response), boost::test_tools::per_element());

      // add more data
      const uint64_t updated_block_offset = 2;
      data_log[updated_block_offset] = {
         data_log_entry{ block_trace_v0 {
            "b000000000000000000000000000000000000000000000000000000000000002"_h,
            1,
            "0000000000000000000000000000000000000000000000000000000000000000"_h,
            chain::block_timestamp_type(1),
            "bp.two"_n,
            {}
         }}
      };

      // push an entry to the metadata log that marks that block as LIB
      metadata_log.emplace_back(metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000002"_h, 1, updated_block_offset } });

      fc::variant irreversible_response = get_block_trace( 1 );
      BOOST_TEST(to_kv(expected_updated_response) == to_kv(irreversible_response), boost::test_tools::per_element());
   }

   BOOST_FIXTURE_TEST_CASE(corrupt_block_data, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      data_log = decltype(data_log) {
         {
            0,
            std::make_exception_ptr(std::runtime_error("BAD DATA"))
         }
      };

      BOOST_REQUIRE_THROW(get_block_trace( 1 ), bad_data_exception);
   }

   BOOST_FIXTURE_TEST_CASE(corrupt_block_metadata, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } },
         std::make_exception_ptr(std::runtime_error("BAD METADATA"))
      };

      BOOST_REQUIRE_THROW(get_block_trace( 1 ), bad_data_exception);
   }

   BOOST_FIXTURE_TEST_CASE(missing_block_data, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      fc::variant null_response = get_block_trace( 1 );

      BOOST_TEST(null_response.is_null());
   }

   BOOST_FIXTURE_TEST_CASE(missing_block_metadata, response_test_fixture)
   {
      data_log = decltype(data_log) {
         {
            0,
            data_log_entry{ block_trace_v0 {
               "b000000000000000000000000000000000000000000000000000000000000001"_h,
               1,
               "0000000000000000000000000000000000000000000000000000000000000000"_h,
               chain::block_timestamp_type(0),
               "bp.one"_n,
               {}
            }}
         }
      };

      fc::variant null_response = get_block_trace( 1 );

      BOOST_TEST(null_response.is_null());
   }

   BOOST_FIXTURE_TEST_CASE(deadline_throws, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         metadata_log_entry { block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 } }
      };

      data_log = decltype(data_log) {
         {
            0,
            data_log_entry{ block_trace_v0 {
               "b000000000000000000000000000000000000000000000000000000000000001"_h,
               1,
               "0000000000000000000000000000000000000000000000000000000000000000"_h,
               chain::block_timestamp_type(0),
               "bp.one"_n,
               {
                  {
                     "0000000000000000000000000000000000000000000000000000000000000001"_h,
                     chain::transaction_receipt_header::executed,
                     {
                        {
                           0,
                           "receiver"_n, "contract"_n, "action"_n,
                           {{ "alice"_n, "active"_n }},
                           { 0x00, 0x01, 0x02, 0x03 }
                        }
                     }
                  }
               }
            }}
         }
      };

      auto deadline = fc::time_point(fc::microseconds(1));
      int countdown = 3;
      mock_now = [&]() -> fc::time_point {
         if (countdown-- == 0) {
            return deadline;
         } else {
            return fc::time_point();
         }
      };


      BOOST_REQUIRE_THROW(get_block_trace( 1, deadline ), deadline_exceeded);
   }

BOOST_AUTO_TEST_SUITE_END()
