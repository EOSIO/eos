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
   uint32_t                         trim_block;
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
         ("first,f", bpo::value<uint32_t>(&first_block)->default_value(1),
          "the first block number to log")
         ("last,l", bpo::value<uint32_t>(&last_block)->default_value(std::numeric_limits<uint32_t>::max()),
          "the last block number (inclusive) to log")
         ("no-pretty-print", bpo::bool_switch(&no_pretty_print)->default_value(false),
          "Do not pretty print the output.  Useful if piping to jq to improve performance.")
         ("as-json-array", bpo::bool_switch(&as_json_array)->default_value(false),
          "Print out json blocks wrapped in json array (otherwise the output is free-standing json objects).")
         ("make-index", bpo::bool_switch(&make_index)->default_value(false),
          "Create blocks.index from blocks.log. Must give 'blocks-dir'. Give 'output-file' relative to blocks-dir (default is blocks.index).")
         ("trim-blocklog", bpo::bool_switch(&make_index)->default_value(false),
          "Trim blocks.log and blocks.index in place. Must give 'blocks-dir' and 'last' (else nothing is done).")
         ("help,h", "Print this help message and exit.")
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

//struct used by trunc_blocklog() and make_index() to read first 18 bytes of a block from blocks.log
struct __attribute__((packed)) Block_Start {    //first 18 bytes of each block (must match block_header)
   block_timestamp_type timestamp;
   account_name         prodname;
   uint16_t             confirmed;
   uint32_t             blknum;     //low 32 bits of previous blockid, is big endian block number of previous block
} blk_start;
static_assert( sizeof(Block_Start) == 18, "Update block_start if block_header changes");

int trunc_blocklog(bfs::path block_dir, uint32_t n) {       //n is last block to keep
   using namespace std;
   //read blocks.log to see if version 1 or 2 and get first block number
   filebuf fin_blocks;
   cout << "In directory " << block_dir << " will truncate blocks.log and blocks.index after block " << n << '\n';
   string block_file_name= (block_dir/"blocks.log").generic_string();
   fin_blocks.open(block_file_name, ios::in|ios::binary);
   EOS_ASSERT( fin_blocks.is_open(), block_log_not_found, "cannot read ${file}", ("file",block_file_name) );
   uint32_t version=0, first_block;
   fin_blocks.sgetn((char*)&version,sizeof(version));
   cout << "block log version= " << version << '\n';
   EOS_ASSERT( version==1 || version==2, block_log_unsupported_version, "block log version ${v} is not supported", ("v",version));
   if (version == 1)
      first_block= 1;
   else
      fin_blocks.sgetn((char*)&first_block,sizeof(first_block));
   cout << "first block= " << first_block << '\n';
   if (n <= first_block) {
      cerr << n << " is before first block so nothing to do\n";
      return 2;
   }

   //open blocks.index, get last_block and blocks.log position for block 'n'
   filebuf fin_index;
   string index_file_name= (block_dir/"blocks.index").generic_string();
   fin_index.open(index_file_name,ios::in|ios::binary);
   EOS_ASSERT( fin_index.is_open(), block_index_not_found, "cannot read ${file}", ("file",index_file_name) );
   uint64_t file_end= fin_index.pubseekoff(0,ios::end,ios::in);
   uint32_t last_block= file_end/sizeof(uint64_t);
   cout << "last block=  " << last_block << '\n';
   if (n >= last_block) {
      cerr << n << " is not before last block so nothing to do\n";
      return 2;
   }
   uint64_t index_pos= sizeof(uint64_t)*(n-first_block);
   uint64_t pos= fin_index.pubseekoff(index_pos,ios::beg,ios::in);
   EOS_ASSERT( pos==index_pos, block_log_exception, "cannot read blocks.index entry for block ${b}", ("b",n) );
   uint64_t fpos0, fpos1;                                   //filepos of block n and block n+1, will read from blocks.index
   fin_index.sgetn((char*)&fpos0,sizeof(fpos0));
   fin_index.sgetn((char*)&fpos1,sizeof(fpos1));
   fin_index.close();
   cout << "According to blocks.index:\n";
   cout << "    block " << n << " starts at position " << fpos0 << '\n';
   cout << "    block " << n+1 << " starts at position " << fpos1 << '\n';

   //read blocks.log and verify block number n is found at file position fpos0
   fin_blocks.pubseekoff(fpos0,ios::beg,ios::in);
   fin_blocks.sgetn((char*)&blk_start,sizeof(blk_start));   //read first 18 bytes of block
   fin_blocks.close();
   uint32_t bnum= endian_reverse_u32(blk_start.blknum)+1;   //convert from big endian to little endian, add 1 since prior block
   cout << "At position " << fpos0 << " in blocks.log find block " << bnum << (bnum==n? " as expected\n": " - not good!\n");
   EOS_ASSERT( bnum==n, block_log_exception, "blocks.index does not agree with blocks.log" );
   EOS_ASSERT( truncate(block_file_name.c_str(),fpos1)==0, block_log_exception, "truncate blocks.log fails");
   index_pos+= sizeof(uint64_t);                            //advance to after record for block n
   EOS_ASSERT( truncate(index_file_name.c_str(),index_pos)==0, block_log_exception, "truncate blocks.index fails");
   cout << "blocks.log has been truncated to " << fpos1 << " bytes\n";
   cout << "blocks.index has been truncated to " << index_pos << " bytes\n";
   return 0;
}

