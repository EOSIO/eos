#pragma once

#include <sys/stat.h>
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace eosio::resource_monitor {
   class system_file_space_provider {
   public:
      system_file_space_provider()
      {
      }

      // Wrapper for Linux stat
      int get_stat(const char *path, struct stat *buf) const;

      // Wrapper for boost file system space
      bfs::space_info get_space(const bfs::path& p, boost::system::error_code& ec) const;
   };
}
