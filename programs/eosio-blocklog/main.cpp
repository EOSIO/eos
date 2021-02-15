#include <memory>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/reversible_block_object.hpp>

#include <eosio/state_history/log.hpp>

#include <fc/io/json.hpp>
#include <fc/filesystem.hpp>
#include <fc/variant.hpp>
#include <fc/bitutil.hpp>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include <chrono>

#ifndef _WIN32
#define FOPEN(p, m) fopen(p, m)
#else
#define CAT(s1, s2) s1 ## s2
#define PREL(s) CAT(L, s)
#define FOPEN(p, m) _wfopen(p, PREL(m))
#endif

using namespace eosio::chain;
namespace bfs = boost::filesystem;
namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

struct blocklog {
   blocklog()
   {}

   void read_log();
   void set_program_options(options_description& cli);
   void initialize(const variables_map& options);

   bfs::path                        blocks_dir;
   bfs::path                        output_file;
   uint32_t                         first_block = 0;
   uint32_t                         last_block = std::numeric_limits<uint32_t>::max();
   bool                             no_pretty_print = false;
   bool                             as_json_array = false;
   bool                             make_index = false;
   bool                             trim_log = false;
   bool                             fix_irreversible_blocks = false;
   bool                             smoke_test = false;
   bool                             prune_transactions = false;
   bool                             help               = false;
   bool                             extract_blocklog   = false;
   uint32_t                         blocklog_split_stride   = 0;
};

struct report_time {
    report_time(std::string desc)
    : _start(std::chrono::high_resolution_clock::now())
    , _desc(desc) {
    }

    void report() {
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - _start).count() / 1000;
        ilog("eosio-blocklog - ${desc} took ${t} msec", ("desc", _desc)("t", duration));
    }

    const std::chrono::high_resolution_clock::time_point _start;
    const std::string                                    _desc;
};

void blocklog::read_log() {
   report_time rt("reading log");
   
   block_log block_logger({ .log_dir = blocks_dir });
   const auto end = block_logger.head();
   EOS_ASSERT( end, block_log_exception, "No blocks found in block log" );
   EOS_ASSERT( end->block_num() > 1, block_log_exception, "Only one block found in block log" );

   //fix message below, first block might not be 1, first_block_num is not set yet
   ilog( "existing block log contains block num ${first} through block num ${n}",
         ("first",block_logger.first_block_num())("n",end->block_num()) );
   if (first_block < block_logger.first_block_num()) {
      first_block = block_logger.first_block_num();
   }

   std::optional<chainbase::database> reversible_blocks;
   try {
      ilog("opening reversible db");
      reversible_blocks.emplace(blocks_dir / config::reversible_blocks_dir_name, chainbase::database::read_only, config::default_reversible_cache_size);
      reversible_blocks->add_index<reversible_block_index>();
      const auto& idx = reversible_blocks->get_index<reversible_block_index,by_num>();
      auto first = idx.lower_bound(end->block_num());
      auto last = idx.rbegin();
      if (first != idx.end() && last != idx.rend())
         ilog( "existing reversible block num ${first} through block num ${last} ", ("first",first->get_block()->block_num())("last",last->get_block()->block_num()) );
      else {
         elog( "no blocks available in reversible block database: only block_log blocks are available" );
         reversible_blocks.reset();
      }
   } catch (const std::system_error&e) {
      if (chainbase::db_error_code::dirty == e.code().value()) {
         elog( "database dirty flag set (likely due to unclean shutdown): only block_log blocks are available" );
      } else {
         throw;
      }
   }

   std::ofstream output_blocks;
   std::ostream* out;
   if (!output_file.empty()) {
      output_blocks.open(output_file.generic_string().c_str());
      if (output_blocks.fail()) {
         std::ostringstream ss;
         ss << "Unable to open file '" << output_file.string() << "'";
         throw std::runtime_error(ss.str());
      }
      out = &output_blocks;
   }
   else
      out = &std::cout;

   if (as_json_array)
      *out << "[";
   uint32_t block_num = (first_block < 1) ? 1 : first_block;
   fc::variant pretty_output;
   const fc::microseconds deadline = fc::seconds(10);
   auto print_block = [&](auto& next) {
      abi_serializer::to_variant(*next,
                                 pretty_output,
                                 []( account_name n ) { return std::optional<abi_serializer>(); },
                                 abi_serializer::create_yield_function( deadline ));
      const auto block_id = next->calculate_id();
      const uint32_t ref_block_prefix = block_id._hash[1];
      const auto enhanced_object = fc::mutable_variant_object
                 ("block_num",next->block_num())
                 ("id", block_id)
                 ("ref_block_prefix", ref_block_prefix)
                 (pretty_output.get_object());
      fc::variant v(std::move(enhanced_object));
      if (no_pretty_print)
         *out << fc::json::to_string(v, fc::time_point::maximum());
      else
         *out << fc::json::to_pretty_string(v) << "\n";
   };
   bool contains_obj = false;
   while( block_num <= last_block ) {
      auto sb = block_logger.read_signed_block_by_num( block_num );
      if( !sb ) break;
      if (as_json_array && contains_obj)
         *out << ",";
      print_block(sb);
      ++block_num;
      contains_obj = true;
   }

   if (reversible_blocks) {
      const reversible_block_object* obj = nullptr;
      while( (block_num <= last_block) && (obj = reversible_blocks->find<reversible_block_object,by_num>(block_num)) ) {
         if (as_json_array && contains_obj)
            *out << ",";
         auto next = obj->get_block();
         print_block(next);
         ++block_num;
         contains_obj = true;
      }
   }

   if (as_json_array)
      *out << "]";
   rt.report();
}

