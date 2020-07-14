#define BOOST_TEST_MODULE add_file_system
#include <boost/test/included/unit_test.hpp>

#include <fc/variant_object.hpp>

#include <eosio/resource_monitor_plugin/file_space_handler.hpp>

using namespace eosio;
using namespace eosio::resource_monitor;
using namespace boost::system;

struct add_file_system_fixture {
   struct mock_space_provider {
      mock_space_provider(add_file_system_fixture& fixture)
      :fixture(fixture)
      {}

      int get_stat(const char *path, struct stat *buf) const {
         return fixture.mock_get_stat(path, buf);
      }

      bfs::space_info get_space(const bfs::path& p, boost::system::error_code& ec) const {
         return fixture.mock_get_space(p, ec);
      }

      add_file_system_fixture& fixture;
   };

   boost::asio::io_context ctx;

   using file_space_handler_t = file_space_handler<mock_space_provider>;
   add_file_system_fixture()
   : space_handler(mock_space_provider(*this), ctx)
   {
   }

   void add_file_system(const bfs::path& path_name) {
      space_handler.add_file_system(path_name);
   }

   void set_threshold(uint32_t threshold, uint32_t warning_threshold) {
      space_handler.set_threshold( threshold, warning_threshold );
   }

   bool is_threshold_exceeded() const {
      return space_handler.is_threshold_exceeded();
   }

   void test_add_file_systems_common(std::vector<int>& capacity, std::vector<int>& available, std::vector<int>& devs) {
      mock_get_space = [ i = 0, capacity, available ]( const bfs::path& p, boost::system::error_code& ec) mutable -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::success);

         bfs::space_info rc;
         rc.capacity  = capacity[i];
         rc.available = available[i];
         i++;

         return rc;
      };

      mock_get_stat = [ j = 0, devs ]( const char *path, struct stat *buf ) mutable -> int {
         buf->st_dev = devs[j];
         j++;

         return 0;
      };

      set_threshold(80, 75);

      for (auto k = 0; k < capacity.size(); k++) {
         add_file_system("/test" + std::to_string(k));
      }
   }

   // fixture data and methods
   std::function<bfs::space_info(const bfs::path& p, boost::system::error_code& ec)> mock_get_space;
   std::function<int(const char *path, struct stat *buf)> mock_get_stat;

   file_space_handler_t space_handler;
};

BOOST_AUTO_TEST_SUITE(space_handler_tests)
   BOOST_FIXTURE_TEST_CASE(get_stat_failure, add_file_system_fixture)
   {
      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         return 1;  // anything other than 0 is an error in stat
      };

      BOOST_REQUIRE_THROW(add_file_system("/test"), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(get_space_failure, add_file_system_fixture)
   {
      mock_get_space = []( const bfs::path& p, boost::system::error_code& ec) -> bfs::space_info {
         ec = boost::system::errc::make_error_code(errc::no_such_file_or_directory);
         bfs::space_info rc;
         return rc;
      };

      mock_get_stat = []( const char *path, struct stat *buf ) -> int {
         buf->st_dev = 0;
         return 0;
      };

      BOOST_REQUIRE_THROW(add_file_system("/test"), chain::plugin_config_exception);
   }

   BOOST_FIXTURE_TEST_CASE(different_file_systems, add_file_system_fixture)
   {
      std::vector<int> capacity  {1000000, 2000000, 3000000, 4000000};
      std::vector<int> available {500000,  1500000, 2500000, 3500000};
      std::vector<int> devs      {0,       1,       2,       3};

      // As long as no exceptions, it is considered success.
      BOOST_REQUIRE_NO_THROW(test_add_file_systems_common(capacity, available, devs));
   }

   BOOST_FIXTURE_TEST_CASE(same_file_system, add_file_system_fixture)
   {
      std::vector<int> capacity  {1000000, 2000000, 3000000, 4000000};
      std::vector<int> available {500000,  1500000, 2500000, 3500000};
      std::vector<int> devs      {0,       0,       0,       0};

      // As long as no exceptions, it is considered success.
      BOOST_REQUIRE_NO_THROW(test_add_file_systems_common(capacity, available, devs));
   }

   BOOST_FIXTURE_TEST_CASE(mixed_file_systems, add_file_system_fixture)
   {
      std::vector<int> capacity  {1000000, 2000000, 3000000, 4000000, 50000};
      std::vector<int> available {500000,  1500000, 2500000, 3500000, 20000};
      std::vector<int> devs      {0,       2,       2,       0,       3};

      // As long as no exceptions, it is considered success.
      BOOST_REQUIRE_NO_THROW(test_add_file_systems_common(capacity, available, devs));
   }

BOOST_AUTO_TEST_SUITE_END()
