#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

namespace eosio { namespace chain { namespace eosvmoc {

enum map_mode {
   mapped,
   heap,
   locked
};

struct config {
   uint64_t       cache_size      = 1024u*1024u*1024u;
   map_mode       cache_map_mode  = map_mode::mapped;
   std::vector<std::string> cache_hugepage_paths;
};

std::istream& operator>>(std::istream& in, map_mode& runtime);
std::ostream& operator<<(std::ostream& osm, map_mode m);

}}}