void blocklog::set_program_options(options_description& cli)
{
   cli.add_options()
         ("blocks-dir", bpo::value<bfs::path>()->default_value("blocks"),
          "the location of the blocks directory (absolute path or relative to the current directory)")
          ("blocks-filebase", bpo::value<bfs::path>()->default_value("blocks"),
          "the name of the blocks log/index files without file extension (absolute path or relative to the current directory)."
          " This can only used for extract-blocklog.")
         ("state-history-dir", bpo::value<bfs::path>()->default_value("state-history"),
           "the location of the state-history directory (absolute path or relative to the current dir)")
         ("output-file,o", bpo::value<bfs::path>(),
          "the file to write the output to (absolute or relative path).  If not specified then output is to stdout.")
         ("first,f", bpo::value<uint32_t>(&first_block)->default_value(0),
          "the first block number to log or the first to keep if trim-blocklog")
         ("last,l", bpo::value<uint32_t>(&last_block)->default_value(std::numeric_limits<uint32_t>::max()),
          "the last block number to log or the last to keep if trim-blocklog")
         ("no-pretty-print", bpo::bool_switch(&no_pretty_print)->default_value(false),
          "Do not pretty print the output.  Useful if piping to jq to improve performance.")
         ("as-json-array", bpo::bool_switch(&as_json_array)->default_value(false),
          "Print out json blocks wrapped in json array (otherwise the output is free-standing json objects).")
         ("make-index", bpo::bool_switch(&make_index)->default_value(false),
          "Create blocks.index from blocks.log. Must give 'blocks-dir'. Give 'output-file' relative to current directory or absolute path (default is <blocks-dir>/blocks.index).")
         ("trim-blocklog", bpo::bool_switch(&trim_log)->default_value(false),
          "Trim blocks.log and blocks.index. Must give 'blocks-dir' and 'first and/or 'last'.")
         ("fix-irreversible-blocks", bpo::bool_switch(&fix_irreversible_blocks)->default_value(false),
          "When the existing block log is inconsistent with the index, allows fixing the block log and index files automatically - that is, "
          "it will take the highest indexed block if it is valid; otherwise it will repair the block log and reconstruct the index.")
         ("smoke-test", bpo::bool_switch(&smoke_test)->default_value(false),
          "Quick test that blocks.log and blocks.index are well formed and agree with each other.")
         ("block-num", bpo::value<uint32_t>()->default_value(0), "The block number which contains the transactions to be pruned")
         ("transaction,t", bpo::value<std::vector<std::string> >()->multitoken(), "The transaction id to be pruned")
         ("prune-transactions", bpo::bool_switch(&prune_transactions)->default_value(false),
          "Prune the context free data and signatures from specified transactions of specified block-num.")
         ("output-dir", bpo::value<bfs::path>()->default_value("."),
          "the output location for 'split-blocklog' or 'extract-blocklog'.")
         ("split-blocklog", bpo::value<uint32_t>(&blocklog_split_stride)->default_value(0),
          "split the block log file based on the stride and store the result in the specified 'output-dir'.")
         ("extract-blocklog", bpo::bool_switch(&extract_blocklog)->default_value(false),
          "Extract blocks from blocks.log and blocks.index and keep the original." 
          " Must give 'blocks-dir' or 'blocks-filebase','output-dir', 'first' and 'last'.")
         ("help,h", bpo::bool_switch(&help)->default_value(false), "Print this help message and exit.")
         ;
}

