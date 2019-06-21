#include "print.hpp"

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/intrinsic_debug_log.hpp>

#include <ostream>

void exec_print_block(  subcommand_dispatcher_base& dispatcher,
                        const configuration::root& root,
                        const configuration::print& print,
                        const configuration::print_block&  block )
{
   dispatcher.stream << "print block was called" << std::endl;
   idump((print.log_path.generic_string())(block.block_num)(block.trxs_filter));
}

void exec_print_blocks( subcommand_dispatcher_base& dispatcher,
                        const configuration::root& root,
                        const configuration::print& print,
                        const configuration::print_blocks& blocks )
{
   dispatcher.stream << "print blocks was called" << std::endl;
   idump((print.log_path.generic_string())(blocks.first_block_num)(blocks.last_block_num));
}
