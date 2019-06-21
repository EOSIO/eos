#pragma once

#include "root.hpp"

#include <eosio/chain/types.hpp>

namespace configuration {
   struct print {
      bfs::path log_path;
      bool      no_pretty_print = false;
   };

   struct print_block {
      uint32_t                                        block_num = 0;
      std::vector<eosio::chain::transaction_id_type>  trxs_filter;
   };

   struct print_blocks {
      uint32_t first_block_num = 0;
      uint32_t last_block_num = std::numeric_limits<uint32_t>::max();
   };

}

void exec_print_block(  subcommand_dispatcher_base& dispatcher,
                        const configuration::root& root,
                        const configuration::print& print,
                        const configuration::print_block&  block );

void exec_print_blocks( subcommand_dispatcher_base& dispatcher,
                        const configuration::root& root,
                        const configuration::print& print,
                        const configuration::print_blocks& blocks );
