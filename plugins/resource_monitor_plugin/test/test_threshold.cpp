#define BOOST_TEST_MODULE threshold
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/resource_monitor_plugin/file_space_handler.hpp>

using namespace eosio;
using namespace eosio::resource_monitor;
using namespace boost::system;

struct threshold_fixture {
   struct mock_space_provider {
      mock_space_provider(threshold_fixture& fixture)
      :fixture(fixture)
      {}

      int get_stat(const char *path, struct stat *buf) const {
         return fixture.mock_get_stat(path, buf);
      }

      bfs::space_info get_space(const bfs::path& p, boost::system::error_code& ec) const {
         return fixture.mock_get_space(p, ec);
      }

      threshold_fixture& fixture;
   };

   boost::asio::io_context ctx;

   using file_space_handler_t = file_space_handler<mock_space_provider>;
   threshold_fixture()
   : space_handler(mock_space_provider(*this), ctx)
   {
   }

   void add_file_system(const bfs::path& path_name) {
      space_handler.add_file_system(path_name);
   }

   void set_threshold(uint32_t threshold, uint32_t warning_threshold) {
      space_handler.set_threshold(threshold, warning_threshold);
   }

   bool is_threshold_exceeded() const {
      return space_handler.is_threshold_exceeded();
   }

   void set_shutdown_on_exceeded(bool shutdown_on_exceeded) {
      space_handler.set_shutdown_on_exceeded(shutdown_on_exceeded);
   }

   bool test_threshold_common(std::map<bfs::path, uintmax_t>& available, std::map<bfs::path, int>& dev, uint32_t warning_threshold=75)
   {
      mock_get_space = [available]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = available[p];

         return rc;
      };

      mock_get_stat = [dev]( const char *path, struct stat *buf ) mutable -> int {
         bfs::path name = path;
         buf->st_dev = dev[name];

         return 0;
      };

      set_threshold(80, warning_threshold);
      set_shutdown_on_exceeded(true);

      for (auto i = 0; i < available.size(); i++) {
        add_file_system("/test" + std::to_string(i));
      }

      return is_threshold_exceeded();
   }

   // fixture data and methods
   std::function<bfs::space_info(const bfs::path& p, boost::system::error_code& ec)> mock_get_space;
   std::function<int(const char *path, struct stat *buf)> mock_get_stat;

   file_space_handler_t space_handler;
};

BOOST_AUTO_TEST_SUITE(threshol_tests)
   BOOST_FIXTURE_TEST_CASE(equal_to_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 200000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(above_threshold_1_byte, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 199999}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(above_threshold_1000_byte, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 199000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(within_warning, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 249999}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(not_yet_warning, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 250001}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(below_threshold_1_byte, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 200001}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(below_threshold_500_byte, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 200500}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(first_file_system_over_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 199999},
                                                 {"/test1", 200500}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1}};

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(second_file_system_over_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 300000},
                                                 {"/test1", 100000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1}};

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(no_file_system_over_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 300000},
                                                 {"/test1", 200000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(both_file_systems_over_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 150000},
                                                 {"/test1", 100000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1}};

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(one_of_three_over_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 300000},
                                                 {"/test1", 199999},
                                                 {"/test2", 250000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1},
                                                 {"/test2", 2}};

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(one_of_three_over_threshold_dup, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 100000},
                                                 {"/test1", 250000},
                                                 {"/test2", 250000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1},  // dup
                                                 {"/test2", 1}}; // dup

      BOOST_TEST( test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(none_of_three_over_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 300000},
                                                 {"/test1", 200000},
                                                 {"/test2", 250000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1},
                                                 {"/test2", 2}};

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(none_of_three_over_threshold_dup, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 800000},
                                                 {"/test1", 550000},
                                                 {"/test2", 550000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0},
                                                 {"/test1", 1},  // dup
                                                 {"/test2", 1}}; // dup

      BOOST_TEST( !test_threshold_common(availables, devs) );
   }

   BOOST_FIXTURE_TEST_CASE(warning_threshold_equal_to_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 150000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_REQUIRE_THROW(test_threshold_common(availables, devs, 80), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(warning_threshold_greater_than_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 150000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_REQUIRE_THROW( test_threshold_common(availables, devs, 85), chain::plugin_config_exception );
   }

   BOOST_FIXTURE_TEST_CASE(warning_threshold_less_than_threshold, threshold_fixture)
   {
      std::map<bfs::path, uintmax_t> availables {{"/test0", 200000}};
      std::map<bfs::path, int>       devs       {{"/test0", 0}};

      BOOST_TEST( !test_threshold_common(availables, devs, 70) );
   }

   BOOST_FIXTURE_TEST_CASE(get_space_failure_in_middle, threshold_fixture)
   {
      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         if ( i == 3 ) {
            ec = boost::system::errc::make_error_code(errc::no_such_file_or_directory);
         } else {
            ec = boost::system::errc::make_error_code(errc::success);
         }

         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = 200500;

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80, 75);
      add_file_system("/test");

      auto expected_response = false;

      auto actual_response_0 = is_threshold_exceeded();
      auto actual_response_1 = is_threshold_exceeded();
      auto actual_response_2 = is_threshold_exceeded();
      auto actual_response_3 = is_threshold_exceeded();
      auto actual_response_4 = is_threshold_exceeded();
      auto actual_response_5 = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response_0);
      BOOST_TEST(expected_response == actual_response_1);
      BOOST_TEST(expected_response == actual_response_2);
      BOOST_TEST(expected_response == actual_response_3);
      BOOST_TEST(expected_response == actual_response_4);
      BOOST_TEST(expected_response == actual_response_5);
   }

BOOST_AUTO_TEST_SUITE_END()
