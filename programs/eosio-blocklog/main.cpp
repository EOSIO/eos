#include <memory>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/config.hpp>
#include <eosio/chain/reversible_block_object.hpp>

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
   bool                             smoke_test = false;
   bool                             help = false;
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
   block_log block_logger(blocks_dir);
   const auto end = block_logger.read_head();
   EOS_ASSERT( end, block_log_exception, "No blocks found in block log" );
   EOS_ASSERT( end->block_num() > 1, block_log_exception, "Only one block found in block log" );

   //fix message below, first block might not be 1, first_block_num is not set yet
   ilog( "existing block log contains block num 1 through block num ${n}", ("n",end->block_num()) );

   optional<chainbase::database> reversible_blocks;
   try {
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
   } catch( const std::runtime_error& e ) {
      if( std::string(e.what()).find("database dirty flag set") != std::string::npos ) {
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
   signed_block_ptr next;
   fc::variant pretty_output;
   const fc::microseconds deadline = fc::seconds(10);
   auto print_block = [&](signed_block_ptr& next) {
      abi_serializer::to_variant(*next,
                                 pretty_output,
                                 []( account_name n ) { return optional<abi_serializer>(); },
                                 deadline);
      const auto block_id = next->id();
      const uint32_t ref_block_prefix = block_id._hash[1];
      const auto enhanced_object = fc::mutable_variant_object
                 ("block_num",next->block_num())
                 ("id", block_id)
                 ("ref_block_prefix", ref_block_prefix)
                 (pretty_output.get_object());
      fc::variant v(std::move(enhanced_object));
       if (no_pretty_print)
          fc::json::to_stream(*out, v, fc::json::stringify_large_ints_and_doubles);
       else
          *out << fc::json::to_pretty_string(v) << "\n";
   };
   bool contains_obj = false;
   while((block_num <= last_block) && (next = block_logger.read_block_by_num( block_num ))) {
      if (as_json_array && contains_obj)
         *out << ",";
      print_block(next);
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
         ("smoke-test", bpo::bool_switch(&smoke_test)->default_value(false),
          "Quick test that blocks.log and blocks.index are well formed and agree with each other.")
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

constexpr int blknum_offset{14};                      //offset from start of block to 4 byte block number
//this offset is valid for version 1 and 2 blocklogs, version is checked before using blknum_offset

//to derive blknum_offset==14 see block_header.hpp and note on disk struct is packed
//   block_timestamp_type timestamp;                  //bytes 0:3
//   account_name         producer;                   //bytes 4:11
//   uint16_t             confirmed;                  //bytes 12:13
//   block_id_type        previous;                   //bytes 14:45, low 4 bytes is big endian block number of previous block

struct trim_data {            //used by trim_blocklog_front(), trim_blocklog_end(), and smoke_test()
   trim_data(bfs::path block_dir);
   ~trim_data() {
      fclose(blk_in);
      fclose(ind_in);
   }
   void find_block_pos(uint32_t n);
   bfs::path block_file_name, index_file_name;        //full pathname for blocks.log and blocks.index
   uint32_t version = 0;                              //blocklog version (1 or 2)
   uint32_t first_block = 0;                          //first block in blocks.log
   uint32_t last_block = 0;                          //last block in blocks.log
   FILE* blk_in = nullptr;                            //C style files for reading blocks.log and blocks.index
   FILE* ind_in = nullptr;                            //C style files for reading blocks.log and blocks.index
   //we use low level file IO because it is distinctly faster than C++ filebuf or iostream
   uint64_t index_pos = 0;                            //filepos in blocks.index for block n, +8 for block n+1
   uint64_t fpos0 = 0;                                //filepos in blocks.log for block n and block n+1
   uint64_t fpos1 = 0;                                //filepos in blocks.log for block n and block n+1
};


trim_data::trim_data(bfs::path block_dir) {
   report_time rt("trimming log");
   using namespace std;
   block_file_name = block_dir / "blocks.log";
   index_file_name = block_dir / "blocks.index";
   blk_in = FOPEN(block_file_name.c_str(), "rb");
   EOS_ASSERT( blk_in != nullptr, block_log_not_found, "cannot read file ${file}", ("file",block_file_name.string()) );
   ind_in = FOPEN(index_file_name.c_str(), "rb");
   EOS_ASSERT( ind_in != nullptr, block_log_not_found, "cannot read file ${file}", ("file",index_file_name.string()) );
   auto size = fread((void*)&version,sizeof(version), 1, blk_in);
   EOS_ASSERT( size == 1, block_log_unsupported_version, "invalid format for file ${file}", ("file",block_file_name.string()));
   cout << "block log version= " << version << '\n';
   EOS_ASSERT( version == 1 || version == 2, block_log_unsupported_version, "block log version ${v} is not supported", ("v",version));
   if (version == 1) {
      first_block = 1;
   }
   else {
      size = fread((void *) &first_block, sizeof(first_block), 1, blk_in);
      EOS_ASSERT(size == 1, block_log_exception, "invalid format for file ${file}",
                 ("file", block_file_name.string()));
   }
   cout << "first block= " << first_block << '\n';
   const auto status = fseek(ind_in, 0, SEEK_END);                //get length of blocks.index (gives number of blocks)
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} end", ("file", index_file_name.string()) );
   const uint64_t file_end = ftell(ind_in);                //get length of blocks.index (gives number of blocks)
   last_block = first_block + file_end/sizeof(uint64_t) - 1;
   cout << "last block=  " << last_block << '\n';
   rt.report();
}

void trim_data::find_block_pos(uint32_t n) {
   //get file position of block n from blocks.index then confirm block n is found in blocks.log at that position
   //sets fpos0 and fpos1, throws exception if block at fpos0 is not block n
   report_time rt("finding block position");
   using namespace std;
   index_pos = sizeof(uint64_t) * (n - first_block);
   auto status = fseek(ind_in, index_pos, SEEK_SET);
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file for block ${b}", ("file", index_file_name.string())("pos", index_pos)("b",n) );
   const uint64_t pos = ftell(ind_in);
   EOS_ASSERT( pos == index_pos, block_log_exception, "cannot seek to ${file} entry for block ${b}", ("file", index_file_name.string())("b",n) );
   auto size = fread((void*)&fpos0, sizeof(fpos0), 1, ind_in);                   //filepos of block n
   EOS_ASSERT( size == 1, block_log_exception, "cannot read ${file} entry for block ${b}", ("file", index_file_name.string())("b",n) );
   size = fread((void*)&fpos1, sizeof(fpos1), 1, ind_in);                   //filepos of block n+1
   if (n != last_block)
      EOS_ASSERT( size == 1, block_log_exception, "cannot read ${file} entry for block ${b}, size=${size}", ("file", index_file_name.string())("b",n + 1)("size",size) );

   cout << "According to blocks.index:\n";
   cout << "    block " << n << " starts at position " << fpos0 << '\n';
   cout << "    block " << n + 1;

   if (n != last_block)
      cout << " starts at position " << fpos1 << '\n';
   else
      cout << " is past end\n";

   //read blocks.log and verify block number n is found at file position fpos0
   status = fseek(blk_in, fpos0 + blknum_offset, SEEK_SET);
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", block_file_name.string())("pos", fpos0 + blknum_offset) );
   const uint64_t block_offset_pos = ftell(blk_in);
   EOS_ASSERT( block_offset_pos == fpos0 + blknum_offset, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", block_file_name.string())("pos", fpos0 + blknum_offset) );
   uint32_t prior_blknum;
   size = fread((void*)&prior_blknum, sizeof(prior_blknum), 1, blk_in);     //read bigendian block number of prior block
   EOS_ASSERT( size == 1, block_log_exception, "cannot read prior block");
   const uint32_t bnum = endian_reverse_u32(prior_blknum) + 1;          //convert to little endian, add 1 since prior block
   cout << "At position " << fpos0 << " in " << index_file_name << " find block " << bnum << (bnum == n ? " as expected\n": " - not good!\n");
   EOS_ASSERT( bnum == n, block_log_exception, "${index} does not agree with ${blocks}", ("index", index_file_name.string())("blocks", block_file_name.string()) );
   rt.report();
}

int trim_blocklog_end(bfs::path block_dir, uint32_t n) {       //n is last block to keep (remove later blocks)
   report_time rt("trimming blocklog end");
   using namespace std;
   trim_data td(block_dir);
   cout << "\nIn directory " << block_dir << " will trim all blocks after block " << n << " from " << td.block_file_name << " and " << td.index_file_name << ".\n";
   if (n < td.first_block) {
      cerr << "All blocks are after block " << n << " so do nothing (trim_end would delete entire blocks.log)\n";
      return 1;
   }
   if (n >= td.last_block) {
      cerr << "There are no blocks after block " << n << " so do nothing\n";
      return 2;
   }
   td.find_block_pos(n);
   bfs::resize_file(td.block_file_name, td.fpos1);
   uint64_t index_end= td.index_pos + sizeof(uint64_t);             //advance past record for block n
   bfs::resize_file(td.index_file_name, index_end);
   cout << "blocks.index has been trimmed to " << index_end << " bytes\n";
   rt.report();
   return 0;
}

int trim_blocklog_front(bfs::path block_dir, uint32_t n) {        //n is first block to keep (remove prior blocks)
   report_time rt("trimming blocklog start");
   using namespace std;
   cout << "\nIn directory " << block_dir << " will trim all blocks before block " << n << " from blocks.log and blocks.index.\n";
   trim_data td(block_dir);
   if (n <= td.first_block) {
      cerr << "There are no blocks before block " << n << " so do nothing\n";
      return 1;
   }
   if (n > td.last_block) {
      cerr << "All blocks are before block " << n << " so do nothing (trim_front would delete entire blocks.log)\n";
      return 2;
   }
   td.find_block_pos(n);

   constexpr uint32_t buf_len{1U<<24};                            //buf_len must be a power of 2
   auto buffer =  make_unique<char[]>(buf_len);                   //read big chunks of old blocks.log into this buffer
   char* buf =  buffer.get();

   bfs::path block_out_filename = block_dir / "blocks.out";
   FILE* blk_out = FOPEN(block_out_filename.c_str(), "wb");
   EOS_ASSERT( blk_out != nullptr, block_log_not_found, "cannot write ${file}", ("file", block_out_filename.string()) );

   //in version 1 file: version number, no first block number, rest of header length 0x6e, at 0x72 first block
   //in version 2 file: version number, first block number,    rest of header length 0x6e, at 0x76 totem,  at 0x7e first block
   *(uint32_t*)buf = 2;
   auto size = fwrite((void*)buf, sizeof(uint32_t), 1, blk_out);                           //write version number 2
   EOS_ASSERT( size == 1, block_log_exception, "blocks.out write fails" );
   size = fwrite((void*)&n, sizeof(n), 1, blk_out);                            //write first block number
   EOS_ASSERT( size == 1, block_log_exception, "blocks.out write fails" );

   const auto past_version_offset = (td.version == 1) ? 4 : 8;
   auto status = fseek(td.blk_in, past_version_offset, SEEK_SET) ;                //position past version number and maybe block number
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", td.block_file_name.string())("pos", past_version_offset) );
   const auto header_len = 0x6e;
   size = fread((void*)buf, header_len, 1, td.blk_in);
   EOS_ASSERT( size == 1, block_log_exception, "blocks.log read fails" );
   const auto totem_len = 8; //copy rest of header length 0x6e
   memset(buf + header_len, 0xff, totem_len);                                       //totem is 8 bytes of 0xff
   size = fwrite(buf, header_len + totem_len, 1, blk_out);                          //write header and totem
   EOS_ASSERT( size == 1, block_log_exception, "blocks.out write fails" );

   //pos_delta is the amount to subtract from every file position record in blocks.log because the blocks < n are removed
   uint64_t pos_delta = td.fpos0 - 0x7e;                           //bytes removed from the blocklog
   //bytes removed is file position of block n minus file position 'where file header ends'
   //even if version 1 blocklog 'where file header ends' is where the new version 2 file header of length 0x7e ends

   //read big chunks of blocks.log into buf, update the file position records, then write to blk_out
   status = fseek(td.blk_in, 0, SEEK_END);                     //get blocks.log file length
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", td.block_file_name.string())("pos", 0) );
   const uint64_t end = ftell(td.blk_in);
   uint32_t last_buf_len = (end - td.fpos0 >= buf_len)? buf_len : end - td.fpos0; //bytes to read from blk_in
   status = fseek(td.blk_in, -(uint64_t)last_buf_len, SEEK_END);        //pos is where read last buf from blk_in
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", td.block_file_name.string())("pos", last_buf_len) );
   uint64_t pos = ftell(td.blk_in);
   EOS_ASSERT( pos == end - last_buf_len, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", td.block_file_name.string())("pos", last_buf_len) );
   uint64_t did_read = fread((void*)buf, last_buf_len, 1, td.blk_in);           //read tail of blocks.log file into buf
   cout << "seek " << td.block_file_name << " to " << pos << " read " << last_buf_len << " bytes\n";//debug
   EOS_ASSERT( did_read == 1, block_log_exception, "${file} read fails", ("file", td.block_file_name.string()) );

   //prepare to write index_out_filename
   uint64_t total_fpos_len = (td.last_block + 1 - n) * sizeof(uint64_t);
   auto fpos_buffer = make_unique<uint64_t[]>(buf_len);
   uint64_t* fpos_list = fpos_buffer.get();                        //list of file positions, periodically write to blocks.index
   bfs::path index_out_filename = block_dir / "index.out";
   FILE* ind_out = FOPEN(index_out_filename.c_str(), "wb");
   EOS_ASSERT( ind_out != nullptr, block_log_not_found, "cannot write ${file}", ("file",index_out_filename.string()) );
   uint64_t last_fpos_len = total_fpos_len & ((uint64_t)buf_len - 1);//buf_len is a power of 2 so -1 creates low bits all 1
   if (!last_fpos_len)                                            //will write integral number of buf_len and one time write last_fpos_len
      last_fpos_len = buf_len;
   uint32_t blk_base = td.last_block + 1 - (last_fpos_len >> 3);     //first entry in fpos_list is for block blk_base
   cout << "******filling index buf blk_base " << blk_base << " last_fpos_len " << last_fpos_len << '\n';//debug

   //as we traverse linked list of blocks in buf (from end toward start), for each block we know this:
   int32_t index_start;                                           //buf index for block start
   int32_t index_end;                                             //buf index for block end == buf index for block start file position
   uint64_t file_pos;                                             //file position of block start
   uint32_t bnum;                                                 //block number

   //some code here is repeated in the loop below but we do it twice so can print a status message before the loop starts
   index_end = last_buf_len - 8;                                     //buf index where last block ends and the block file position starts
   file_pos = *(uint64_t*)(buf + index_end);                         //file pos of block start
   index_start = file_pos - pos;                                   //buf index for block start
   bnum = *(uint32_t*)(buf + index_start + blknum_offset);         //block number of previous block (is big endian)
   bnum = endian_reverse_u32(bnum) + 1;                              //convert from big endian to little endian and add 1
   EOS_ASSERT( bnum == td.last_block, block_log_exception, "last block from ${index} ${ind} != last block from ${block} ${log}", ("index", td.index_file_name.string())("ind", td.last_block)("block", td.block_file_name.string())("log", bnum) );

   cout << '\n';                                                  //print header for periodic update messages
   cout << setw(10) << "block" << setw(16) << "old file pos" << setw(16) << "new file pos" << '\n';
   cout << setw(10) << bnum << setw(16) << file_pos << setw(16) << file_pos-pos_delta << '\n';
   uint64_t last_file_pos = file_pos + 1;                            //used to check that file_pos is strictly decreasing
   constexpr uint32_t fk{4096};                                   //buffer shift that keeps buf disk block aligned

   for (;;) {        //invariant: at top of loop index_end and bnum are set and index_end is >= 0
      file_pos = *(uint64_t *) (buf + index_end);                 //get file pos of block start
      if (file_pos >= last_file_pos) {                            //check that file_pos decreases
         cout << '\n';
         cout << "file pos for block " << bnum + 1 << " is " << last_file_pos << '\n';
         cout << "file pos for block " << bnum << " is " << file_pos << '\n';
         EOS_ASSERT(file_pos < last_file_pos, block_log_exception, "${file} linked list of blocks is corrupt", ("file", td.index_file_name.string()) );
      }
      last_file_pos = file_pos;
      *(uint64_t *) (buf + index_end) = file_pos - pos_delta;     //update file position in buf
      fpos_list[bnum - blk_base] = file_pos - pos_delta;          //save updated file position for new index file
      index_start = file_pos - pos;                               //will go negative when pass first block in buf

      if ((bnum & 0xfffff) == 0)                                  //periodic progress indicator
         cout << setw(10) << bnum << setw(16) << file_pos << setw(16) << file_pos-pos_delta << '\n';

      if (bnum == blk_base) {                                     //if fpos_list is full write it to file
         cout << "****** bnum=" << bnum << " blk_base= " << blk_base << " so write index buf seek to " << (blk_base - n) * sizeof(uint64_t) << " write len " << last_fpos_len << '\n';//debug
         status = fseek(ind_out, (blk_base - n) * sizeof(uint64_t), SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", index_out_filename.string())("pos", last_buf_len) );
         size = fwrite((void *) fpos_list, last_fpos_len, 1, ind_out);       //write fpos_list to index file
         EOS_ASSERT( size == 1, block_log_exception, "${file} write fails", ("file", index_out_filename.string()) );
         last_fpos_len = buf_len;
         if (bnum == n) {                                         //if done with all blocks >= block n
            cout << setw(10) << bnum << setw(16) << file_pos << setw(16) << file_pos-pos_delta << '\n';
            EOS_ASSERT( index_start==0, block_log_exception, "block ${n} found with unexpected index_start ${s}", ("n",n) ("s",index_start));
            EOS_ASSERT( pos==td.fpos0, block_log_exception, "block ${n} found at unexpected file position ${p}", ("n",n) ("p",file_pos));
            status = fseek(blk_out, pos - pos_delta, SEEK_SET);
            EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", index_out_filename.string())("pos", pos - pos_delta) );
            size = fwrite((void*)buf, last_buf_len, 1, blk_out);
            EOS_ASSERT( size == 1, block_log_exception, "${file} write fails", ("file", block_out_filename.string()) );
            break;
         } else {
            blk_base -= (buf_len >> 3);
            cout << "******filling index buf blk_base " << blk_base << " fpos_len " << buf_len << '\n';//debug
         }
      }

      if (index_start <= 0) {                                     //if buf is ready to write (all file pos are updated)
         //cout << "index_start = " << index_start << " so write buf len " << last_buf_len << " bnum= " << bnum << '\n';//debug
         EOS_ASSERT(file_pos > td.fpos0, block_log_exception, "reverse scan of ${file} did not halt at block ${n}", ("file", td.block_file_name.string())("n",n));
         status = fseek(blk_out, pos - pos_delta, SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", block_out_filename.string())("pos", pos - pos_delta) );
         size = fwrite((void*)buf, last_buf_len, 1, blk_out);
         EOS_ASSERT( size == 1, block_log_exception, "${file} write fails", ("file", block_out_filename.string()) );
         last_buf_len = (pos - td.fpos0 > buf_len) ? buf_len : pos - td.fpos0;
         pos -= last_buf_len;
         index_start += last_buf_len;
         //cout << "seek blocks to " << pos << " read " << last_buf_len << " bytes\n";//debug
         status = fseek(td.blk_in, pos, SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", td.block_file_name.string())("pos", pos) );
         did_read = fread((void*)buf, last_buf_len, 1, td.blk_in);           //read next buf from blocklog
         EOS_ASSERT(did_read == 1, block_log_exception, "${file} read fails", ("file", td.index_file_name.string()));
      }

      index_end = index_start - sizeof(uint64_t);                 //move to prior block in buf
      --bnum;

      if (index_end < 0) {                                        //the file pos record straddles buf boundary
         cout << "****  index_end= " << index_end << " so save buf-4K len= " << last_buf_len-fk << " bnum= " << bnum << '\n';//debug
         status = fseek(blk_out, pos - pos_delta + fk, SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", index_out_filename.string())("pos", pos - pos_delta + fk) );
         size = fwrite((void*)(buf + fk), last_buf_len - fk, 1, blk_out);             //skip first 4K of buf write out the rest
         EOS_ASSERT( size == 1, block_log_exception, "${file} write fails", ("file", index_out_filename.string()) );
         last_buf_len = (pos - td.fpos0 >= buf_len - fk) ? buf_len - fk : pos - td.fpos0;   //bytes to read from blk_in
         memcpy(buf + last_buf_len, buf, fk);                     //move first 4K of buf after zone will read
         pos-= last_buf_len;
         index_end += last_buf_len;
         cout << "seek blocks to " << pos << " read " << last_buf_len << " bytes before saved 4k\n";//debug
         status = fseek(td.blk_in, pos, SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from end of file", ("file", td.block_file_name.string())("pos", pos) );
         did_read = fread(buf, last_buf_len, 1, td.blk_in);
         EOS_ASSERT(did_read == 1, block_log_exception, "${file} read fails", ("file", td.block_file_name.string()) );
         last_buf_len += fk;                                       //bytes in buf will eventually write to disk
      }
   }

   fclose(blk_out);
   fclose(ind_out);
   bfs::path old_log = block_dir / "old.log";
   bfs::path old_ind = block_dir / "old.index";
   rename(td.block_file_name, old_log);
   rename(td.index_file_name, old_ind);
   rename(block_out_filename, td.block_file_name);
   rename(index_out_filename, td.index_file_name);
   cout << "The new " << td.block_file_name << " and " << td.index_file_name << " files contain blocks " << n << " through " << td.last_block << '\n';
   cout << "The original (before trim front) files have been renamed to " << old_log << " and " << old_ind << ".\n";
   rt.report();
   return 0;
}


void smoke_test(bfs::path block_dir) {
   using namespace std;
   cout << "\nSmoke test of blocks.log and blocks.index in directory " << block_dir << '\n';
   trim_data td(block_dir);
   auto status = fseek(td.blk_in, -sizeof(uint64_t), SEEK_END);             //get last_block from blocks.log, compare to from blocks.index
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", td.block_file_name.string())("pos", sizeof(uint64_t)) );
   uint64_t file_pos;
   auto size = fread((void*)&file_pos, sizeof(uint64_t), 1, td.blk_in);
   EOS_ASSERT( size == 1, block_log_exception, "${file} read fails", ("file", td.block_file_name.string()) );
   status = fseek(td.blk_in, file_pos + blknum_offset, SEEK_SET);
   EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", td.block_file_name.string())("pos", file_pos + blknum_offset) );
   uint32_t bnum;
   size = fread((void*)&bnum, sizeof(uint32_t), 1, td.blk_in);
   EOS_ASSERT( size == 1, block_log_exception, "${file} read fails", ("file", td.block_file_name.string()) );
   bnum = endian_reverse_u32(bnum) + 1;                       //convert from big endian to little endian and add 1
   EOS_ASSERT( td.last_block == bnum, block_log_exception, "blocks.log says last block is ${lb} which disagrees with blocks.index", ("lb", bnum) );
   cout << "blocks.log and blocks.index agree on number of blocks\n";
   uint32_t delta = (td.last_block + 8 - td.first_block) >> 3;
   if (delta < 1)
      delta = 1;
   for (uint32_t n = td.first_block; ; n += delta) {
      if (n > td.last_block)
         n = td.last_block;
      td.find_block_pos(n);                                 //check block 'n' is where blocks.index says
      if (n == td.last_block)
         break;
   }
   cout << "\nno problems found\n";                         //if get here there were no exceptions
}

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
      if (blog.smoke_test) {
         smoke_test(vmap.at("blocks-dir").as<bfs::path>());
         return 0;
      }
      if (blog.trim_log) {
         if (blog.first_block == 0 && blog.last_block == std::numeric_limits<uint32_t>::max()) {
            std::cerr << "trim-blocklog does nothing unless specify first and/or last block.";
            return -1;
         }
         if (blog.last_block != std::numeric_limits<uint32_t>::max()) {
            if (trim_blocklog_end(vmap.at("blocks-dir").as<bfs::path>(), blog.last_block) != 0)
               return -1;
         }
         if (blog.first_block != 0) {
            if (trim_blocklog_front(vmap.at("blocks-dir").as<bfs::path>(), blog.first_block) != 0)
               return -1;
         }
         return 0;
      }
      if (blog.make_index) {
         const bfs::path blocks_dir = vmap.at("blocks-dir").as<bfs::path>();
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
      //else print blocks.log as JSON
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
