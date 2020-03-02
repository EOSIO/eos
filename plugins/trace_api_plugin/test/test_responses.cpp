#define BOOST_TEST_MODULE trace_data_responses
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/trace_api_plugin/request_handler.hpp>
#include <eosio/trace_api_plugin/test_common.hpp>

using namespace eosio;
using namespace eosio::trace_api_plugin;
using namespace eosio::trace_api_plugin::test_common;

template<typename LogfileImpl>
struct placeholder_impl {
   explicit placeholder_impl( LogfileImpl&& logfile_impl )
   {
   }

   fc::variant get_block_trace( uint32_t block_height ) {
      return {};
   }
};

template<typename LogfileImpl>
using response_impl_type = request_handler<LogfileImpl>;

namespace {
   void to_kv_helper(const fc::variant& v, std::function<void(const std::string&, const std::string&)>&& append){
      if (v.is_object() ) {
         const auto& obj = v.get_object();
         static const std::string sep = ".";

         for (const auto& entry: obj) {
            to_kv_helper( entry.value(), [&append, &entry](const std::string& path, const std::string& value){
               append(sep + entry.key() + path, value);
            });
         }
      } else if (v.is_array()) {
         const auto& arr = v.get_array();
         for (size_t idx = 0; idx < arr.size(); idx++) {
            const auto& entry = arr.at(idx);
            to_kv_helper( entry, [&append, idx](const std::string& path, const std::string& value){
               append(std::string("[") + std::to_string(idx) + std::string("]") + path, value);
            });
         }
      } else if (!v.is_null()) {
         append("", v.as_string());
      }
   }

   auto to_kv(const fc::variant& v) {
      std::map<std::string, std::string> result;
      to_kv_helper(v, [&result](const std::string& k, const std::string& v){
         result.emplace(k, v);
      });
      return result;
   }
}

namespace std {
   /*
    * operator for printing to_kv entries
    */
   ostream& operator<<(ostream& os, const pair<string, string>& entry) {
      os << entry.first + "=" + entry.second;
      return os;
   }
}

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
            if (! fn(entry) ) {
               break;
            }
         }

         return cur;
      }


      response_test_fixture& fixture;
   };

   /**
    * TODO: initialize extraction implementation here with `mock_logfile_provider` as template param
    */
   response_test_fixture()
   : response_impl(mock_logfile_provider(*this), fc::time_point::now)
   {

   }

   fc::variant get_block_trace( uint32_t block_height, const fc::time_point& deadline = fc::time_point::maximum() ) {
      return response_impl.get_block_trace( block_height, deadline );
   }

   // fixture data and methods
   std::vector<metadata_log_entry> metadata_log = {};
   std::map<uint64_t, fc::static_variant<std::exception_ptr, data_log_entry>> data_log = {};

   response_impl_type<mock_logfile_provider> response_impl;

};

BOOST_AUTO_TEST_SUITE(trace_responses)
   BOOST_FIXTURE_TEST_CASE(basic_response, response_test_fixture)
   {
      metadata_log = decltype(metadata_log){
         block_entry_v0 { "b000000000000000000000000000000000000000000000000000000000000001"_h, 1, 0 }
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


BOOST_AUTO_TEST_SUITE_END()
