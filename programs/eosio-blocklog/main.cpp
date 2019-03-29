/**
 *  @file
 *  @copyright defined in eosio/LICENSE.txt
 */
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
   uint32_t                         first_block;
   uint32_t                         last_block;
   bool                             no_pretty_print;
   bool                             as_json_array;
   bool                             make_index;
   bool                             trim_log;
   bool                             smoke_test;
   bool                             help;
};

void blocklog::read_log() {
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
      if( std::string(e.what()) == "database dirty flag set" ) {
         elog( "database dirty flag set (likely due to unclean shutdown): only block_log blocks are available" );
      } else if( std::string(e.what()) == "database metadata dirty flag set" ) {
         elog( "database metadata dirty flag set (likely due to unclean shutdown): only block_log blocks are available" );
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
          "Create blocks.index from blocks.log. Must give 'blocks-dir'. Give 'output-file' relative to blocks-dir (default is blocks.index).")
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
      close(blk_in);
      close(ind_in);
   }
   void find_block_pos(uint32_t n);
   std::string block_file_name, index_file_name;      //full pathname for blocks.log and blocks.index
   uint32_t version;                                  //blocklog version (1 or 2)
   uint32_t first_block, last_block;                  //first and last block in blocks.log
   int blk_in, ind_in;                                //C style file descriptors for reading blocks.log and blocks.index
   //we use low level file IO because it is distinctly faster than C++ filebuf or iostream
   uint64_t index_pos;                                //filepos in blocks.index for block n, +8 for block n+1
   uint64_t fpos0, fpos1;                             //filepos in blocks.log for block n and block n+1
};


trim_data::trim_data(bfs::path block_dir) {
   using namespace std;
   block_file_name= (block_dir/"blocks.log").generic_string();
   index_file_name= (block_dir/"blocks.index").generic_string();
   blk_in = open(block_file_name.c_str(), O_RDONLY);
   EOS_ASSERT( blk_in>0, block_log_not_found, "cannot read file ${file}", ("file",block_file_name) );
   ind_in = open(index_file_name.c_str(), O_RDONLY);
   EOS_ASSERT( ind_in>0, block_log_not_found, "cannot read file ${file}", ("file",index_file_name) );
   read(blk_in,(char*)&version,sizeof(version));
   cout << "block log version= " << version << '\n';
   EOS_ASSERT( version==1 || version==2, block_log_unsupported_version, "block log version ${v} is not supported", ("v",version));
   if (version == 1)
      first_block= 1;
   else
      read(blk_in,(char*)&first_block,sizeof(first_block));
   cout << "first block= " << first_block << '\n';
   uint64_t file_end= lseek(ind_in,0,SEEK_END);                //get length of blocks.index (gives number of blocks)
   last_block= first_block + file_end/sizeof(uint64_t) - 1;
   cout << "last block=  " << last_block << '\n';
}

void trim_data::find_block_pos(uint32_t n) {
   //get file position of block n from blocks.index then confirm block n is found in blocks.log at that position
   //sets fpos0 and fpos1, throws exception if block at fpos0 is not block n
   using namespace std;
   index_pos= sizeof(uint64_t)*(n-first_block);
   uint64_t pos= lseek(ind_in,index_pos,SEEK_SET);
   EOS_ASSERT( pos==index_pos, block_log_exception, "cannot seek to blocks.index entry for block ${b}", ("b",n) );
   read(ind_in,(char*)&fpos0,sizeof(fpos0));                   //filepos of block n
   read(ind_in,(char*)&fpos1,sizeof(fpos1));                   //filepos of block n+1
   cout << "According to blocks.index:\n";
   cout << "    block " << n << " starts at position " << fpos0 << '\n';
   cout << "    block " << n+1;
   if (n!=last_block)
      cout << " starts at position " << fpos1 << '\n';
   else
      cout << " is past end\n";
   //read blocks.log and verify block number n is found at file position fpos0
   lseek(blk_in,fpos0+blknum_offset,SEEK_SET);
   uint32_t prior_blknum;
   read(blk_in,(char*)&prior_blknum,sizeof(prior_blknum));     //read bigendian block number of prior block
   uint32_t bnum= endian_reverse_u32(prior_blknum)+1;          //convert to little endian, add 1 since prior block
   cout << "At position " << fpos0 << " in blocks.log find block " << bnum << (bnum==n? " as expected\n": " - not good!\n");
   EOS_ASSERT( bnum==n, block_log_exception, "blocks.index does not agree with blocks.log" );
}

