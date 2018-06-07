/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/block_log.hpp>
#include <fstream>
#include <fc/io/raw.hpp>

#define LOG_READ  (std::ios::in | std::ios::binary)
#define LOG_WRITE (std::ios::out | std::ios::binary | std::ios::app)

namespace eosio { namespace chain {

   const uint32_t block_log::supported_version = 1;

   namespace detail {
      class block_log_impl {
         public:
            signed_block_ptr         head;
            block_id_type            head_id;
            std::fstream             block_stream;
            std::fstream             index_stream;
            fc::path                 block_file;
            fc::path                 index_file;
            bool                     block_write;
            bool                     index_write;
            bool                     genesis_written_to_block_log = false;

            inline void check_block_read() {
               if (block_write) {
                  block_stream.close();
                  block_stream.open(block_file.generic_string().c_str(), LOG_READ);
                  block_write = false;
               }
            }

            inline void check_block_write() {
               if (!block_write) {
                  block_stream.close();
                  block_stream.open(block_file.generic_string().c_str(), LOG_WRITE);
                  block_write = true;
               }
            }

            inline void check_index_read() {
               try {
                  if (index_write) {
                     index_stream.close();
                     index_stream.open(index_file.generic_string().c_str(), LOG_READ);
                     index_write = false;
                  }
               }
               FC_LOG_AND_RETHROW()
            }

            inline void check_index_write() {
               if (!index_write) {
                  index_stream.close();
                  index_stream.open(index_file.generic_string().c_str(), LOG_WRITE);
                  index_write = true;
               }
            }
      };
   }

   block_log::block_log(const fc::path& data_dir)
   :my(new detail::block_log_impl()) {
      my->block_stream.exceptions(std::fstream::failbit | std::fstream::badbit);
      my->index_stream.exceptions(std::fstream::failbit | std::fstream::badbit);
      open(data_dir);
   }

   block_log::block_log(block_log&& other) {
      my = std::move(other.my);
   }

   block_log::~block_log() {
      if (my) {
         flush();
         my.reset();
      }
   }

   void block_log::open(const fc::path& data_dir) {
      if (my->block_stream.is_open())
         my->block_stream.close();
      if (my->index_stream.is_open())
         my->index_stream.close();

      if (!fc::is_directory(data_dir))
         fc::create_directories(data_dir);
      my->block_file = data_dir / "blocks.log";
      my->index_file = data_dir / "blocks.index";

      //ilog("Opening block log at ${path}", ("path", my->block_file.generic_string()));
      my->block_stream.open(my->block_file.generic_string().c_str(), LOG_WRITE);
      my->index_stream.open(my->index_file.generic_string().c_str(), LOG_WRITE);
      my->block_write = true;
      my->index_write = true;

      /* On startup of the block log, there are several states the log file and the index file can be
       * in relation to each other.
       *
       *                          Block Log
       *                     Exists       Is New
       *                 +------------+------------+
       *          Exists |    Check   |   Delete   |
       *   Index         |    Head    |    Index   |
       *    File         +------------+------------+
       *          Is New |   Replay   |     Do     |
       *                 |    Log     |   Nothing  |
       *                 +------------+------------+
       *
       * Checking the heads of the files has several conditions as well.
       *  - If they are the same, do nothing.
       *  - If the index file head is not in the log file, delete the index and replay.
       *  - If the index file head is in the log, but not up to date, replay from index head.
       */
      auto log_size = fc::file_size(my->block_file);
      auto index_size = fc::file_size(my->index_file);

      if (log_size) {
         ilog("Log is nonempty");
         my->check_block_read();
         my->block_stream.seekg( 0 );
         uint32_t version = 0;
         my->block_stream.read( (char*)&version, sizeof(version) );
         FC_ASSERT( version > 0, "Block log was not setup properly with genesis information." );
         FC_ASSERT( version == block_log::supported_version,
                    "Unsupported version of block log. Block log version is ${version} while code supports version ${supported}",
                    ("version", version)("supported", block_log::supported_version) );

         my->genesis_written_to_block_log = true; // Assume it was constructed properly.
         my->head = read_head();
         my->head_id = my->head->id();
         edump((my->head->block_num()));

         if (index_size) {
            my->check_block_read();
            my->check_index_read();

            ilog("Index is nonempty");
            uint64_t block_pos;
            my->block_stream.seekg(-sizeof(uint64_t), std::ios::end);
            my->block_stream.read((char*)&block_pos, sizeof(block_pos));

            uint64_t index_pos;
            my->index_stream.seekg(-sizeof(uint64_t), std::ios::end);
            my->index_stream.read((char*)&index_pos, sizeof(index_pos));

            if (block_pos < index_pos) {
               ilog("block_pos < index_pos, close and reopen index_stream");
               construct_index();
            } else if (block_pos > index_pos) {
               ilog("Index is incomplete");
               construct_index();
            }
         } else {
            ilog("Index is empty");
            construct_index();
         }
      } else if (index_size) {
         ilog("Index is nonempty, remove and recreate it");
         my->index_stream.close();
         fc::remove_all(my->index_file);
         my->index_stream.open(my->index_file.generic_string().c_str(), LOG_WRITE);
         my->index_write = true;
      }
   }

