#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fstream>
#include <fc/bitutil.hpp>
#include <fc/io/cfile.hpp>
#include <fc/io/raw.hpp>


#define LOG_READ  (std::ios::in | std::ios::binary)
#define LOG_WRITE (std::ios::out | std::ios::binary | std::ios::app)
#define LOG_RW ( std::ios::in | std::ios::out | std::ios::binary )
#define LOG_WRITE_C "ab+"
#define LOG_RW_C "rb+"

#ifndef _WIN32
#define FC_FOPEN(p, m) fopen(p, m)
#else
#define FC_CAT(s1, s2) s1 ## s2
#define FC_PREL(s) FC_CAT(L, s)
#define FC_FOPEN(p, m) _wfopen(p, FC_PREL(m))
#endif

namespace eosio { namespace chain {

   const uint32_t block_log::min_supported_version = 1;

   /**
    * History:
    * Version 1: complete block log from genesis
    * Version 2: adds optional partial block log, cannot be used for replay without snapshot
    *            this is in the form of an first_block_num that is written immediately after the version
    */
   const uint32_t block_log::max_supported_version = 2;

   namespace detail {
      using unique_file = std::unique_ptr<FILE, decltype(&fclose)>;

      class block_log_impl {
         public:
            signed_block_ptr         head;
            block_id_type            head_id;
            fc::cfile                block_file;
            fc::cfile                index_file;
            bool                     open_files = false;
            bool                     genesis_written_to_block_log = false;
            uint32_t                 version = 0;
            uint32_t                 first_block_num = 0;

            inline void check_open_files() {
               if( !open_files ) {
                  reopen();
               }
            }
            void reopen();

            void close() {
               if( block_file.is_open() )
                  block_file.close();
               if( index_file.is_open() )
                  index_file.close();
               open_files = false;
            }
      };

      void block_log_impl::reopen() {
         close();

         // open to create files if they don't exist
         //ilog("Opening block log at ${path}", ("path", my->block_file.generic_string()));
         block_file.open( LOG_WRITE_C );
         index_file.open( LOG_WRITE_C );

         close();

         block_file.open( LOG_RW_C );
         index_file.open( LOG_RW_C );

         open_files = true;
      }

      class reverse_iterator {
      public:
         reverse_iterator();
         // open a block log file and return the total number of blocks in it
         uint32_t open(const fc::path& block_file_name);
         uint64_t previous();
         uint32_t version() const { return _version; }
         uint32_t first_block_num() const { return _first_block_num; }
      private:
         void update_buffer();

         unique_file                    _file;
         uint32_t                       _version                          = 0;
         uint32_t                       _first_block_num                  = 0;
         uint32_t                       _last_block_num                   = 0;
         uint32_t                       _blocks_found                     = 0;
         uint32_t                       _blocks_expected                  = 0;
         uint64_t                       _current_position_in_file         = 0;
         uint64_t                       _eof_position_in_file             = 0;
         uint64_t                       _end_of_buffer_position           = _unset_position;
         uint64_t                       _start_of_buffer_position         = 0;
         std::unique_ptr<char[]>        _buffer_ptr;
         std::string                    _block_file_name;
         constexpr static int64_t       _unset_position                   = -1;
         constexpr static uint32_t      _buf_len                          = 1U << 24;
         constexpr static uint64_t      _position_size                    = sizeof(_current_position_in_file);
         constexpr static int           _blknum_offset_from_pos           = 14; //offset from start of block to 4 byte block number, valid for the only allowed versions (1 & 2)
      };

      constexpr uint64_t buffer_location_to_file_location(uint32_t buffer_location) { return buffer_location << 3; }
      constexpr uint32_t file_location_to_buffer_location(uint32_t file_location) { return file_location >> 3; }

      class index_writer {
      public:
         index_writer(const fc::path& block_index_name, uint32_t blocks_expected);
         void write(uint64_t pos);
         void complete();
         void update_buffer_position();
      private:
         void prepare_buffer();
         bool shift_buffer();

         unique_file                        _file;
         const std::string                  _block_index_name;
         const uint32_t                     _blocks_expected;
         uint32_t                           _block_written;
         std::unique_ptr<uint64_t[]>        _buffer_ptr;
         int64_t                            _current_position         = 0;
         int64_t                            _start_of_buffer_position = 0;
         int64_t                            _end_of_buffer_position   = 0;
         constexpr static uint64_t          _buffer_bytes             = 1U << 22;
         constexpr static uint64_t          _max_buffer_length        = file_location_to_buffer_location(_buffer_bytes);
      };
   }

