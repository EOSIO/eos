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

   void set_threshold(uint32_t threshold, uint32_t warning_threshold) {
      space_handler.set_threshold( threshold, warning_threshold );
   }

   void set_sleep_time(uint32_t sleep_time) {
      space_handler.set_sleep_time( sleep_time );
   }

   void set_shutdown_on_exceeded(bool shutdown_on_exceeded) {
      space_handler.set_shutdown_on_exceeded(shutdown_on_exceeded);
   }

   bool is_threshold_exceeded() {
      return space_handler.is_threshold_exceeded();
   }

   void space_monitor_loop() {
      return space_handler.space_monitor_loop();
   }

   bool test_loop_common(int num_loops, int interval)
   {
      mock_get_space = [ i = 0, num_loops ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
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

      set_threshold(80, 75);
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
      std::chrono::duration<double> test_duration = end - start;

      // For tests to be repeatable on any platforms under any loads,
      // particularly for longer runs,
      // we just make sure the test duration is longer than a margin
      // of theroretical duration. 
      bool finished_in_time = (test_duration >= std::chrono::duration<double>((num_loops - 1) * interval));

      return finished_in_time;
   }

   // fixture data and methods
   std::function<bfs::space_info(const bfs::path& p, boost::system::error_code& ec)> mock_get_space;
   std::function<int(const char *path, struct stat *buf)> mock_get_stat;

   file_space_handler_t space_handler;
};

BOOST_AUTO_TEST_SUITE(monitor_loop_tests)
   BOOST_FIXTURE_TEST_CASE(zero_loop, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(0, 1) );
   }

   BOOST_FIXTURE_TEST_CASE(one_loop_1_secs_interval, space_handler_fixture)
   {
     BOOST_TEST( test_loop_common(1, 1) );
   }

   BOOST_FIXTURE_TEST_CASE(two_loops_1_sec_interval, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(2, 1) );
   }

   BOOST_FIXTURE_TEST_CASE(ten_loops_1_sec_interval, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(10, 1) );
   }

   BOOST_FIXTURE_TEST_CASE(one_loop_5_secs_interval, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(1, 5) );
   }

   BOOST_FIXTURE_TEST_CASE(two_loops_5_sec_interval, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(2, 5) );
   }

   BOOST_FIXTURE_TEST_CASE(ten_loops_5_sec_interval, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(10, 5) );
   }

   BOOST_FIXTURE_TEST_CASE(one_hundred_twenty_loops_1_sec_interval, space_handler_fixture)
   {
      BOOST_TEST( test_loop_common(120, 1) );
   }

BOOST_AUTO_TEST_SUITE_END()