   uint64_t block_log::append(const signed_block_ptr& b) {
      try {
         FC_ASSERT( my->genesis_written_to_block_log, "Cannot append to block log until the genesis is first written" );

         my->check_block_write();
         my->check_index_write();

         uint64_t pos = my->block_stream.tellp();
         FC_ASSERT(my->index_stream.tellp() == sizeof(uint64_t) * (b->block_num() - 1),
                   "Append to index file occuring at wrong position.",
                   ("position", (uint64_t) my->index_stream.tellp())
                   ("expected", (b->block_num() - 1) * sizeof(uint64_t)));
         auto data = fc::raw::pack(*b);
         my->block_stream.write(data.data(), data.size());
         my->block_stream.write((char*)&pos, sizeof(pos));
         my->index_stream.write((char*)&pos, sizeof(pos));
         my->head = b;
         my->head_id = b->id();

         flush();

         return pos;
      }
      FC_LOG_AND_RETHROW()
   }

   void block_log::flush() {
      my->block_stream.flush();
      my->index_stream.flush();
   }

   uint64_t block_log::reset_to_genesis( const genesis_state& gs, const signed_block_ptr& genesis_block ) {
      if( my->block_stream.is_open() )
         my->block_stream.close();
      if( my->index_stream.is_open() )
         my->index_stream.close();

      fc::remove_all( my->block_file );
      fc::remove_all( my->index_file );

      my->block_stream.open(my->block_file.generic_string().c_str(), LOG_WRITE);
      my->index_stream.open(my->index_file.generic_string().c_str(), LOG_WRITE);
      my->block_write = true;
      my->index_write = true;

      auto data = fc::raw::pack( gs );
      uint32_t version = 0; // version of 0 is invalid; it indicates that the genesis was not properly written to the block log
      my->block_stream.write( (char*)&version, sizeof(version) );
      my->block_stream.write( data.data(), data.size() );
      my->genesis_written_to_block_log = true;

      auto ret = append( genesis_block );

      auto pos = my->block_stream.tellp();

      my->block_stream.close();
      my->block_stream.open(my->block_file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary ); // Bypass append-only writing just once

      static_assert( block_log::supported_version > 0, "a version number of zero is not supported" );
      version = block_log::supported_version;
      my->block_stream.seekp( 0 );
      my->block_stream.write( (char*)&version, sizeof(version) ); // Finally write actual version to disk.
      my->block_stream.seekp( pos );
      flush();

      my->block_write = false;
      my->check_block_write(); // Reset to append-only writing.

      return ret;
   }

   std::pair<signed_block_ptr, uint64_t> block_log::read_block(uint64_t pos)const {
      my->check_block_read();

      my->block_stream.seekg(pos);
      std::pair<signed_block_ptr,uint64_t> result;
      result.first = std::make_shared<signed_block>();
      fc::raw::unpack(my->block_stream, *result.first);
      result.second = uint64_t(my->block_stream.tellg()) + 8;
      return result;
   }

   signed_block_ptr block_log::read_block_by_num(uint32_t block_num)const {
      try {
         signed_block_ptr b;
         uint64_t pos = get_block_pos(block_num);
         if (pos != npos) {
            b = read_block(pos).first;
            FC_ASSERT(b->block_num() == block_num,
                      "Wrong block was read from block log.", ("returned", b->block_num())("expected", block_num));
         }
         return b;
      } FC_LOG_AND_RETHROW()
   }