void blocklog::initialize(const variables_map& options) {
   try {
      auto bld = options.at( "blocks-dir" ).as<bfs::path>();
      if( bld.is_relative())
         blocks_dir = bfs::current_path() / bld;
      else
         blocks_dir = bld;

      if (options.count( "output-file" )) {
         bld = options.at( "output-file" ).as<bfs::path>();
         if( bld.is_relative())
            output_file = bfs::current_path() / bld;
         else
            output_file = bld;
      }
   } FC_LOG_AND_RETHROW()

}

int trim_blocklog_end(bfs::path block_dir, uint32_t n) {       //n is last block to keep (remove later blocks)
   report_time rt("trimming blocklog end");   
   int ret = block_log::trim_blocklog_end(block_dir, n);
   rt.report();
   return ret;
}

bool trim_blocklog_front(bfs::path block_dir, uint32_t n) {        //n is first block to keep (remove prior blocks)
   report_time rt("trimming blocklog start");
   const bool status = block_log::trim_blocklog_front(block_dir, block_dir / "old", n);
   rt.report();
   return status;
}

void fix_irreversible_blocks(bfs::path block_dir) {
   std::cout << "\nfix_irreversible_blocks of blocks.log and blocks.index in directory " << block_dir << '\n';
   block_log::config_type config;
   config.log_dir = block_dir;
   config.fix_irreversible_blocks = true;
   block_log block_logger(config);

   std::cout << "\nSmoke test of blocks.log and blocks.index in directory " << block_dir << '\n';
   block_log::smoke_test(block_dir, 0);
}

void smoke_test(bfs::path block_dir) {
   using namespace std;
   cout << "\nSmoke test of blocks.log and blocks.index in directory " << block_dir << '\n';
   block_log::smoke_test(block_dir, 0);
   cout << "\nno problems found\n"; // if get here there were no exceptions
}

template <typename Log>
int prune_transactions(const char* type, bfs::path dir, uint32_t block_num,
                       std::vector<transaction_id_type> unpruned_ids) {
   using namespace std;
   if (Log::exists(dir)) {
      Log log( { .log_dir = dir });
      log.prune_transactions(block_num, unpruned_ids);
      if (unpruned_ids.size()) {
         cerr << "block " << block_num << " in " << type << " does not contain the following transactions: ";
         copy(unpruned_ids.begin(), unpruned_ids.end(), ostream_iterator<string>(cerr, " "));
         cerr << "\n";
      }
   } else {
      cerr << "No " << type << " is found in " << dir.native() << "\n";
   }
   return unpruned_ids.size();
}

int prune_transactions(bfs::path block_dir, bfs::path state_history_dir, uint32_t block_num,
                       const std::vector<transaction_id_type>& ids) {

   if (block_num == 0 || ids.size() == 0) {
      std::cerr << "prune-transaction does nothing unless specify block-num and transaction\n";
      return -1;
   }

   using eosio::state_history_traces_log;
   return prune_transactions<block_log>("block log", block_dir, block_num, ids) +
          prune_transactions<state_history_traces_log>("state history traces log", state_history_dir, block_num, ids);
}

