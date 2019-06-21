#include "diff.hpp"

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/intrinsic_debug_log.hpp>

#include <ostream>

using namespace eosio;
using namespace eosio::chain;

void exec_diff( subcommand_dispatcher_base& dispatcher,
                const configuration::root& root,
                const configuration::diff& diff  )
{
   dispatcher.stream << "diff was called" << std::endl;
   idump((diff.first_log.generic_string())(diff.second_log.generic_string()));
}