   uint64_t block_log::get_block_pos(uint32_t block_num) const {
      my->check_index_read();

      if (!(my->head && block_num <= block_header::num_from_id(my->head_id) && block_num > 0))
         return npos;
      my->index_stream.seekg(sizeof(uint64_t) * (block_num - 1));
      uint64_t pos;
      my->index_stream.read((char*)&pos, sizeof(pos));
      return pos;
   }

   signed_block_ptr block_log::read_head()const {
      my->check_block_read();

      uint64_t pos;

      // Check that the file is not empty
      my->block_stream.seekg(0, std::ios::end);
      if (my->block_stream.tellg() <= sizeof(pos))
         return {};

      my->block_stream.seekg(-sizeof(pos), std::ios::end);
      my->block_stream.read((char*)&pos, sizeof(pos));
      return read_block(pos).first;
   }

   const signed_block_ptr& block_log::head()const {
      return my->head;
   }

   void block_log::construct_index() {
      ilog("Reconstructing Block Log Index...");
      my->index_stream.close();
      fc::remove_all(my->index_file);
      my->index_stream.open(my->index_file.generic_string().c_str(), LOG_WRITE);
      my->index_write = true;

      uint64_t end_pos;
      my->check_block_read();

      my->block_stream.seekg(-sizeof( uint64_t), std::ios::end);
      my->block_stream.read((char*)&end_pos, sizeof(end_pos));
      signed_block tmp;

      uint64_t pos = 4; // Skip version which should have already been checked.
      my->block_stream.seekg(pos);

      genesis_state gs;
      fc::raw::unpack(my->block_stream, gs);

      while( pos < end_pos ) {
         fc::raw::unpack(my->block_stream, tmp);
         my->block_stream.read((char*)&pos, sizeof(pos));
         my->index_stream.write((char*)&pos, sizeof(pos));
      }
   } // construct_index

