/**
 *  @file
 *  @copyright defined in eosio/LICENSE.txt
 */
#include "root.hpp"
#include "diff.hpp"
#include "print.hpp"

#include "cli_parser.hpp"

#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <cstring>
#include <iostream>

using namespace eosio;
using namespace eosio::chain;

namespace bpo = boost::program_options;

struct print_block_subcommand
   :  public configuration::print_block,
      public subcommand<
         print_block_subcommand,
         subcommand_style::terminal_subcommand_with_positional_arguments
      >
{
   static constexpr const char* name = "block";

   static std::string get_description() {
      return "Print a specific block";
   }

   void set_options( bpo::options_description& options ) {
      options.add_options()
         ("output-file,o", bpo::value<bfs::path>(),
          "the file to write the output to (absolute or relative path).  If not specified then output is to stdout.")
      ;
   }

   bpo::positional_options_description
   get_positional_options( bpo::options_description& options, cli_parser::positional_descriptions& pos_desc ) {
      options.add_options()
         ("block-num", bpo::value<uint32_t>(&block_num),
          "the block number to print")
         ("trx-id", bpo::value<std::vector<std::string>>(),
          "ID of transaction to print (optional)")
      ;

      return cli_parser::build_positional_descriptions( options, pos_desc, { "block-num" }, {}, "trx-id" );
   }

   void initialize( bpo::variables_map&& vm ) {
      if( vm.count( "trx-id" ) ) {
         for( const auto& trx_id : vm.at( "trx-id" ).as<std::vector<std::string>>() ) {
            try {
               trxs_filter.emplace_back( fc::variant( trx_id ).as<transaction_id_type>() );
            } EOS_RETHROW_EXCEPTIONS( fc::exception, "invalid transaction id: ${trx_id}", ("trx_id", trx_id) )
         }
      }
   }
};

struct print_blocks_subcommand
   :  public configuration::print_blocks,
      public subcommand<
         print_blocks_subcommand,
         subcommand_style::terminal_subcommand_without_positional_arguments
      >
{
   static constexpr const char* name = "blocks";

   static std::string get_description() {
      return "Print a range of blocks";
   }

   void set_options( bpo::options_description& options ) {
      options.add_options()
         ("output-file,o", bpo::value<bfs::path>(),
          "the file to write the output to (absolute or relative path).  If not specified then output is to stdout.")
         ("first,f", bpo::value<uint32_t>(&first_block_num)->default_value(first_block_num),
          "the first block number to print")
         ("last,l", bpo::value<uint32_t>()->default_value(last_block_num),
          "the first block number to print")
      ;
   }

   void initialize( bpo::variables_map&& vm ) {
      if( vm.count( "last" ) ) {
         last_block_num = vm.at( "last" ).as<uint32_t>();
         FC_ASSERT( last_block_num >= first_block_num, "invalid range" );
      }
   }
};

struct print_subcommand
   :  public configuration::print,
      public subcommand<
         print_subcommand,
         subcommand_style::only_contains_subcommands,
         print_block_subcommand,
         print_blocks_subcommand
      >
{
   static constexpr const char* name = "print";

   static std::string get_description() {
      return "Print blocks in a log file";
   }

   void set_options( bpo::options_description& options ) {
      options.add_options()
         ("log-file", bpo::value<bfs::path>(), "path to the log file (required)")
         ("no-pretty-print", bpo::bool_switch()->default_value(no_pretty_print), "avoid pretty printing")
      ;
   }

   void initialize( bpo::variables_map&& vm ) {
      FC_ASSERT( vm.count( "log-file" ) > 0, "path to log file must be provided (use --log-file)" );
      log_path = bfs::canonical( vm.at( "log-file" ).as<bfs::path>() );
   }
};