int make_index(bfs::path block_dir, string out_file) {
   //this code makes blocks.index much faster than nodeos (in recent test 80 seconds vs. 90 minutes)
   using namespace std;
   string block_file_name= (block_dir / "blocks.log").generic_string();
   string out_file_name=   (block_dir / out_file     ).generic_string();
   cout << "Will read blocks.log file " << block_file_name << '\n';
   cout << "Will write blocks.index file " << out_file_name << '\n';
   int fin = open(block_file_name.c_str(), O_RDONLY);
   EOS_ASSERT( fin>0, block_log_not_found, "cannot read block file ${file}", ("file",block_file_name) );

   //will read big chunks of blocks.log into buf, will fill fpos_list with file positions before write to blocks.index
   constexpr uint32_t bufLen{1U<<24};                       //bufLen must be power of 2 >= largest possible block == one MByte
   auto buffer= make_unique<char[]>(bufLen+8);              //first 8 bytes of prior buf are put past end of current buf
   char* buf= buffer.get();
   constexpr uint64_t fpos_list_len{1U<<22};                //length of fpos_list[] in bytes
   auto fpos_buffer= make_unique<uint64_t[]>(fpos_list_len>>3);
   uint64_t* fpos_list= fpos_buffer.get();

   //read blocks.log to see if version 1 or 2 and get first_blocknum (implicit 1 if version 1)
   uint32_t version=0, first_block;
   read(fin,(char*)&version,sizeof(version));
   cout << "block log version= " << version << '\n';
   EOS_ASSERT( version==1 || version==2, block_log_unsupported_version, "block log version ${v} is not supported", ("v",version));
   if (version == 1)
      first_block= 1;
   else
      read(fin,(char*)&first_block,sizeof(first_block));
   cout << "first block= " << first_block << '\n';

   uint64_t pos= lseek(fin,0,SEEK_END);                     //get blocks.log file length
   uint64_t last_buf_len= pos & ((uint64_t)bufLen-1);       //bufLen is a power of 2 so -1 creates low bits all 1
   if (!last_buf_len)                                       //will read integral number of bufLen and one time read last_buf_len
      last_buf_len= bufLen;
   pos= lseek(fin,-(uint64_t)last_buf_len,SEEK_END);
   uint64_t did_read= read(fin,buf,last_buf_len);           //read tail of file into buf
   EOS_ASSERT( did_read==last_buf_len, block_log_exception, "blocks.log read fails" );

   uint32_t index_start;                                    //buf index for block start
   uint32_t index_end;                                      //buf index for block end == buf index for block start file position
   uint64_t file_pos;                                       //file pos of block start
   uint64_t last_file_pos;                                  //used to check that file_pos is strictly decreasing
   Block_Start* bst;                                        //pointer to Block_Start

   index_start= last_buf_len;                               //pretend a block starts just past end of buf then read prior block
   index_end= index_start-8;                                //index in buf where block ends and block file position starts
   last_file_pos= file_pos= *(uint64_t*)(buf+index_end);    //file pos of block start
   index_start= file_pos - pos;                             //buf index for block start
   bst= (Block_Start*)(buf+index_start);                    //pointer to Block_Start
   uint32_t last_block= endian_reverse_u32(bst->blknum);    //convert from big endian to little endian
   uint32_t bnum= ++last_block;                             //add 1 since took block number from prior block id
   cout << "last block=  " << last_block << '\n';
   cout << '\n';
   cout << "block " << setw(10) << bnum << "    file_pos " << setw(14) << file_pos << '\n';  //first progress indicator

   //we use low level file IO because it is distinctly faster than C++ filebuf or iostream
   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;     //if create file permissions will be 644
   int fout = open(out_file_name.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode); //create if no exists, truncate if does exist
   EOS_ASSERT( fout>0, block_index_not_found, "cannot write blocks.index" );

   uint64_t ind_file_len= bnum<<3;                          //index file holds 8 bytes per block in blocks.log
   uint64_t last_ind_buf_len= ind_file_len & (fpos_list_len-1);    //fpos_list_len is a power of 2 so -1 creates low bits all 1
   if (!last_ind_buf_len)                                   //will write integral number of bufLen and last_ind_buf_len one time to index file
      last_ind_buf_len= bufLen;
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
         blk_base-= fpos_list_len>>3;
         did_read= lseek(fout,ind_pos,SEEK_SET);
         EOS_ASSERT( did_read==ind_pos, block_log_exception, "blocks.log seek fails" );
         last_ind_buf_len= fpos_list_len;                   //from now on all writes to index file write a full fpos_list[]
      }
      if (index_start < 8) {                                //if block start is split across buf boundary
         memcpy(buf+bufLen,buf,8);                          //copy portion at start of buf to past end of buf
         pos-= bufLen;                                      //file position of buf
         lseek(fin,pos,SEEK_SET);
         did_read= read(fin,buf,bufLen);                    //read next buf
         EOS_ASSERT( did_read==bufLen, block_log_exception, "blocks.log read fails" );
         index_start+= bufLen;
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
         pos-= bufLen;                                      //file position of buf
         lseek(fin,pos,SEEK_SET);
         did_read= read(fin,buf,bufLen);                    //read next buf
         EOS_ASSERT( did_read==bufLen, block_log_exception, "blocks.log read fails" );
         index_end+= bufLen;
      }
      index_start= file_pos - pos;                          //buf index for block start
       fpos_list[bnum-blk_base]= file_pos;                  //write filepos for block bnum
      if ((bnum & 0xfffff) == 0)                            //periodically print a progress indicator
         cout << "block " << setw(10) << bnum << "    file_pos " << setw(14) << file_pos << '\n';
   }

   close(fout);
   close(fin);
   cout << "\nwrote " << last_block << " file positions to " << out_file_name << '\n';
   return 0;
}

int main(int argc, char** argv)
{
   std::ios::sync_with_stdio(false); // for potential performance boost for large block log files
   options_description cli ("eosio-blocklog command line options");
   try {
      blocklog blog;
      blog.set_program_options(cli);
      variables_map vmap;
      bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
      bpo::notify(vmap);
      if (vmap.count("help") > 0) {
        cli.print(std::cerr);
        return 0;
      }
      if (vmap.at("trim-blocklog").as<bool>()) {
         uint32_t last= vmap.at("last").as<uint32_t>();
         if (last == std::numeric_limits<uint32_t>::max()) {   //if 'last' was not given on command line
            std::cout << "'trim-block-blocklog' does nothing unless specify 'last' block.";
            return -1;
         }
         return trunc_blocklog(vmap.at("blocks-dir").as<bfs::path>(), last);
      }
      if (vmap.at("make-index").as<bool>()) {
         string out_file{vmap.count("output-file")==0? string("blocks.index"): vmap.at("output-file").as<bfs::path>().generic_string()};
         return make_index(vmap.at("blocks-dir").as<bfs::path>(), out_file);
      }
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