int trim_blocklog_end(bfs::path block_dir, uint32_t n) {       //n is last block to keep (remove later blocks)
   using namespace std;
   cout << "\nIn directory " << block_dir << " will trim all blocks after block " << n << " from blocks.log and blocks.index.\n";
   trim_data td(block_dir);
   if (n < td.first_block) {
      cerr << "All blocks are after block " << n << " so do nothing (trim_end would delete entire blocks.log)\n";
      return 1;
   }
   if (n >= td.last_block) {
      cerr << "There are no blocks after block " << n << " so do nothing\n";
      return 2;
   }
   td.find_block_pos(n);
   EOS_ASSERT( truncate(td.block_file_name.c_str(),td.fpos1)==0, block_log_exception, "truncate blocks.log fails");
   uint64_t index_end= td.index_pos+sizeof(uint64_t);             //advance past record for block n
   EOS_ASSERT( truncate(td.index_file_name.c_str(),index_end)==0, block_log_exception, "truncate blocks.index fails");
   cout << "blocks.log has been trimmed to " << td.fpos1 << " bytes\n";
   cout << "blocks.index has been trimmed to " << index_end << " bytes\n";
   return 0;
}

int trim_blocklog_front(bfs::path block_dir, uint32_t n) {        //n is first block to keep (remove prior blocks)
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
   auto buffer=  make_unique<char[]>(buf_len);                    //read big chunks of old blocks.log into this buffer
   char* buf=  buffer.get();

   string block_out_filename= (block_dir/"blocks.out").generic_string();
   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;           //if create file the permissions will be 644
   int blk_out = open(block_out_filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
   EOS_ASSERT( blk_out>0, block_log_not_found, "cannot write ${file}", ("file",block_out_filename) );

   //in version 1 file: version number, no first block number, rest of header length 0x6e, at 0x72 first block
   //in version 2 file: version number, first block number,    rest of header length 0x6e, at 0x76 totem,  at 0x7e first block
   *(uint32_t*)buf= 2;
   write(blk_out,buf,sizeof(uint32_t));                           //write version number 2
   write(blk_out,(char*)&n,sizeof(n));                            //write first block number

   lseek(td.blk_in,td.version==1? 4: 8,SEEK_SET) ;                //position past version number and maybe block number
   read(td.blk_in,buf,0x6e);                                      //copy rest of header length 0x6e
   memset(buf+0x6e,0xff,8);                                       //totem is 8 bytes of 0xff
   write(blk_out,buf,0x6e + 8);                                   //write header and totem

   //pos_delta is the amount to subtract from every file position record in blocks.log because the blocks < n are removed
   uint64_t pos_delta= td.fpos0 - 0x7e;                           //bytes removed from the blocklog
   //bytes removed is file position of block n minus file position 'where file header ends'
   //even if version 1 blocklog 'where file header ends' is where the new version 2 file header of length 0x7e ends

   //read big chunks of blocks.log into buf, update the file position records, then write to blk_out
   uint64_t pos= lseek(td.blk_in,0,SEEK_END);                     //get blocks.log file length
   uint32_t last_buf_len= (pos-td.fpos0 >= buf_len)? buf_len: pos-td.fpos0; //bytes to read from blk_in
   pos= lseek(td.blk_in,-(uint64_t)last_buf_len,SEEK_END);        //pos is where read last buf from blk_in
   uint64_t did_read= read(td.blk_in,buf,last_buf_len);           //read tail of blocks.log file into buf
   cout << "seek blocks.log to " << pos << " read " << last_buf_len << " bytes\n";//debug
   EOS_ASSERT( did_read==last_buf_len, block_log_exception, "blocks.log read fails" );

   //prepare to write index_out_filename
   uint64_t total_fpos_len= (td.last_block+1-n)*sizeof(uint64_t);
   auto fpos_buffer= make_unique<uint64_t[]>(buf_len);
   uint64_t* fpos_list= fpos_buffer.get();                        //list of file positions, periodically write to blocks.index
   string index_out_filename= (block_dir/"index.out" ).generic_string();
   int ind_out = open(index_out_filename.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
   EOS_ASSERT( ind_out>0, block_log_not_found, "cannot write ${file}", ("file",index_out_filename) );
   uint64_t last_fpos_len= total_fpos_len & ((uint64_t)buf_len-1);//buf_len is a power of 2 so -1 creates low bits all 1
   if (!last_fpos_len)                                            //will write integral number of buf_len and one time write last_fpos_len
      last_fpos_len= buf_len;
   uint32_t blk_base= td.last_block + 1 - (last_fpos_len>>3);     //first entry in fpos_list is for block blk_base
   cout << "******filling index buf blk_base " << blk_base << " last_fpos_len " << last_fpos_len << '\n';//debug

   //as we traverse linked list of blocks in buf (from end toward start), for each block we know this:
   int32_t index_start;                                           //buf index for block start
   int32_t index_end;                                             //buf index for block end == buf index for block start file position
   uint64_t file_pos;                                             //file position of block start
   uint32_t bnum;                                                 //block number

   //some code here is repeated in the loop below but we do it twice so can print a status message before the loop starts
   index_end= last_buf_len-8;                                     //buf index where last block ends and the block file position starts
   file_pos= *(uint64_t*)(buf+index_end);                         //file pos of block start
   index_start= file_pos - pos;                                   //buf index for block start
   bnum= *(uint32_t*)(buf + index_start + blknum_offset);         //block number of previous block (is big endian)
   bnum= endian_reverse_u32(bnum)+1;                              //convert from big endian to little endian and add 1
   EOS_ASSERT( bnum==td.last_block, block_log_exception, "last block from blocks.index ${ind} != last block from blocks.log ${log}", ("ind", td.last_block) ("log", bnum) );

   cout << '\n';                                                  //print header for periodic update messages
   cout << setw(10) << "block" << setw(16) << "old file pos" << setw(16) << "new file pos" << '\n';
   cout << setw(10) << bnum << setw(16) << file_pos << setw(16) << file_pos-pos_delta << '\n';
   uint64_t last_file_pos= file_pos+1;                            //used to check that file_pos is strictly decreasing
   constexpr uint32_t fk{4096};                                   //buffer shift that keeps buf disk block aligned

   for (;;) {        //invariant: at top of loop index_end and bnum are set and index_end is >= 0
      file_pos = *(uint64_t *) (buf + index_end);                 //get file pos of block start
      if (file_pos >= last_file_pos) {                            //check that file_pos decreases
         cout << '\n';
         cout << "file pos for block " << bnum + 1 << " is " << last_file_pos << '\n';
         cout << "file pos for block " << bnum << " is " << file_pos << '\n';
         EOS_ASSERT(file_pos < last_file_pos, block_log_exception, "blocks.log linked list of blocks is corrupt");
      }
      last_file_pos = file_pos;
      *(uint64_t *) (buf + index_end) = file_pos - pos_delta;     //update file position in buf
      fpos_list[bnum - blk_base] = file_pos - pos_delta;          //save updated file position for new index file
      index_start = file_pos - pos;                               //will go negative when pass first block in buf

      if ((bnum & 0xfffff) == 0)                                  //periodic progress indicator
         cout << setw(10) << bnum << setw(16) << file_pos << setw(16) << file_pos-pos_delta << '\n';

      if (bnum == blk_base) {                                     //if fpos_list is full write it to file
         cout << "****** bnum=" << bnum << " blk_base= " << blk_base << " so write index buf seek to " << (blk_base - n) * sizeof(uint64_t) << " write len " << last_fpos_len << '\n';//debug
         lseek(ind_out, (blk_base - n) * sizeof(uint64_t), SEEK_SET);
         write(ind_out, (char *) fpos_list, last_fpos_len);       //write fpos_list to index file
         last_fpos_len = buf_len;
         if (bnum == n) {                                         //if done with all blocks >= block n
            cout << setw(10) << bnum << setw(16) << file_pos << setw(16) << file_pos-pos_delta << '\n';
            EOS_ASSERT( index_start==0, block_log_exception, "block ${n} found with unexpected index_start ${s}", ("n",n) ("s",index_start));
            EOS_ASSERT( pos==td.fpos0, block_log_exception, "block ${n} found at unexpected file position ${p}", ("n",n) ("p",file_pos));
            lseek(blk_out, pos-pos_delta, SEEK_SET);
            write(blk_out, buf, last_buf_len);
            break;
         } else {
            blk_base -= (buf_len>>3);
            cout << "******filling index buf blk_base " << blk_base << " fpos_len " << buf_len << '\n';//debug
         }
      }

      if (index_start <= 0) {                                     //if buf is ready to write (all file pos are updated)
         //cout << "index_start = " << index_start << " so write buf len " << last_buf_len << " bnum= " << bnum << '\n';//debug
         EOS_ASSERT(file_pos>td.fpos0, block_log_exception, "reverse scan of blocks.log did not halt at block ${n}",("n",n));
         lseek(blk_out, pos-pos_delta, SEEK_SET);
         write(blk_out, buf, last_buf_len);
         last_buf_len= (pos-td.fpos0 > buf_len)? buf_len: pos-td.fpos0;
         pos -= last_buf_len;
         index_start += last_buf_len;
         //cout << "seek blocks to " << pos << " read " << last_buf_len << " bytes\n";//debug
         lseek(td.blk_in, pos, SEEK_SET);
         did_read = read(td.blk_in, buf, last_buf_len);           //read next buf from blocklog
         EOS_ASSERT(did_read == last_buf_len, block_log_exception, "blocks.log read fails");
      }

      index_end = index_start - sizeof(uint64_t);                 //move to prior block in buf
      --bnum;

      if (index_end < 0) {                                        //the file pos record straddles buf boundary
         cout << "****  index_end= " << index_end << " so save buf-4K len= " << last_buf_len-fk << " bnum= " << bnum << '\n';//debug
         lseek(blk_out, pos-pos_delta+fk, SEEK_SET);
         write(blk_out, buf + fk, last_buf_len - fk);             //skip first 4K of buf write out the rest
         last_buf_len= (pos-td.fpos0 >= buf_len-fk)? buf_len-fk: pos-td.fpos0;   //bytes to read from blk_in
         memcpy(buf + last_buf_len, buf, fk);                     //move first 4K of buf after zone will read
         pos-= last_buf_len;
         index_end += last_buf_len;cout << "seek blocks to " << pos << " read " << last_buf_len << " bytes before saved 4k\n";//debug
         lseek(td.blk_in, pos, SEEK_SET);
         did_read = read(td.blk_in, buf, last_buf_len);
         EOS_ASSERT(did_read == last_buf_len, block_log_exception, "blocks.log read fails");
         last_buf_len+= fk;                                       //bytes in buf will eventually write to disk
      }
   }

   close(blk_out);
   close(ind_out);
   string old_log= (block_dir/"old.log").generic_string();
   string old_ind= (block_dir/"old.index").generic_string();
   rename(td.block_file_name,old_log);
   rename(td.index_file_name,old_ind);
   rename(block_out_filename,td.block_file_name);
   rename(index_out_filename,td.index_file_name);
   cout << "The new blocks.log and blocks.index files contain blocks " << n << " through " << td.last_block << '\n';
   cout << "The original (before trim front) files have been renamed to old.log and old.index.\n";
   return 0;
}


int make_index(bfs::path block_dir, string out_file) {
   //this code makes blocks.index much faster than nodeos (in recent test 80 seconds vs. 90 minutes)
   using namespace std;
   string block_file_name= (block_dir / "blocks.log").generic_string();
   string out_file_name=   (block_dir / out_file     ).generic_string();
   cout << '\n';
   cout << "Will read existing blocks.log file " << block_file_name << '\n';
   cout << "Will write new blocks.index file " << out_file_name << '\n';
   int fin = open(block_file_name.c_str(), O_RDONLY);
   EOS_ASSERT( fin>0, block_log_not_found, "cannot read block file ${file}", ("file",block_file_name) );

   //will read big chunks of blocks.log into buf, will fill fpos_list with file positions before write to blocks.index
   constexpr uint32_t buf_len{1U<<24};                      //buf_len must be power of 2 >= largest possible block == one MByte
   auto buffer= make_unique<char[]>(buf_len+8);             //can write up to 8 bytes past end of buf
   char* buf= buffer.get();
   constexpr uint64_t fpos_list_len{1U<<22};                //length of fpos_list[] in bytes
   auto fpos_buffer= make_unique<uint64_t[]>(fpos_list_len>>3);
   uint64_t* fpos_list= fpos_buffer.get();

   //read blocks.log to see if version 1 or 2 and get first_blocknum (implicit 1 if version 1)
   uint32_t version=0, first_block=0;
   read(fin,(char*)&version,sizeof(version));
   cout << "block log version= " << version << '\n';
   EOS_ASSERT( version==1 || version==2, block_log_unsupported_version, "block log version ${v} is not supported", ("v",version));
   if (version == 1)
      first_block= 1;
   else
      read(fin,(char*)&first_block,sizeof(first_block));
   cout << "first block= " << first_block << '\n';

   uint64_t pos= lseek(fin,0,SEEK_END);                     //get blocks.log file length
   uint64_t last_buf_len= pos & ((uint64_t)buf_len-1);      //buf_len is a power of 2 so -1 creates low bits all 1
   if (!last_buf_len)                                       //will read integral number of buf_len and one time read last_buf_len
      last_buf_len= buf_len;
   pos= lseek(fin,-(uint64_t)last_buf_len,SEEK_END);        //one time read last_buf_len
   uint64_t did_read= read(fin,buf,last_buf_len);           //read tail of blocks.log file into buf
   EOS_ASSERT( did_read==last_buf_len, block_log_exception, "blocks.log read fails" );

   //we traverse linked list of blocks in buf (from end to start), for each block we know this:
   uint32_t index_start;                                    //buf index for block start
   uint32_t index_end;                                      //buf index for block end == buf index for block start file position
   uint64_t file_pos;                                       //file pos of block start
   uint32_t bnum;                                           //block number

   index_end= last_buf_len-8;                               //index in buf where last block ends and block file position starts
   file_pos= *(uint64_t*)(buf+index_end);                   //file pos of block start
   index_start= file_pos - pos;                             //buf index for block start
   bnum= *(uint32_t*)(buf + index_start + blknum_offset);   //block number of previous block (is big endian)
   bnum= endian_reverse_u32(bnum)+1;                        //convert from big endian to little endian and add 1
   cout << "last block=  " << bnum << '\n';
   cout << '\n';
   cout << "block " << setw(10) << bnum << "    file_pos " << setw(14) << file_pos << '\n';  //first progress indicator
   uint64_t last_file_pos= file_pos;                        //used to check that file_pos is strictly decreasing
   uint32_t end_block{bnum};                                //save for message at end

   //we use low level file IO because it is distinctly faster than C++ filebuf or iostream
   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;     //if create file permissions will be 644
   int fout = open(out_file_name.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode);
   EOS_ASSERT( fout>0, block_index_not_found, "cannot write blocks.index" );

   uint64_t ind_file_len= (bnum+1-first_block)<<3;                 //index file holds 8 bytes for each block in blocks.log
   uint64_t last_ind_buf_len= ind_file_len & (fpos_list_len-1);    //fpos_list_len is a power of 2 so -1 creates low bits all 1
   if (!last_ind_buf_len)                                   //will write integral number of buf_len and last_ind_buf_len one time to index file
      last_ind_buf_len= buf_len;
   uint64_t ind_pos= lseek(fout,ind_file_len-last_ind_buf_len,SEEK_SET);
   uint64_t blk_base= (ind_pos>>3) + first_block;           //first entry in fpos_list is for block blk_base
   //cout << "ind_pos= " << ind_pos << "  blk_base= " << blk_base << '\n';
   fpos_list[bnum-blk_base]= file_pos;                      //write filepos for block bnum

   for (;;) {
      if (bnum==blk_base) {                                 //if fpos_list is full
         write(fout,(char*)fpos_list,last_ind_buf_len);     //write fpos_list to index file
         if (ind_pos==0) {                                  //if done writing index file
            cout << "block " << setw(10) << bnum << "    file_pos " << setw(14) << file_pos << '\n';  //last progress indicator
            EOS_ASSERT( bnum == first_block, block_log_exception, "blocks.log does not contain consecutive block numbers" );
            break;
         }
         ind_pos-= fpos_list_len;
         blk_base-= (fpos_list_len>>3);
         did_read= lseek(fout,ind_pos,SEEK_SET);
         EOS_ASSERT( did_read==ind_pos, block_log_exception, "blocks.log seek fails" );
         last_ind_buf_len= fpos_list_len;                   //from now on all writes to index file write a full fpos_list[]
      }
      if (index_start < 8) {                                //if block start is split across buf boundary
         memcpy(buf+buf_len,buf,8);                         //copy portion at start of buf to past end of buf
         pos-= buf_len;                                     //file position of buf
         lseek(fin,pos,SEEK_SET);
         did_read= read(fin,buf,buf_len);                   //read next buf
         EOS_ASSERT( did_read==buf_len, block_log_exception, "blocks.log read fails" );
         index_start+= buf_len;
      }
      --bnum;                                               //now move index_start and index_end to prior block
      index_end= index_start-8;                             //index in buf where block ends and block file position starts
      file_pos= *(uint64_t*)(buf+index_end);                //file pos of block start
      if (file_pos >= last_file_pos) {                      //file_pos will decrease if linked list is not corrupt
         cout << '\n';
         cout << "file pos for block " << bnum+1 << " is " << last_file_pos << '\n';
         cout << "file pos for block " << bnum   << " is " <<      file_pos << '\n';
         cout << "The linked list of blocks in blocks.log should run from last block to first block in reverse order\n";
         EOS_ASSERT( file_pos<last_file_pos, block_log_exception, "blocks.log linked list of blocks is corrupt" );
      }
      last_file_pos= file_pos;
      if (file_pos < pos) {                                 //if block start is in prior buf
         pos-= buf_len;                                     //file position of buf
         lseek(fin,pos,SEEK_SET);
         did_read= read(fin,buf,buf_len);                   //read next buf
         EOS_ASSERT( did_read==buf_len, block_log_exception, "blocks.log read fails" );
         index_end+= buf_len;
      }
      index_start= file_pos - pos;                          //buf index for block start
      fpos_list[bnum-blk_base]= file_pos;                   //write filepos for block bnum
      if ((bnum & 0xfffff) == 0)                            //periodically print a progress indicator
         cout << "block " << setw(10) << bnum << "    file_pos " << setw(14) << file_pos << '\n';
   }

   close(fout);
   close(fin);
   cout << "\nwrote " << (end_block+1-first_block) << " file positions to " << out_file_name << '\n';
   return 0;
}

void smoke_test(bfs::path block_dir) {
   using namespace std;
   cout << "\nSmoke test of blocks.log and blocks.index in directory " << block_dir << '\n';
   trim_data td(block_dir);
   lseek(td.blk_in,-sizeof(uint64_t),SEEK_END);             //get last_block from blocks.log, compare to from blocks.index
   uint64_t file_pos;
   read(td.blk_in,&file_pos,sizeof(uint64_t));
   lseek(td.blk_in,file_pos+blknum_offset,SEEK_SET);
   uint32_t bnum;
   read(td.blk_in,&bnum,sizeof(uint32_t));
   bnum= endian_reverse_u32(bnum)+1;                        //convert from big endian to little endian and add 1
   EOS_ASSERT( td.last_block==bnum, block_log_exception, "blocks.log says last block is ${lb} which disagrees with blocks.index", ("lb",bnum) );
   cout << "blocks.log and blocks.index agree on number of blocks\n";
   uint32_t delta= (td.last_block+8-td.first_block)>>3;
   if (delta<1)
      delta= 1;
   for (uint32_t n= td.first_block; ; n+= delta) {
      if (n>td.last_block)
         n= td.last_block;
      cout << '\n';
      td.find_block_pos(n);                                 //check block 'n' is where blocks.index says
      if (n==td.last_block)
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
         if (blog.first_block==0 && blog.last_block==std::numeric_limits<uint32_t>::max()) {
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
         string out_file{vmap.count("output-file")==0? string("blocks.index"): vmap.at("output-file").as<bfs::path>().generic_string()};
         return make_index(vmap.at("blocks-dir").as<bfs::path>(), out_file);
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
