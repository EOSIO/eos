/**
 *  @file
 *  @copyright defined in eosio/LICENSE.txt
 */
#include "subcommand.hpp"
#include <eosio/chain/intrinsic_debug_log.hpp>

#include <fc/io/json.hpp>
#include <fc/filesystem.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <cstring>

using namespace eosio;
using namespace eosio::chain;

namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;

struct intrinsic_log {
   intrinsic_log()
   {}

   fc::optional<intrinsic_debug_log> log;
};

struct print_block_subcommand
   :  public subcommand<
         print_block_subcommand,
         subcommand_style::terminal_subcommand_with_positional_arguments
      >
{
   static constexpr const char* name = "block";

   static std::string get_description() {
      return "Print a specific block";
   }

   void set_options( bpo::options_description& options )const {
      options.add_options()
         ("output-file,o", bpo::value<bfs::path>(),
          "the file to write the output to (absolute or relative path).  If not specified then output is to stdout.")
      ;
   }

   bpo::positional_options_description
   get_positional_options( bpo::options_description& options, positional_descriptions& pos_desc )const {
      options.add_options()
         ("block-num", bpo::value<uint32_t>(),
          "the block number to print")
      ;

      std::array<std::string, 1> pos_args = {
         "block-num"
      };

      return build_positional_descriptions( pos_args, options, pos_desc );
   }
};

struct print_blocks_subcommand
   :  public subcommand<
         print_blocks_subcommand,
         subcommand_style::terminal_subcommand_without_positional_arguments
      >
{
   static constexpr const char* name = "blocks";

   static std::string get_description() {
      return "Print a range of blocks";
   }

   void set_options( bpo::options_description& options )const {
      options.add_options()
         ("output-file,o", bpo::value<bfs::path>(),
          "the file to write the output to (absolute or relative path).  If not specified then output is to stdout.")
         ("first,f", bpo::value<uint32_t>()->default_value(0),
          "the first block number to print")
         ("last,l", bpo::value<uint32_t>()->default_value(std::numeric_limits<uint32_t>::max()),
          "the first block number to print")
      ;
   }

   void initialize( bpo::variables_map&& vm ) {
      if( vm.count( "first" ) ) {
         first_block_num = vm.at( "first" ).as<uint32_t>();
      }

      if( vm.count( "last" ) ) {
         last_block_num = vm.at( "last" ).as<uint32_t>();
         FC_ASSERT( last_block_num >= first_block_num, "invalid range" );
      }
   }

   uint32_t first_block_num = 0;
   uint32_t last_block_num = 0;
};

struct print_subcommand
   :  public subcommand<
         print_subcommand,
         subcommand_style::contains_subcommands,
         print_block_subcommand,
         print_blocks_subcommand
      >
{
   static constexpr const char* name = "print";

   static std::string get_description() {
      return "Print blocks in a log file";
   }

   void set_options( bpo::options_description& options )const {
      options.add_options()
         ("no-pretty-print", bpo::bool_switch()->default_value(false), "avoid pretty printing")
      ;
   }
};

struct diff_subcommand
   :  public subcommand<
         diff_subcommand,
         subcommand_style::terminal_subcommand_with_positional_arguments
      >
{
   static constexpr const char* name = "diff";

   static std::string get_description() {
      return "Find the differences between two log files";
   }

   void set_options( bpo::options_description& options )const {
      options.add_options()
         ("start-block", bpo::value<uint32_t>()->default_value(0),
          "ignore any differences prior to this block")
      ;
   }

   bpo::positional_options_description
   get_positional_options( bpo::options_description& options, positional_descriptions& pos_desc )const {
      options.add_options()
         ("first-log", bpo::value<bfs::path>(),
         "path to the first log file to compare")
         ("second-log", bpo::value<bfs::path>(),
         "path to the second log file to compare")
      ;

      std::array<std::string, 2> pos_args = {
         "first-log",
         "second-log"
      };

      return build_positional_descriptions( pos_args, options, pos_desc );
   }
};

struct root_command
   : public subcommand<
      root_command,
      subcommand_style::contains_subcommands,
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
         ("help,h", bpo::bool_switch(&print_help)->default_value(false), "Print this help message and exit.")
         ("no-detail", bpo::bool_switch()->default_value(false), "Temporarily added for testing purposes.")
      ;
   }

   bool print_help = false;
};

int main( int argc, const char** argv ) {
   try {
      root_command rc;
      auto res = parse_program_options_extended( rc, argc, argv, std::cout, &std::cerr );

      if( std::holds_alternative<parse_failure>( res ) ) {
         FC_ASSERT( false, "parse error occurred" );
      } else if( std::holds_alternative<autocomplete_failure>( res ) ) {
         return -1;
      } else if( std::holds_alternative<autocomplete_success>( res ) ) {
         return 0;
      }

      const auto& pm = std::get<parse_metadata>( res );

      if( argc == 1 || rc.print_help ) {
         print_subcommand_help( std::cout, bfs::path( argv[0] ).filename().generic_string(), pm );
         return 0;
      }

      std::ios::sync_with_stdio(false); // for potential performance boost for large log files



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
