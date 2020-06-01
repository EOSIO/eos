#include <eosio/resource_monitor_plugin/system_file_space_provider.hpp>

namespace bfs = boost::filesystem;

namespace eosio::resource_monitor {
   int system_file_space_provider::get_stat(const char *path, struct stat *buf) const {
      return stat(path, buf);
   }

   bfs::space_info system_file_space_provider::get_space(const bfs::path& p, boost::system::error_code& ec) const {
      return bfs::space(p, ec);
   }

   using bfs::directory_iterator;
}