struct diff_subcommand
   :  public configuration::diff,
      public subcommand<
         diff_subcommand,
         subcommand_style::terminal_subcommand_with_positional_arguments
      >
{
   static constexpr const char* name = "diff";

   static std::string get_description() {
      return "Find the differences between two log files";
   }

   void set_options( bpo::options_description& options ) {
      options.add_options()
         ("start-block", bpo::value<uint32_t>()->default_value(0),
          "ignore any differences prior to this block")
      ;
   }

   bpo::positional_options_description
   get_positional_options( bpo::options_description& options, cli_parser::positional_descriptions& pos_desc ) {
      options.add_options()
         ("first-log", bpo::value<bfs::path>(&first_log),
         "path to the first log file to compare")
         ("second-log", bpo::value<bfs::path>(&second_log),
         "path to the second log file to compare")
      ;

      return cli_parser::build_positional_descriptions( options, pos_desc, {
         "first-log", "second-log"
      } );
   }

   void initialize( bpo::variables_map&& vm ) {
      first_log  = bfs::canonical( first_log );
      second_log = bfs::canonical( second_log );
   }
};

struct root_command
   :  public configuration::root,
      public subcommand<
         root_command,
         subcommand_style::only_contains_subcommands,
         print_subcommand,
         diff_subcommand
      >
{
   static constexpr const char* name = "Generic";

   static std::string get_description() {
      return "Utility to inspect intrinsic log files";
   }

   void set_options( bpo::options_description& options ) {
      options.add_options()
         ("help,h", bpo::bool_switch(&print_help)->default_value(print_help), "Print this help message and exit.")
         ("no-detail", bpo::bool_switch()->default_value(no_detail), "Temporarily added for testing purposes.")
      ;
   }
};

class subcommand_dispatcher : public subcommand_dispatcher_base {
public:
   subcommand_dispatcher( std::ostream& stream )
   :subcommand_dispatcher_base( stream )
   {}

   void operator()( const root_command& root, const diff_subcommand& diff ) {
      exec_diff( *this, root, diff );
   }

   void operator()( const root_command& root, const print_subcommand& print, const print_blocks_subcommand& blocks ) {
      exec_print_blocks( *this, root, print, blocks );
   }

   void operator()( const root_command& root, const print_subcommand& print, const print_block_subcommand& block ) {
      exec_print_block( *this, root, print, block );
   }
};

int main( int argc, const char** argv ) {
   try {
      root_command rc;
      auto parse_result = cli_parser::parse_program_options_extended( rc, argc, argv, std::cout, &std::cerr );

      if( std::holds_alternative<cli_parser::parse_failure>( parse_result ) ) {
         FC_ASSERT( false, "parse error occurred" );
      } else if( std::holds_alternative<cli_parser::autocomplete_failure>( parse_result ) ) {
         return -1;
      } else if( std::holds_alternative<cli_parser::autocomplete_success>( parse_result ) ) {
         return 0;
      }

      const auto& pm = std::get<cli_parser::parse_metadata>( parse_result );

      if( argc == 1 || rc.print_help ) {
         cli_parser::print_subcommand_help( std::cout, bfs::path( argv[0] ).filename().generic_string(), pm );
         return 0;
      }

      std::ios::sync_with_stdio(false); // for potential performance boost for large log files

      subcommand_dispatcher d( std::cout );
      auto dispatch_result = cli_parser::dispatch( d, rc );

      FC_ASSERT( dispatch_result.first != cli_parser::dispatch_status::no_handler_for_terminal_subcommand,
                 "The ${subcommand} sub-command has not been implemented yet.",
                 ("subcommand", dispatch_result.second)
      );

      if( dispatch_result.first == cli_parser::dispatch_status::no_handler_for_nonterminal_subcommand ) {
         std::cout << "\033[0;31m"
                   << "The sub-command "
                   << "\033[1;31m" << dispatch_result.second << "\033[0;31m"
                   << " cannot be called by itself."
                   << "\033[0m"
                   << std::endl;
         cli_parser::print_subcommand_help( std::cout, bfs::path( argv[0] ).filename().generic_string(), pm );
         return -1;
      }

   } catch( const fc::exception& e ) {
      elog( "${e}", ("e", e.to_detail_string()));
      return -1;
   } catch( const boost::exception& e ) {
      elog("${e}", ("e",boost::diagnostic_information(e)));
      return -1;
   } catch( const std::exception& e ) {
      elog("${e}", ("e",e.what()));
      return -1;
   } catch( ... ) {
      elog("unknown exception");
      return -1;
   }

   return 0;
}
