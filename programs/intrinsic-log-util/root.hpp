#pragma once

#include <iosfwd>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace configuration {
   struct root {
      bool print_help = false;
      bool no_detail = false;
   };
}

class subcommand_dispatcher_base {
public:
   subcommand_dispatcher_base( std::ostream& stream )
   :stream( stream )
   {}

public:
   std::ostream& stream;
};