   block_log::block_log(const fc::path& data_dir)
   :my(new detail::block_log_impl()) {
      open(data_dir);
   }

   block_log::block_log(block_log&& other) {
      my = std::move(other.my);
   }

   block_log::~block_log() {
      if (my) {
         flush();
         my->close();
         my.reset();
      }
   }

   void block_log::open(const fc::path& data_dir) {
      my->close();

      if (!fc::is_directory(data_dir))
         fc::create_directories(data_dir);

      my->block_file.set_file_path( data_dir / "blocks.log" );
      my->index_file.set_file_path( data_dir / "blocks.index" );

      my->reopen();

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
      auto log_size = fc::file_size( my->block_file.get_file_path() );
      auto index_size = fc::file_size( my->index_file.get_file_path() );

      if (log_size) {
         ilog("Log is nonempty");
         my->block_file.seek( 0 );
         my->version = 0;
         my->block_file.read( (char*)&my->version, sizeof(my->version) );
         EOS_ASSERT( my->version > 0, block_log_exception, "Block log was not setup properly" );
         EOS_ASSERT( my->version >= min_supported_version && my->version <= max_supported_version, block_log_unsupported_version,
                 "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
                 ("version", my->version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );


         my->genesis_written_to_block_log = true; // Assume it was constructed properly.
         if (my->version > 1){
            my->first_block_num = 0;
            my->block_file.read( (char*)&my->first_block_num, sizeof(my->first_block_num) );
            EOS_ASSERT(my->first_block_num > 0, block_log_exception, "Block log is malformed, first recorded block number is 0 but must be greater than or equal to 1");
         } else {
            my->first_block_num = 1;
         }

         my->head = read_head();
         if( my->head ) {
            my->head_id = my->head->id();
         } else {
            my->head_id = {};
         }

         if (index_size) {
            ilog("Index is nonempty");
            uint64_t block_pos;
            my->block_file.seek_end(-sizeof(uint64_t));
            my->block_file.read((char*)&block_pos, sizeof(block_pos));

            uint64_t index_pos;
            my->index_file.seek_end(-sizeof(uint64_t));
            my->index_file.read((char*)&index_pos, sizeof(index_pos));

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
         my->close();
         fc::remove_all( my->index_file.get_file_path() );
         my->reopen();
      }
   }

   uint64_t block_log::append(const signed_block_ptr& b) {
      try {
         EOS_ASSERT( my->genesis_written_to_block_log, block_log_append_fail, "Cannot append to block log until the genesis is first written" );

         my->check_open_files();

         my->block_file.seek_end(0);
         my->index_file.seek_end(0);
         uint64_t pos = my->block_file.tellp();
         EOS_ASSERT(my->index_file.tellp() == sizeof(uint64_t) * (b->block_num() - my->first_block_num),
                   block_log_append_fail,
                   "Append to index file occuring at wrong position.",
                   ("position", (uint64_t) my->index_file.tellp())
                   ("expected", (b->block_num() - my->first_block_num) * sizeof(uint64_t)));
         auto data = fc::raw::pack(*b);
         my->block_file.write(data.data(), data.size());
         my->block_file.write((char*)&pos, sizeof(pos));
         my->index_file.write((char*)&pos, sizeof(pos));
         my->head = b;
         my->head_id = b->id();

         flush();

         return pos;
      }
      FC_LOG_AND_RETHROW()
   }

   void block_log::flush() {
      my->block_file.flush();
      my->index_file.flush();
   }

   void block_log::reset( const genesis_state& gs, const signed_block_ptr& first_block, uint32_t first_block_num ) {
      my->close();

      fc::remove_all( my->block_file.get_file_path() );
      fc::remove_all( my->index_file.get_file_path() );

      my->reopen();

      auto data = fc::raw::pack(gs);
      my->version = 0; // version of 0 is invalid; it indicates that the genesis was not properly written to the block log
      my->first_block_num = first_block_num;
      my->block_file.seek_end(0);
      my->block_file.write((char*)&my->version, sizeof(my->version));
      my->block_file.write((char*)&my->first_block_num, sizeof(my->first_block_num));
      my->block_file.write(data.data(), data.size());
      my->genesis_written_to_block_log = true;

      // append a totem to indicate the division between blocks and header
      auto totem = npos;
      my->block_file.write((char*)&totem, sizeof(totem));

      if (first_block) {
         append(first_block);
      } else {
         my->head.reset();
         my->head_id = {};
      }

      auto pos = my->block_file.tellp();

      static_assert( block_log::max_supported_version > 0, "a version number of zero is not supported" );
      my->version = block_log::max_supported_version;
      my->block_file.seek( 0 );
      my->block_file.write( (char*)&my->version, sizeof(my->version) );
      my->block_file.seek( pos );
      flush();
   }

   signed_block_ptr block_log::read_block(uint64_t pos)const {
      my->check_open_files();

      my->block_file.seek(pos);
      signed_block_ptr result = std::make_shared<signed_block>();
      auto ds = my->block_file.create_datastream();
      fc::raw::unpack(ds, *result);
      return result;
   }

   signed_block_ptr block_log::read_block_by_num(uint32_t block_num)const {
      try {
         signed_block_ptr b;
         uint64_t pos = get_block_pos(block_num);
         if (pos != npos) {
            b = read_block(pos);
            EOS_ASSERT(b->block_num() == block_num, reversible_blocks_exception,
                      "Wrong block was read from block log.", ("returned", b->block_num())("expected", block_num));
         }
         return b;
      } FC_LOG_AND_RETHROW()
   }

   uint64_t block_log::get_block_pos(uint32_t block_num) const {
      my->check_open_files();
      if (!(my->head && block_num <= block_header::num_from_id(my->head_id) && block_num >= my->first_block_num))
         return npos;
      my->index_file.seek(sizeof(uint64_t) * (block_num - my->first_block_num));
      uint64_t pos;
      my->index_file.read((char*)&pos, sizeof(pos));
      return pos;
   }

   signed_block_ptr block_log::read_head()const {
      my->check_open_files();

      uint64_t pos;

      // Check that the file is not empty
      my->block_file.seek_end(0);
      if (my->block_file.tellp() <= sizeof(pos))
         return {};

      my->block_file.seek_end(-sizeof(pos));
      my->block_file.read((char*)&pos, sizeof(pos));
      if (pos != npos) {
         return read_block(pos);
      } else {
         return {};
      }
   }

   const signed_block_ptr& block_log::head()const {
      return my->head;
   }

   uint32_t block_log::first_block_num() const {
      return my->first_block_num;
   }

   void block_log::construct_index() {
      ilog("Reconstructing Block Log Index...");
      my->close();

      fc::remove_all( my->index_file.get_file_path() );

      my->reopen();


      my->close();

      block_log::construct_index(my->block_file.get_file_path(), my->index_file.get_file_path());

      my->reopen();
   } // construct_index

   void block_log::construct_index(const fc::path& block_file_name, const fc::path& index_file_name) {
      detail::reverse_iterator block_log_iter;

      ilog("Will read existing blocks.log file ${file}", ("file", block_file_name.generic_string()));
      ilog("Will write new blocks.index file ${file}", ("file", index_file_name.generic_string()));

      const uint32_t num_blocks = block_log_iter.open(block_file_name);

      ilog("block log version= ${version}", ("version", block_log_iter.version()));

      if (num_blocks == 0) {
         return;
      }

      ilog("first block= ${first}         last block= ${last}",
           ("first", block_log_iter.first_block_num())("last", (block_log_iter.first_block_num() + num_blocks)));

      detail::index_writer index(index_file_name, num_blocks);
      uint64_t position;
      while ((position = block_log_iter.previous()) != npos) {
         index.write(position);
      }
      index.complete();
   }

   fc::path block_log::repair_log( const fc::path& data_dir, uint32_t truncate_at_block ) {
      ilog("Recovering Block Log...");
      EOS_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"), block_log_not_found,
                 "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );

      auto now = fc::time_point::now();

      auto blocks_dir = fc::canonical( data_dir );
      if( blocks_dir.filename().generic_string() == "." ) {
         blocks_dir = blocks_dir.parent_path();
      }
      auto backup_dir = blocks_dir.parent_path();
      auto blocks_dir_name = blocks_dir.filename();
      EOS_ASSERT( blocks_dir_name.generic_string() != ".", block_log_exception, "Invalid path to blocks directory" );
      backup_dir = backup_dir / blocks_dir_name.generic_string().append("-").append( now );

      EOS_ASSERT( !fc::exists(backup_dir), block_log_backup_dir_exist,
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
      EOS_ASSERT( version > 0, block_log_exception, "Block log was not setup properly" );
      EOS_ASSERT( version >= min_supported_version && version <= max_supported_version, block_log_unsupported_version,
                 "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
                 ("version", version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );

      new_block_stream.write( (char*)&version, sizeof(version) );

      uint32_t first_block_num = 1;
      if (version != 1) {
         old_block_stream.read ( (char*)&first_block_num, sizeof(first_block_num) );
         new_block_stream.write( (char*)&first_block_num, sizeof(first_block_num) );
      }

      genesis_state gs;
      fc::raw::unpack(old_block_stream, gs);

      auto data = fc::raw::pack( gs );
      new_block_stream.write( data.data(), data.size() );

      if (version != 1) {
         auto expected_totem = npos;
         std::decay_t<decltype(npos)> actual_totem;
         old_block_stream.read ( (char*)&actual_totem, sizeof(actual_totem) );

         EOS_ASSERT(actual_totem == expected_totem, block_log_exception,
                    "Expected separator between block log header and blocks was not found( expected: ${e}, actual: ${a} )",
                    ("e", fc::to_hex((char*)&expected_totem, sizeof(expected_totem) ))("a", fc::to_hex((char*)&actual_totem, sizeof(actual_totem) )));

         new_block_stream.write( (char*)&actual_totem, sizeof(actual_totem) );
      }

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
            bad_block.emplace(std::move(tmp));
            break;
         }

         auto data = fc::raw::pack(tmp);
         new_block_stream.write( data.data(), data.size() );
         new_block_stream.write( reinterpret_cast<char*>(&pos), sizeof(pos) );
         block_num = tmp.block_num();
         if(block_num % 1000 == 0)
            ilog( "Recovered block ${num}", ("num", block_num) );
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
      EOS_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"), block_log_not_found,
                 "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );

      std::fstream  block_stream;
      block_stream.open( (data_dir / "blocks.log").generic_string().c_str(), LOG_READ );

      uint32_t version = 0;
      block_stream.read( (char*)&version, sizeof(version) );
      EOS_ASSERT( version > 0, block_log_exception, "Block log was not setup properly." );
      EOS_ASSERT( version >= min_supported_version && version <= max_supported_version, block_log_unsupported_version,
                 "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
                 ("version", version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );

      uint32_t first_block_num = 1;
      if (version != 1) {
         block_stream.read ( (char*)&first_block_num, sizeof(first_block_num) );
      }

      genesis_state gs;
      fc::raw::unpack(block_stream, gs);
      return gs;
   }

   detail::reverse_iterator::reverse_iterator()
   : _file(nullptr, &fclose)
   , _buffer_ptr(std::make_unique<char[]>(_buf_len)) {
   }

   uint32_t detail::reverse_iterator::open(const fc::path& block_file_name) {
      _block_file_name = block_file_name.generic_string();
      _file.reset( FC_FOPEN(_block_file_name.c_str(), "r"));
      EOS_ASSERT( _file, block_log_exception, "Could not open Block log file at '${blocks_log}'", ("blocks_log", _block_file_name) );
      _end_of_buffer_position = _unset_position;

      //read block log to see if version 1 or 2 and get first blocknum (implicit 1 if version 1)
      _version = 0;
      auto size = fread((char*)&_version, sizeof(_version), 1, _file.get());
      EOS_ASSERT( size == 1, block_log_exception, "Block log file at '${blocks_log}' could not be read.", ("file", _block_file_name) );
      EOS_ASSERT( _version == 1 || _version == 2, block_log_unsupported_version, "block log version ${v} is not supported", ("v", _version));
      if (_version == 1) {
         _first_block_num = 1;
      }
      else {
         size = fread((char*)&_first_block_num, sizeof(_first_block_num), 1, _file.get());
         EOS_ASSERT( size == 1, block_log_exception, "Block log file at '${blocks_log}' not formatted consistently with version ${v}.", ("file", _block_file_name)("v", _version) );
      }

      auto status = fseek(_file.get(), 0, SEEK_END);
      EOS_ASSERT( status == 0, block_log_exception, "Could not open Block log file at '${blocks_log}'. Returned status: ${status}", ("blocks_log", _block_file_name)("status", status) );

      _eof_position_in_file = ftell(_file.get());
      EOS_ASSERT( _eof_position_in_file > 0, block_log_exception, "Block log file at '${blocks_log}' could not be read.", ("blocks_log", _block_file_name) );
      _current_position_in_file = _eof_position_in_file - _position_size;

      update_buffer();

      _blocks_found = 0;
      char* buf = _buffer_ptr.get();
      const uint32_t index_of_pos = _current_position_in_file - _start_of_buffer_position;
      const uint64_t block_pos = *reinterpret_cast<uint64_t*>(buf + index_of_pos);

      if (block_pos == block_log::npos) {
         return 0;
      }

      uint32_t bnum = 0;
      if (block_pos >= _start_of_buffer_position) {
         const uint32_t index_of_block = block_pos - _start_of_buffer_position;
         bnum = *reinterpret_cast<uint32_t*>(buf + index_of_block + _blknum_offset_from_pos);  //block number of previous block (is big endian)
      }
      else {
         const auto blknum_offset_pos = block_pos + _blknum_offset_from_pos;
         auto status = fseek(_file.get(), blknum_offset_pos, SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "Could not seek in '${blocks_log}' to position: ${pos}. Returned status: ${status}", ("blocks_log", _block_file_name)("pos", blknum_offset_pos)("status", status) );
         auto size = fread((void*)&bnum, sizeof(bnum), 1, _file.get());
         EOS_ASSERT( size == 1, block_log_exception, "Could not read in '${blocks_log}' at position: ${pos}", ("blocks_log", _block_file_name)("pos", blknum_offset_pos) );
      }
      _last_block_num = fc::endian_reverse_u32(bnum) + 1;                     //convert from big endian to little endian and add 1
      _blocks_expected = _last_block_num - _first_block_num + 1;
      return _blocks_expected;
   }

   uint64_t detail::reverse_iterator::previous() {
      EOS_ASSERT( _current_position_in_file != block_log::npos,
                  block_log_exception,
                  "Block log file at '${blocks_log}' first block already returned by former call to previous(), it is no longer valid to call this function.", ("blocks_log", _block_file_name) );

      if (_start_of_buffer_position > _current_position_in_file) {
         update_buffer();
      }

      char* buf = _buffer_ptr.get();
      auto offset = _current_position_in_file - _start_of_buffer_position;
      uint64_t block_location_in_file = *reinterpret_cast<uint64_t*>(buf + offset);

      ++_blocks_found;
      if (block_location_in_file == block_log::npos) {
         _current_position_in_file = block_location_in_file;
         EOS_ASSERT( _blocks_found != _blocks_expected,
                    block_log_exception,
                    "Block log file at '${blocks_log}' formatting indicated last block: ${last_block_num}, first block: ${first_block_num}, but found ${num} blocks",
                    ("blocks_log", _block_file_name)("last_block_num", _last_block_num)("first_block_num", _first_block_num)("num", _blocks_found) );
      }
      else {
         const uint64_t previous_position_in_file = _current_position_in_file;
         _current_position_in_file = block_location_in_file - _position_size;
         EOS_ASSERT( _current_position_in_file < previous_position_in_file,
                     block_log_exception,
                     "Block log file at '${blocks_log}' formatting is incorrect, indicates position later location in file: ${pos}, which was retrieved at: ${orig_pos}.",
                     ("blocks_log", _block_file_name)("pos", _current_position_in_file)("orig_pos", previous_position_in_file) );
         if (_version == 1 && _blocks_found != _blocks_expected) {
            _current_position_in_file = block_location_in_file = block_log::npos;
         }
      }

      return block_location_in_file;
   }

   void detail::reverse_iterator::update_buffer() {
      EOS_ASSERT( _current_position_in_file != block_log::npos, block_log_exception, "Block log file not setup properly" );

      // since we need to read in a new section, just need to ensure the next position is at the very end of the buffer
      _end_of_buffer_position = _current_position_in_file + _position_size;
      if (_end_of_buffer_position < _buf_len) {
         _start_of_buffer_position = 0;
      }
      else {
         _start_of_buffer_position = _end_of_buffer_position - _buf_len;
      }

      auto status = fseek(_file.get(), _start_of_buffer_position, SEEK_SET);
      EOS_ASSERT( status == 0, block_log_exception, "Could not seek in '${blocks_log}' to position: ${pos}. Returned status: ${status}", ("blocks_log", _block_file_name)("pos", _start_of_buffer_position)("status", status) );
      char* buf = _buffer_ptr.get();
      auto size = fread((void*)buf, (_end_of_buffer_position - _start_of_buffer_position), 1, _file.get());//read tail of blocks.log file into buf
      EOS_ASSERT( size == 1, block_log_exception, "blocks.log read fails" );
   }

   detail::index_writer::index_writer(const fc::path& block_index_name, uint32_t blocks_expected)
   : _file(nullptr, &fclose)
   , _block_index_name(block_index_name.generic_string())
   , _blocks_expected(blocks_expected)
   , _block_written(blocks_expected)
   , _buffer_ptr(std::make_unique<uint64_t[]>(_max_buffer_length)) {
   }

   void detail::index_writer::write(uint64_t pos) {
      prepare_buffer();
      uint64_t* buffer = _buffer_ptr.get();
      buffer[_current_position - _start_of_buffer_position] = pos;
      --_current_position;
      if ((_block_written & 0xfffff) == 0) {                            //periodically print a progress indicator
         dlog("block: ${block_written}      position in file: ${pos}", ("block_written", _block_written)("pos",pos));
      }
      --_block_written;
   }

   void detail::index_writer::prepare_buffer() {
      if (_file == nullptr) {
         _file.reset(FC_FOPEN(_block_index_name.c_str(), "w"));
         EOS_ASSERT( _file, block_log_exception, "Could not open Block index file at '${blocks_index}'", ("blocks_index", _block_index_name) );
         // allocate 8 bytes for each block position to store
         const auto full_file_size = buffer_location_to_file_location(_blocks_expected);
         auto status = fseek(_file.get(), full_file_size, SEEK_SET);
         EOS_ASSERT( status == 0, block_log_exception, "Could not allocate in '${blocks_index}' storage for all the blocks, size: ${size}. Returned status: ${status}", ("blocks_index", _block_index_name)("size", full_file_size)("status", status) );
         const auto block_end = file_location_to_buffer_location(full_file_size);
         _current_position = block_end - 1;
         update_buffer_position();
      }

      shift_buffer();
   }

   bool detail::index_writer::shift_buffer() {
      if (_current_position >= _start_of_buffer_position) {
         return false;
      }

      const auto file_location_start = buffer_location_to_file_location(_start_of_buffer_position);

      auto status = fseek(_file.get(), file_location_start, SEEK_SET);
      EOS_ASSERT( status == 0, block_log_exception, "Could not navigate in '${blocks_index}' file_location_start: ${loc}, _start_of_buffer_position: ${_start_of_buffer_position}. Returned status: ${status}", ("blocks_index", _block_index_name)("loc", file_location_start)("_start_of_buffer_position",_start_of_buffer_position)("status", status) );

      const auto buffer_size = _end_of_buffer_position - _start_of_buffer_position;
      const auto file_size = buffer_location_to_file_location(buffer_size);
      uint64_t* buf = _buffer_ptr.get();
      auto size = fwrite((void*)buf, file_size, 1, _file.get());
      EOS_ASSERT( size == 1, block_log_exception, "Writing Block Index file '${file}' failed at location: ${loc}", ("file", _block_index_name)("loc", file_location_start) );
      update_buffer_position();
      return true;
   }

   void detail::index_writer::complete() {
      const bool shifted = shift_buffer();
      EOS_ASSERT(shifted, block_log_exception, "Failed to write buffer to '${blocks_index}'", ("blocks_index", _block_index_name) );
      EOS_ASSERT(_current_position == -1,
                 block_log_exception,
                 "Should have written buffer, starting at the 0 index block position, to '${blocks_index}' but instead writing ${pos} position",
                 ("blocks_index", _block_index_name)("pos", _current_position) );
   }

   void detail::index_writer::update_buffer_position() {
      _end_of_buffer_position = _current_position + 1;
      if (_end_of_buffer_position < _max_buffer_length) {
         _start_of_buffer_position = 0;
      }
      else {
         _start_of_buffer_position = _end_of_buffer_position - _max_buffer_length;
      }
   }
} } /// eosio::chain