   fc::path block_log::repair_log( const fc::path& data_dir, uint32_t truncate_at_block ) {
      ilog("Recovering Block Log...");
      FC_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"),
                 "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );

      auto now = fc::time_point::now();

      auto blocks_dir = fc::canonical( data_dir );
      if( blocks_dir.filename().generic_string() == "." ) {
         blocks_dir = blocks_dir.parent_path();
      }
      auto backup_dir = blocks_dir.parent_path();
      auto blocks_dir_name = blocks_dir.filename();
      FC_ASSERT( blocks_dir_name.generic_string() != ".", "Invalid path to blocks directory" );
      backup_dir = backup_dir / blocks_dir_name.generic_string().append("-").append( now );

      FC_ASSERT( !fc::exists(backup_dir),
                 "Cannot move existing blocks directory to already existing directory '${new_blocks_dir}'",
                 ("new_blocks_dir", backup_dir) );

      fc::rename( blocks_dir, backup_dir );
      ilog( "Moved existing blocks directory to backup location: '${new_blocks_dir}'", ("new_blocks_dir", backup_dir) );

      fc::create_directories(blocks_dir);
      auto block_log_path = blocks_dir / "blocks.log";

      ilog( "Reconstructing '${new_block_log}' from backed up block log", ("new_block_log", block_log_path) );

      std::fstream  old_block_stream;
      std::fstream  new_block_stream;

      old_block_stream.open( (backup_dir / "blocks.log").generic_string().c_str(), LOG_READ );
      new_block_stream.open( block_log_path.generic_string().c_str(), LOG_WRITE );

      old_block_stream.seekg( 0, std::ios::end );
      uint64_t end_pos = old_block_stream.tellg();
      old_block_stream.seekg( 0 );

      uint32_t version = 0;
      old_block_stream.read( (char*)&version, sizeof(version) );
      FC_ASSERT( version > 0, "Block log was not setup properly with genesis information." );
      FC_ASSERT( version == block_log::supported_version,
                 "Unsupported version of block log. Block log version is ${version} while code supports version ${supported}",
                 ("version", version)("supported", block_log::supported_version) );

      genesis_state gs;
      fc::raw::unpack(old_block_stream, gs);

      auto data = fc::raw::pack( gs );
      new_block_stream.write( (char*)&version, sizeof(version) );
      new_block_stream.write( data.data(), data.size() );

      std::exception_ptr     except_ptr;
      vector<char>           incomplete_block_data;
      optional<signed_block> bad_block;
      uint32_t               block_num = 0;

      block_id_type previous;

      uint64_t pos = old_block_stream.tellg();
      while( pos < end_pos ) {
         signed_block tmp;

         try {
            fc::raw::unpack(old_block_stream, tmp);
         } catch( ... ) {
            except_ptr = std::current_exception();
            incomplete_block_data.resize( end_pos - pos );
            old_block_stream.read( incomplete_block_data.data(), incomplete_block_data.size() );
            break;
         }

         auto id = tmp.id();
         if( block_header::num_from_id(previous) + 1 != block_header::num_from_id(id) ) {
            elog( "Block ${num} (${id}) skips blocks. Previous block in block log is block ${prev_num} (${previous})",
                  ("num", block_header::num_from_id(id))("id", id)
                  ("prev_num", block_header::num_from_id(previous))("previous", previous) );
         }
         if( previous != tmp.previous ) {
            elog( "Block ${num} (${id}) does not link back to previous block. "
                  "Expected previous: ${expected}. Actual previous: ${actual}.",
                  ("num", block_header::num_from_id(id))("id", id)("expected", previous)("actual", tmp.previous) );
         }
         previous = id;

         uint64_t tmp_pos = std::numeric_limits<uint64_t>::max();
         if( (static_cast<uint64_t>(old_block_stream.tellg()) + sizeof(pos)) <= end_pos ) {
            old_block_stream.read( reinterpret_cast<char*>(&tmp_pos), sizeof(tmp_pos) );
         }
         if( pos != tmp_pos ) {
            bad_block = tmp;
            break;
         }

         auto data = fc::raw::pack(tmp);
         new_block_stream.write( data.data(), data.size() );
         new_block_stream.write( reinterpret_cast<char*>(&pos), sizeof(pos) );
         block_num = tmp.block_num();
         pos = new_block_stream.tellp();
         if( block_num == truncate_at_block )
            break;
      }

      if( bad_block.valid() ) {
         ilog( "Recovered only up to block number ${num}. Last block in block log was not properly committed:\n${last_block}",
               ("num", block_num)("last_block", *bad_block) );
      } else if( except_ptr ) {
         std::string error_msg;

         try {
            std::rethrow_exception(except_ptr);
         } catch( const fc::exception& e ) {
            error_msg = e.what();
         } catch( const std::exception& e ) {
            error_msg = e.what();
         } catch( ... ) {
            error_msg = "unrecognized exception";
         }

         ilog( "Recovered only up to block number ${num}. "
               "The block ${next_num} could not be deserialized from the block log due to error:\n${error_msg}",
               ("num", block_num)("next_num", block_num+1)("error_msg", error_msg) );

         auto tail_path = blocks_dir / std::string("blocks-bad-tail-").append( now ).append(".log");
         if( !fc::exists(tail_path) && incomplete_block_data.size() > 0 ) {
            std::fstream tail_stream;
            tail_stream.open( tail_path.generic_string().c_str(), LOG_WRITE );
            tail_stream.write( incomplete_block_data.data(), incomplete_block_data.size() );

            ilog( "Data at tail end of block log which should contain the (incomplete) serialization of block ${num} "
                  "has been written out to '${tail_path}'.",
                  ("num", block_num+1)("tail_path", tail_path) );
         }
      } else if( block_num == truncate_at_block && pos < end_pos ) {
         ilog( "Stopped recovery of block log early at specified block number: ${stop}.", ("stop", truncate_at_block) );
      } else {
         ilog( "Existing block log was undamaged. Recovered all irreversible blocks up to block number ${num}.", ("num", block_num) );
      }

      return backup_dir;
   }

   genesis_state block_log::extract_genesis_state( const fc::path& data_dir ) {
      FC_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"),
                 "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );

      std::fstream  block_stream;
      block_stream.open( (data_dir / "blocks.log").generic_string().c_str(), LOG_READ );

      uint32_t version = 0;
      block_stream.read( (char*)&version, sizeof(version) );
      FC_ASSERT( version > 0, "Block log was not setup properly with genesis information." );
      FC_ASSERT( version == block_log::supported_version,
                 "Unsupported version of block log. Block log version is ${version} while code supports version ${supported}",
                 ("version", version)("supported", block_log::supported_version) );

      genesis_state gs;
      fc::raw::unpack(block_stream, gs);
      return gs;
   }

} } /// eosio::chain
