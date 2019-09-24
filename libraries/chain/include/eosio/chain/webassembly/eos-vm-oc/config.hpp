#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>
#include <fc/reflect/reflect.hpp>

namespace eosio { namespace chain { namespace eosvmoc {

struct config {
   uint64_t       cache_size      = 1024u*1024u*1024u;
   uint64_t threads = 1;
};

}}}