#define BOOST_TEST_MODULE monitor_loop
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/resource_monitor_plugin/file_space_handler.hpp>

using namespace eosio;
using namespace eosio::resource_monitor;
using namespace boost::system;

struct space_handler_fixture {
   struct mock_space_provider {
      mock_space_provider(space_handler_fixture& fixture)
      :fixture(fixture)
      {}

      int get_stat(const char *path, struct stat *buf) const {
         return fixture.mock_get_stat(path, buf);
      }

      bfs::space_info get_space(const bfs::path& p, boost::system::error_code& ec) const {
         return fixture.mock_get_space(p, ec);
      }

      space_handler_fixture& fixture;
   };

   boost::asio::io_context ctx;

   using file_space_handler_t = file_space_handler<mock_space_provider>;
   space_handler_fixture()
   : space_handler(mock_space_provider( *this ), ctx)
   {
   }

   void add_file_system(const bfs::path& path_name) {
      space_handler.add_file_system( path_name );
   }

   void set_threshold(uint32_t threshold) {
      space_handler.set_threshold( threshold );
   }

   void set_sleep_time(uint32_t sleep_time) {
      space_handler.set_sleep_time( sleep_time );
   }

   void set_threshold_warning(uint32_t threshold_warning) {
      space_handler.set_threshold_warning( threshold_warning );
   }

   void set_shutdown_on_exceeded(bool shutdown_on_exceeded) {
      space_handler.set_shutdown_on_exceeded(shutdown_on_exceeded);
   }

   bool is_threshold_exceeded() const {
      return space_handler.is_threshold_exceeded();
   }

   int space_monitor_loop() {
      return space_handler.space_monitor_loop();
   }

   // fixture data and methods
   std::function<bfs::space_info(const bfs::path& p, boost::system::error_code& ec)> mock_get_space;
   std::function<int(const char *path, struct stat *buf)> mock_get_stat;

   file_space_handler_t space_handler;
};

BOOST_AUTO_TEST_SUITE(monitor_loop_tests)
   BOOST_FIXTURE_TEST_CASE(zero_loop, space_handler_fixture)
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
      set_threshold_warning(75);
      set_shutdown_on_exceeded(true);
      set_sleep_time(1);
      add_file_system("/test");

      auto expected_response = 0;
      auto actual_response = space_monitor_loop();

      BOOST_TEST(expected_response == actual_response);

   }

   BOOST_FIXTURE_TEST_CASE(one_loop_1_secs_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 1;
      auto constexpr interval  = 1;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_threshold_warning(75);
      set_shutdown_on_exceeded(true);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      // The thread should end within a tight limit.
      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }

   BOOST_FIXTURE_TEST_CASE(two_loops_1_sec_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 2;
      auto constexpr interval  = 1;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }

   BOOST_FIXTURE_TEST_CASE(ten_loops_1_sec_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 10;
      auto constexpr interval  = 1;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }

   BOOST_FIXTURE_TEST_CASE(one_loop_5_secs_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 1;
      auto constexpr interval  = 5;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }

   BOOST_FIXTURE_TEST_CASE(two_loops_5_sec_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 2;
      auto constexpr interval  = 5;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }

   BOOST_FIXTURE_TEST_CASE(ten_loops_5_sec_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 10;
      auto constexpr interval  = 5;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }

   BOOST_FIXTURE_TEST_CASE(one_loop_59_sec_interval, space_handler_fixture)
   {
      auto constexpr num_loops = 1;
      auto constexpr interval  = 59;

      mock_get_space = [ i = 0 ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = 1000000;

         if ( i < num_loops + 1 ) {  // "+ 1" for the get_space in add_file_system
            rc.available = 300000;
         } else {
            rc.available = 100000;
         }

         i++;

         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      set_threshold(80);
      set_sleep_time(interval);
      add_file_system("/test");

      auto start = std::chrono::system_clock::now();

      auto monitor_thread = std::thread( [this] {
         space_monitor_loop();
         ctx.run();
      });

      monitor_thread.join();

      auto end = std::chrono::system_clock::now();
      std::chrono::duration<double> diff = end - start;

      bool finished_in_time = (diff >= std::chrono::duration<double>(num_loops * interval) && diff <= std::chrono::duration<double>(num_loops * interval + 1)) ? true : false;

      BOOST_TEST(finished_in_time == true);
   }


BOOST_AUTO_TEST_SUITE_END()
