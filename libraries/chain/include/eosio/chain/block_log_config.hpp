#pragma once
#include <boost/filesystem/path.hpp>

namespace eosio {
namespace chain {
    
namespace bfs = boost::filesystem;

struct block_log_config {
   bfs::path log_dir;
   bfs::path retained_dir;
   bfs::path archive_dir;
   uint32_t  stride                  = UINT32_MAX;
   uint32_t  max_retained_files      = UINT32_MAX;
   bool      fix_irreversible_blocks = false;
};

} // namespace chain
} // namespace eosio
