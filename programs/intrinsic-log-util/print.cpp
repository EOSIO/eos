#include "print.hpp"

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/intrinsic_debug_log.hpp>

#include <fc/io/json.hpp>

#include <ostream>

using namespace eosio;
using namespace eosio::chain;

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
   bool blocks_in_range = false;

   intrinsic_debug_log log( print.log_path );
   log.open( intrinsic_debug_log::open_mode::read_only );

   for( auto itr = log.begin_block(), end = log.end_block(); itr != end; ++itr ) {
      if( itr->block_num < blocks.first_block_num ) continue;

      if( itr->block_num > blocks.last_block_num ) break;

      if( blocks_in_range ) {
         dispatcher.stream << "," << std::endl;
      }

      blocks_in_range = true;

      dispatcher.stream << fc::json::to_pretty_string(*itr);
   }

   log.close();

   if( !blocks_in_range ) {
      dispatcher.stream << "the log did not contain any blocks in the specified range";
   }

   dispatcher.stream << std::endl;
}
