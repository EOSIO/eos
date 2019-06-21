#pragma once

#include "root.hpp"

namespace configuration {
   struct root;

   struct diff {
      bfs::path first_log;
      bfs::path second_log;
   };
}

void exec_diff( subcommand_dispatcher_base& dispatcher,
                const configuration::root& root,
                const configuration::diff& diff  );
