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

   void set_threshold(uint32_t threshold) {
      space_handler.set_threshold(threshold);
   }

   bool is_threshold_exceeded() const {
      return space_handler.is_threshold_exceeded();
   }

   // fixture data and methods
   std::function<bfs::space_info(const bfs::path& p, boost::system::error_code& ec)> mock_get_space;
   std::function<int(const char *path, struct stat *buf)> mock_get_stat;

   file_space_handler_t space_handler;
};

BOOST_AUTO_TEST_SUITE(threshol_tests)
   BOOST_FIXTURE_TEST_CASE(equal_to_threshold, threshold_fixture)
   {
      mock_get_space = []( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);
         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = 200000;
         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      add_file_system("/test");

      bool expected_response = false;
      bool actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(above_threshold_1_byte, threshold_fixture)
   {
      mock_get_space = []( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);
         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = 199999;
         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      add_file_system("/test");

      bool expected_response = true;
      bool actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(above_threshold_1000_byte, threshold_fixture)
   {
      mock_get_space = []( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);
         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = 199000;
         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      add_file_system("/test");

      bool expected_response = true;
      bool actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(below_threshold_1_byte, threshold_fixture)
   {
      mock_get_space = []( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);
         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = 200001;
         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      add_file_system("/test");

      bool expected_response = false;
      bool actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(below_threshold_500_byte, threshold_fixture)
   {
      mock_get_space = []( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);
         bfs::space_info rc;
         rc.capacity  = 1000000;
         rc.available = 200500;
         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      add_file_system("/test");

      bool expected_response = false;
      bool actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(first_file_system_over_threshold, threshold_fixture)
   {
      mock_get_space = [ ]( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if (p == "/test0") {
            rc.available = 199999;
         } else {
            rc.available = 200500;
         }

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         if (strcmp(path, "/test0") == 0) {
            buf->st_dev = 0;
         } else {
            buf->st_dev = 1;
         }

         return 0;
      };

      set_threshold(80);
      add_file_system("/test0");
      add_file_system("/test1");

      auto expected_response = true;
      auto actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(second_file_system_over_threshold, threshold_fixture)
   {
      mock_get_space = [ ]( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if (p == "/test0") {
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         if (strcmp(path, "/test0") == 0) {
            buf->st_dev = 0;
         } else {
            buf->st_dev = 1;
         }

         return 0;
      };

      set_threshold(80);
      add_file_system("/test0");
      add_file_system("/test1");

      auto expected_response = true;
      auto actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(no_file_system_over_threshold, threshold_fixture)
   {
      mock_get_space = [ ]( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if (p == "/test0") {
            rc.available = 300000;
         } else {
            rc.available = 200000;
         }

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         if (strcmp(path, "/test0") == 0) {
            buf->st_dev = 0;
         } else {
            buf->st_dev = 1;
         }

         return 0;
      };

      set_threshold(80);
      add_file_system("/test0");
      add_file_system("/test1");

      auto expected_response = false;
      auto actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
   }

   BOOST_FIXTURE_TEST_CASE(both_file_systems_over_threshold, threshold_fixture)
   {
      mock_get_space = [ ]( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if (p == "/test0") {
            rc.available = 150000;
         } else {
            rc.available = 100000;
         }

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         if (strcmp(path, "/test0") == 0) {
            buf->st_dev = 0;
         } else {
            buf->st_dev = 1;
         }

         return 0;
      };

      set_threshold(80);
      add_file_system("/test0");
      add_file_system("/test1");

      auto expected_response = true;
      auto actual_response = is_threshold_exceeded();

      BOOST_TEST(expected_response == actual_response);
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

      set_threshold(80);
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