inline bfs::path operator+(bfs::path left, bfs::path right){return bfs::path(left)+=right;}


int main(int argc, char** argv) {
   std::ios::sync_with_stdio(false); // for potential performance boost for large block log files
   options_description cli ("eosio-blocklog command line options");
   try {
      blocklog blog;
      blog.set_program_options(cli);
      variables_map vmap;
      bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
      bpo::notify(vmap);

      if (blog.help) {
         cli.print(std::cerr);
         return 0;
      }

      const auto blocks_dir = vmap["blocks-dir"].as<bfs::path>();

      if (!blog.extract_blocklog && !block_log::exists(blocks_dir)) {
         std::cerr << "The specified blocks-dir must contain blocks.log and blocks.index files";
         return -1;
      }

      if (blog.smoke_test) {
         smoke_test(blocks_dir);
         return 0;
      }
      if (blog.fix_irreversible_blocks) {
          fix_irreversible_blocks(blocks_dir);
          return 0;
      }
      if (blog.trim_log) {
         if (blog.first_block == 0 && blog.last_block == std::numeric_limits<uint32_t>::max()) {
            std::cerr << "trim-blocklog does nothing unless specify first and/or last block.";
            return -1;
         }
         if (blog.last_block != std::numeric_limits<uint32_t>::max()) {
            if (trim_blocklog_end(blocks_dir, blog.last_block) != 0)
               return -1;
         }
         if (blog.first_block != 0) {
            if (!trim_blocklog_front(blocks_dir, blog.first_block))
               return -1;
         }
         return 0;
      }
      if (blog.make_index) {
         bfs::path out_file = blocks_dir / "blocks.index";
         const bfs::path block_file = blocks_dir / "blocks.log";

         if (vmap.count("output-file") > 0)
             out_file = vmap.at("output-file").as<bfs::path>();

         report_time rt("making index");
         const auto log_level = fc::logger::get(DEFAULT_LOGGER).get_log_level();
         fc::logger::get(DEFAULT_LOGGER).set_log_level(fc::log_level::debug);
         block_log::construct_index(block_file.generic_string(), out_file.generic_string());
         fc::logger::get(DEFAULT_LOGGER).set_log_level(log_level);
         rt.report();
         return 0;
      }
      

      if (blog.prune_transactions) {
         const auto  state_history_dir = vmap["state-history-dir"].as<bfs::path>();
         const auto  block_num         = vmap["block-num"].as<uint32_t>();
         const auto  ids               = vmap.count("transaction") ? vmap["transaction"].as<std::vector<string>>() : std::vector<string>{};
         
         report_time                 rt("prune transactions");
         int ret = prune_transactions(blocks_dir, state_history_dir, block_num, {ids.begin(), ids.end()});
         rt.report();
         return ret;
      }

      const auto output_dir = vmap["output-dir"].as<bfs::path>();

      if (blog.blocklog_split_stride != 0) {
         
         block_log::split_blocklog(blocks_dir, output_dir, blog.blocklog_split_stride);
         return 0;
      }

      if (blog.extract_blocklog) {

         if (blog.first_block == 0 && blog.last_block == std::numeric_limits<uint32_t>::max()) {
            std::cerr << "extract_blocklog does nothing unless specify first and/or last block.";
         }
         
         bfs::path blocks_filebase = vmap["blocks-filebase"].as<bfs::path>();
         if (blocks_filebase.empty() && !blocks_dir.empty()) {
            blocks_filebase = blocks_dir / "blocks";
         }

         bfs::path log_filename = blocks_filebase + ".log";
         bfs::path index_filename = blocks_filebase + ".index";

         if (!bfs::exists(log_filename) || !bfs::exists(index_filename)){
            std::cerr << "Both "<< log_filename << " and " << index_filename << " must exist";
            return -1;
         }

         block_log::extract_blocklog(log_filename, index_filename, output_dir, blog.first_block,
                                       blog.last_block - blog.first_block + 1);
         return 0;
      } 

      // else print blocks.log as JSON
      blog.initialize(vmap);
      blog.read_log();
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
