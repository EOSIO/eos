#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fstream>
#include <fc/bitutil.hpp>
#include <fc/io/cfile.hpp>
#include <fc/io/raw.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>
#include <variant>

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
    * Version 3: improvement on version 2 to not require the genesis state be provided when not starting
    *            from block 1
    * Version 4: changes the block entry from the serialization of signed_block to a tuple of offset to next entry,
    *            compression_status and pruned_block.
    */
   const uint32_t block_log::max_supported_version = 4;

   namespace detail {
      using unique_file = std::unique_ptr<FILE, decltype(&fclose)>;

      /// calculate the offset from the start of serialized block entry to block start
      int offset_to_block_start(uint32_t version) { 
         if (version < 4) return 0;
         return sizeof(uint32_t) + 1;
      }

      class log_entry_v4 : public signed_block {
      public:
         packed_transaction::cf_compression_type compression = packed_transaction::cf_compression_type::none;
         uint32_t offset = 0;
      };


      template <typename Stream>
      void unpack(Stream& ds, log_entry_v4& entry){
         auto start_pos = ds.tellp();
         fc::raw::unpack(ds, entry.offset);
         uint8_t compression;
         fc::raw::unpack(ds, compression);
         entry.compression = static_cast<packed_transaction::cf_compression_type>(compression);
         EOS_ASSERT(entry.compression == packed_transaction::cf_compression_type::none, block_log_exception,
                  "Only support compression_type none");
         fc::raw::unpack(ds, static_cast<signed_block&>(entry));
         EOS_ASSERT(ds.tellp() - start_pos + sizeof(uint64_t) == entry.offset , block_log_exception,
                  "Invalid block log entry offset");
      }

      std::vector<char> pack(const signed_block& block, packed_transaction::cf_compression_type compression) {
         // In version 4 of the irreversible blocks log format, these log entries consists of the following in order:
         //    1. An uint32_t offset from the start of this log entry to the start of the next log entry.
         //    2. An uint8_t indicating the compression status for the serialization of the pruned_block following this.
         //    3. The serialization of a pruned_block representation of the block for the entry including padding.

         std::size_t padded_size = block.maximum_pruned_pack_size(compression);
         std::vector<char> buffer(padded_size + offset_to_block_start(4));
         fc::datastream<char*> stream(buffer.data(), buffer.size());

         uint32_t offset      = buffer.size() + sizeof(uint64_t);
         stream.write((char*)&offset, sizeof(offset));
         fc::raw::pack(stream, static_cast<uint8_t>(compression));
         block.pack(stream, compression);
         return buffer;
      }

      std::vector<char> pack(const log_entry_v4& entry) {
         return pack(static_cast<const signed_block&>(entry), entry.compression);
      }

      using log_entry = std::variant<log_entry_v4, signed_block>;

      template <typename Stream>
      void unpack(Stream& ds, log_entry& entry) {
         std::visit(
             overloaded{[&ds](signed_block& v) { fc::raw::unpack(ds, v); }, [&ds](log_entry_v4& v) { unpack(ds, v); }},
             entry);
      }

      std::vector<char> pack(const log_entry& entry) {
         return std::visit(overloaded{[](const signed_block& v) { return fc::raw::pack(v); },
                                      [](const log_entry_v4& v) { return pack(v); }},
                           entry);
      }

      class block_log_impl {
         public:
            signed_block_ptr head;
            fc::cfile        block_file;
            fc::cfile        index_file;
            bool             open_files                   = false;
            bool             genesis_written_to_block_log = false;
            uint32_t         version                      = 0;
            uint32_t         first_block_num              = 0;

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

            template<typename T>
            void reset( const T& t, const signed_block_ptr& first_block,
                        packed_transaction::cf_compression_type segment_compression, uint32_t first_block_num );

            void write( const genesis_state& gs );

            void write( const chain_id_type& chain_id );

            void flush();

            uint64_t append(const signed_block_ptr& b, packed_transaction::cf_compression_type segment_compression);

            template <typename ChainContext, typename Lambda>
            static fc::optional<ChainContext> extract_chain_context( const fc::path& data_dir, Lambda&& lambda );

            uint64_t write_log_entry(const signed_block& b,
                                     packed_transaction::cf_compression_type segment_compression);

            void read_block_header(block_header& bh, uint64_t file_pos);
            std::unique_ptr<signed_block> read_block(uint64_t pos);
            void read_head();

            

            static int blknum_offset_from_block_entry(uint32_t block_log_version) { 

               //to derive blknum_offset==14 see block_header.hpp and note on disk struct is packed
               //   block_timestamp_type timestamp;                  //bytes 0:3
               //   account_name         producer;                   //bytes 4:11
               //   uint16_t             confirmed;                  //bytes 12:13
               //   block_id_type        previous;                   //bytes 14:45, low 4 bytes is big endian block number of previous block
               
               int blknum_offset = 14;
               blknum_offset += detail::offset_to_block_start(block_log_version);
               return blknum_offset;
            }
      };

      void detail::block_log_impl::reopen() {
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
         constexpr static uint32_t      _buf_len                          = 1U << 24;
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
         constexpr static uint64_t      _position_size                    = sizeof(_current_position_in_file);
      };

      constexpr uint64_t buffer_location_to_file_location(uint32_t buffer_location) { return buffer_location << 3; }
      constexpr uint32_t file_location_to_buffer_location(uint32_t file_location) { return file_location >> 3; }

      class index_writer {
      public:
         index_writer(const fc::path& block_index_name, uint32_t blocks_expected);
         void write(uint64_t pos);
         void complete();
         void update_buffer_position();
         constexpr static uint64_t          _buffer_bytes             = 1U << 22;
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
         constexpr static uint64_t          _max_buffer_length        = file_location_to_buffer_location(_buffer_bytes);
      };

      /*
       *  @brief datastream adapter that adapts FILE* for use with fc unpack
       *
       *  This class supports unpack functionality but not pack.
       */
      class fileptr_datastream {
      public:
         explicit fileptr_datastream( FILE* file, const std::string& filename ) : _file(file), _filename(filename) {}

         void skip( size_t s ) {
            auto status = fseek(_file, s, SEEK_CUR);
            EOS_ASSERT( status == 0, block_log_exception,
                        "Could not seek past ${bytes} bytes in Block log file at '${blocks_log}'. Returned status: ${status}",
                        ("bytes", s)("blocks_log", _filename)("status", status) );
         }

         bool read( char* d, size_t s ) {
            size_t result = fread( d, 1, s, _file );
            EOS_ASSERT( result == s, block_log_exception,
                        "only able to read ${act} bytes of the expected ${exp} bytes in file: ${file}",
                        ("act",result)("exp",s)("file", _filename) );
            return true;
         }

         bool get( unsigned char& c ) { return get( *(char*)&c ); }

         bool get( char& c ) { return read(&c, 1); }

      private:
         FILE* const _file;
         const std::string _filename;
      };
}}} // namespace eosio::chain::detail

FC_REFLECT_DERIVED(eosio::chain::detail::log_entry_v4, (eosio::chain::signed_block), (compression)(offset) )

namespace eosio { namespace chain {

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
         EOS_ASSERT( is_supported_version(my->version), block_log_unsupported_version,
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

         my->read_head();

         if (index_size) {
            ilog("Index is nonempty");
            uint64_t block_pos;
            my->block_file.seek_end(-sizeof(uint64_t));
            my->block_file.read((char*)&block_pos, sizeof(block_pos));

            uint64_t index_pos;
            my->index_file.seek_end(-sizeof(uint64_t));
            my->index_file.read((char*)&index_pos, sizeof(index_pos));

            if (block_pos < index_pos) {
               ilog("block_pos < index_pos, close and reopen index_file");
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

   uint64_t detail::block_log_impl::write_log_entry(const signed_block& b, packed_transaction::cf_compression_type segment_compression) {
      uint64_t pos = block_file.tellp();
      std::vector<char> buffer;
     
      if (version >= 4)  {
         buffer = detail::pack(b, segment_compression);
      } else {
#warning: TODO avoid heap allocation
         auto block_ptr = b.to_signed_block_v0();
         EOS_ASSERT(block_ptr, block_log_append_fail, "Unable to convert block to legacy format");
         EOS_ASSERT(segment_compression == packed_transaction::cf_compression_type::none, block_log_append_fail,
            "the compression must be \"none\" for legacy format");
         buffer = fc::raw::pack(*block_ptr);
      }
      block_file.write(buffer.data(), buffer.size());
      block_file.write((char*)&pos, sizeof(pos));
      index_file.write((char*)&pos, sizeof(pos));
      return pos;
   }

   uint64_t block_log::append(const signed_block_ptr& b, packed_transaction::cf_compression_type segment_compression) {
      return my->append(b, segment_compression);
   }

   uint64_t detail::block_log_impl::append(const  signed_block_ptr& b, packed_transaction::cf_compression_type segment_compression) {
      try {
         EOS_ASSERT( genesis_written_to_block_log, block_log_append_fail, "Cannot append to block log until the genesis is first written" );

         check_open_files();

         block_file.seek_end(0);
         index_file.seek_end(0);
         EOS_ASSERT(index_file.tellp() == sizeof(uint64_t) * (b->block_num() - first_block_num),
                   block_log_append_fail,
                   "Append to index file occuring at wrong position.",
                   ("position", (uint64_t) index_file.tellp())
                   ("expected", (b->block_num() - first_block_num) * sizeof(uint64_t)));

         auto pos = write_log_entry(*b, segment_compression);

         head = b;

         flush();

         return pos;
      }
      FC_LOG_AND_RETHROW()
   }

   void block_log::flush() {
      my->flush();
   }

   void detail::block_log_impl::flush() {
      block_file.flush();
      index_file.flush();
   }

   template<typename T>
   void detail::block_log_impl::reset( const T& t, const signed_block_ptr& first_block, packed_transaction::cf_compression_type segment_compression, uint32_t first_bnum ) {
      close();

      fc::remove_all( block_file.get_file_path() );
      fc::remove_all( index_file.get_file_path() );

      reopen();

      version = 0; // version of 0 is invalid; it indicates that subsequent data was not properly written to the block log
      first_block_num = first_bnum;

      block_file.seek_end(0);
      block_file.write((char*)&version, sizeof(version));
      block_file.write((char*)&first_block_num, sizeof(first_block_num));

      write(t);
      genesis_written_to_block_log = true;

      // append a totem to indicate the division between blocks and header
      auto totem = block_log::npos;
      block_file.write((char*)&totem, sizeof(totem));

      // version must be assigned before this->append() because it is used inside this->append()
      version = block_log::max_supported_version;

      if (first_block) {
         append(first_block, segment_compression);
      } else {
         head.reset();
      }

      auto pos = block_file.tellp();

      static_assert( block_log::max_supported_version > 0, "a version number of zero is not supported" );

      // going back to write correct version to indicate that all block log header data writes completed successfully
      block_file.seek( 0 );
      block_file.write( (char*)&version, sizeof(version) );
      block_file.seek( pos );
      flush();
   }

   void block_log::reset( const genesis_state& gs, const signed_block_ptr& first_block, packed_transaction::cf_compression_type segment_compression ) {
      my->reset(gs, first_block, segment_compression, 1);
   }

   void block_log::reset( const chain_id_type& chain_id, uint32_t first_block_num ) {
      EOS_ASSERT( first_block_num > 1, block_log_exception,
                  "Block log version ${ver} needs to be created with a genesis state if starting from block number 1." );
      my->reset(chain_id, signed_block_ptr(), packed_transaction::cf_compression_type::none, first_block_num);
   }

   void detail::block_log_impl::write( const genesis_state& gs ) {
      auto data = fc::raw::pack(gs);
      block_file.write(data.data(), data.size());
   }

   void detail::block_log_impl::write( const chain_id_type& chain_id ) {
      block_file << chain_id;
   }

   std::unique_ptr<signed_block> detail::block_log_impl::read_block(uint64_t pos) {
      block_file.seek(pos);
      auto ds = block_file.create_datastream();
      if (version >= 4) {
         auto entry = std::make_unique<log_entry_v4>();
         unpack(ds, *entry);
         return entry;
      } else {
         signed_block_v0 block;
         fc::raw::unpack(ds, block);
         return std::make_unique<signed_block>(std::move(block), true);
      }
   }

   void detail::block_log_impl::read_block_header(block_header& bh, uint64_t pos) {
      block_file.seek(pos);
      auto ds = block_file.create_datastream();

      if (version >= 4 ) {
         uint32_t offset;
         uint8_t  compression;
         fc::raw::unpack(ds, offset);
         fc::raw::unpack(ds, compression);
         EOS_ASSERT( compression == static_cast<uint8_t>(packed_transaction::cf_compression_type::none), block_log_exception ,
                     "Only \"none\" compression type is supported.");
      }
      fc::raw::unpack(ds, bh);
   }

   std::unique_ptr<signed_block> block_log::read_signed_block_by_num(uint32_t block_num) const {
      try {
         std::unique_ptr<signed_block> b;
         uint64_t pos = get_block_pos(block_num);
         if (pos != npos) {
            b = my->read_block(pos);
            EOS_ASSERT(b->block_num() == block_num, block_log_exception,
                      "Wrong block was read from block log.");
         }
         return b;
      } FC_LOG_AND_RETHROW()
   }

   block_id_type block_log::read_block_id_by_num(uint32_t block_num)const {
      try {
         uint64_t pos = get_block_pos(block_num);
         if (pos != npos) {
            block_header bh;
            my->read_block_header(bh, pos);
            EOS_ASSERT(bh.block_num() == block_num, reversible_blocks_exception,
                       "Wrong block header was read from block log.", ("returned", bh.block_num())("expected", block_num));
            return bh.calculate_id();
         }
         return {};
      } FC_LOG_AND_RETHROW()
   }

   uint64_t block_log::get_block_pos(uint32_t block_num) const {
      my->check_open_files();
      if (!(my->head && block_num <= my->head->block_num() && block_num >= my->first_block_num))
         return npos;
      my->index_file.seek(sizeof(uint64_t) * (block_num - my->first_block_num));
      uint64_t pos;
      my->index_file.read((char*)&pos, sizeof(pos));
      return pos;
   }


   void detail::block_log_impl::read_head() {
      uint64_t pos;

      block_file.seek_end(-sizeof(pos));
      block_file.read((char*)&pos, sizeof(pos));
      if (pos != block_log::npos) {
         head = read_block(pos);
      } 
   }

   const signed_block_ptr& block_log::head() const {
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

   fc::path block_log::repair_log(const fc::path& data_dir, uint32_t truncate_at_block) {
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
      EOS_ASSERT( is_supported_version(version), block_log_unsupported_version,
                 "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
                 ("version", version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );

      new_block_stream.write( (char*)&version, sizeof(version) );

      uint32_t first_block_num = 1;
      if (version != 1) {
         old_block_stream.read ( (char*)&first_block_num, sizeof(first_block_num) );

         // this assert is only here since repair_log is only used for --hard-replay-blockchain, which removes any
         // existing state, if another API needs to use it, this can be removed and the check for the first block's
         // previous block id will need to accommodate this.
         EOS_ASSERT( first_block_num == 1, block_log_exception,
                     "Block log ${file} must contain a genesis state and start at block number 1.  This block log "
                     "starts at block number ${first_block_num}.",
                     ("file", (backup_dir / "blocks.log").generic_string())("first_block_num", first_block_num));

         new_block_stream.write( (char*)&first_block_num, sizeof(first_block_num) );
      }

      if (contains_genesis_state(version, first_block_num)) {
         genesis_state gs;
         fc::raw::unpack(old_block_stream, gs);

         auto data = fc::raw::pack( gs );
         new_block_stream.write( data.data(), data.size() );
      }
      else if (contains_chain_id(version, first_block_num)) {
         chain_id_type chain_id;
         old_block_stream >> chain_id;

         new_block_stream << chain_id;
      }
      else {
         EOS_THROW( block_log_exception,
                    "Block log ${file} is not supported. version: ${ver} and first_block_num: ${fbn} does not contain "
                    "a genesis_state nor a chain_id.",
                    ("file", (backup_dir / "blocks.log").generic_string())("ver", version)("fbn", first_block_num));
      }

      if (version != 1) {
         auto expected_totem = npos;
         std::decay_t<decltype(npos)> actual_totem;
         old_block_stream.read ( (char*)&actual_totem, sizeof(actual_totem) );

         EOS_ASSERT(actual_totem == expected_totem, block_log_exception,
                    "Expected separator between block log header and blocks was not found( expected: ${e}, actual: ${a} )",
                    ("e", fc::to_hex((char*)&expected_totem, sizeof(expected_totem) ))("a", fc::to_hex((char*)&actual_totem, sizeof(actual_totem) )));

         new_block_stream.write( (char*)&actual_totem, sizeof(actual_totem) );
      }

      std::exception_ptr          except_ptr;
      vector<char>                incomplete_block_data;
      optional<detail::log_entry> bad_block;
      uint32_t                    block_num = 0;

      block_id_type expected_previous;

      uint64_t pos = old_block_stream.tellg();

      detail::log_entry entry;
      if (version < 4) {
         entry.emplace<signed_block>();
      }

      while( pos < end_pos ) {
         try {
            unpack(old_block_stream, entry);
         } catch (...) {
            except_ptr = std::current_exception();
            incomplete_block_data.resize( end_pos - pos );
            old_block_stream.read( incomplete_block_data.data(), incomplete_block_data.size() );
            break;
         }

         auto id = std::visit([](const auto& v) { return v.calculate_id(); }, entry);
         if( block_header::num_from_id(expected_previous) + 1 != block_header::num_from_id(id) ) {
            elog( "Block ${num} (${id}) skips blocks. Previous block in block log is block ${prev_num} (${previous})",
                  ("num", block_header::num_from_id(id))("id", id)
                  ("prev_num", block_header::num_from_id(expected_previous))("previous", expected_previous) );
         }
         auto actual_previous = std::visit([](const auto& v) { return v.previous; }, entry);
         if( expected_previous != actual_previous ) {
            elog( "Block ${num} (${id}) does not link back to previous block. "
                  "Expected previous: ${expected}. Actual previous: ${actual}.",
                  ("num", block_header::num_from_id(id))("id", id)("expected", expected_previous)("actual", actual_previous) );
         }
         expected_previous = id;

         uint64_t tmp_pos = std::numeric_limits<uint64_t>::max();
         if( (static_cast<uint64_t>(old_block_stream.tellg()) + sizeof(pos)) <= end_pos ) {
            old_block_stream.read( reinterpret_cast<char*>(&tmp_pos), sizeof(tmp_pos) );
         }
         if( pos != tmp_pos ) {
            bad_block.emplace(std::move(entry));
            break;
         }

         auto data = detail::pack(entry);
         new_block_stream.write( data.data(), data.size() );
         new_block_stream.write( reinterpret_cast<char*>(&pos), sizeof(pos) );
         block_num = std::visit([](const auto& v) { return v.block_num(); }, entry);
         if(block_num % 1000 == 0)
            ilog( "Recovered block ${num}", ("num", block_num) );
         pos = new_block_stream.tellp();
         if( block_num == truncate_at_block )
            break;
      }

      if( bad_block.valid() ) {
         ilog("Recovered only up to block number ${num}. Last block in block log was not properly committed", ("num", block_num));
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

   template <typename ChainContext, typename Lambda>
   fc::optional<ChainContext> detail::block_log_impl::extract_chain_context( const fc::path& data_dir, Lambda&& lambda ) {
      EOS_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"), block_log_not_found,
                  "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );

      std::fstream  block_stream;
      block_stream.open( (data_dir / "blocks.log").generic_string().c_str(), LOG_READ );

      uint32_t version = 0;
      block_stream.read( (char*)&version, sizeof(version) );
      EOS_ASSERT( version >= block_log::min_supported_version && version <= block_log::max_supported_version, block_log_unsupported_version,
                  "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
                  ("version", version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );

      uint32_t first_block_num = 1;
      if (version != 1) {
         block_stream.read ( (char*)&first_block_num, sizeof(first_block_num) );
      }

      return lambda(block_stream, version, first_block_num);
   }

   fc::optional<genesis_state> block_log::extract_genesis_state( const fc::path& data_dir ) {
      return detail::block_log_impl::extract_chain_context<genesis_state>(data_dir, [](std::fstream& block_stream, uint32_t version, uint32_t first_block_num ) -> fc::optional<genesis_state> {
         if (contains_genesis_state(version, first_block_num)) {
            genesis_state gs;
            fc::raw::unpack(block_stream, gs);
            return gs;
         }

         // current versions only have a genesis state if they start with block number 1
         return fc::optional<genesis_state>();
      });
   }

   chain_id_type block_log::extract_chain_id( const fc::path& data_dir ) {
      return *(detail::block_log_impl::extract_chain_context<chain_id_type>(data_dir, [](std::fstream& block_stream, uint32_t version, uint32_t first_block_num ) -> fc::optional<chain_id_type> {
         // supported versions either contain a genesis state, or else the chain id only
         if (contains_genesis_state(version, first_block_num)) {
            genesis_state gs;
            fc::raw::unpack(block_stream, gs);
            return gs.compute_chain_id();
         }
         EOS_ASSERT( contains_chain_id(version, first_block_num), block_log_exception,
                     "Block log error! version: ${version} with first_block_num: ${num} does not contain a "
                     "chain id or genesis state, so the chain id cannot be determined.",
                     ("version", version)("num", first_block_num) );
         chain_id_type chain_id;
         fc::raw::unpack(block_stream, chain_id);
         return chain_id;
      }));
   }

   bool prune(packed_transaction& ptx) {
      ptx.prune_all();
      return true;
   }
   
   void block_log::prune_transactions(uint32_t block_num, const std::vector<transaction_id_type>& ids) {
      try {
         EOS_ASSERT( my->version >= 4, block_log_exception,
                     "The block log version ${version} does not support transaction pruning.", ("version", my->version) );
         uint64_t pos = get_block_pos(block_num);
         EOS_ASSERT( pos != npos, block_log_exception,
                     "Specified block_num ${block_num} does not exist in block log.", ("block_num", block_num) );

         detail::log_entry_v4 entry;   
         my->block_file.seek(pos);
         auto ds = my->block_file.create_datastream();
         unpack(ds, entry);

         EOS_ASSERT(entry.block_num() == block_num, block_log_exception,
                     "Wrong block was read from block log.");

         auto pruner = overloaded{[](transaction_id_type&) { return false; },
                                  [&ids](packed_transaction& ptx) { return  std::find(ids.begin(), ids.end(), ptx.id()) != ids.end() && prune(ptx); }};

         bool pruned = false;
         for (auto& trx : entry.transactions) {
            pruned |= trx.trx.visit(pruner);
         }

         if (pruned) {
            my->block_file.seek(pos);
            std::vector<char> buffer = detail::pack(entry);
            EOS_ASSERT(buffer.size() <= entry.offset, block_log_exception, "Not enough space reserved in block log entry to serialize pruned block.");
            my->block_file.write(buffer.data(), buffer.size());
            my->block_file.flush();
         }
      }
      FC_LOG_AND_RETHROW()
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

      // TODO: reimplemented with mapped_file_source and verify_log_preamble_and_return_version()

      //read block log to see if version 1 or 2 and get first blocknum (implicit 1 if version 1)
      _version = 0;
      auto size = fread((char*)&_version, sizeof(_version), 1, _file.get());
      EOS_ASSERT( size == 1, block_log_exception, "Block log file at '${blocks_log}' could not be read.", ("file", _block_file_name) );
      EOS_ASSERT( block_log::is_supported_version(_version), block_log_unsupported_version,
                  "block log version ${v} is not supported", ("v", _version));
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
         bnum = *reinterpret_cast<uint32_t*>(buf + index_of_block + block_log_impl::blknum_offset_from_block_entry(_version));  //block number of previous block (is big endian)
      } else {
         const auto blknum_offset_pos = block_pos + block_log_impl::blknum_offset_from_block_entry(_version);
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

      if (_version == 1 && _blocks_found == _blocks_expected) {
         _current_position_in_file = block_log::npos;
         return _current_position_in_file;
      }
	 
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
         ilog("block: ${block_written}      position in file: ${pos}", ("block_written", _block_written)("pos",pos));
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

   bool block_log::contains_genesis_state(uint32_t version, uint32_t first_block_num) {
      return version <= 2 || first_block_num == 1;
   }

   bool block_log::contains_chain_id(uint32_t version, uint32_t first_block_num) {
      return version >= 3 && first_block_num > 1;
   }

   bool block_log::is_supported_version(uint32_t version) {
      return std::clamp(version, min_supported_version, max_supported_version) == version;
   }

   struct trim_data {            //used by trim_blocklog_front(), trim_blocklog_end(), and smoke_test()
      trim_data(fc::path block_dir);
      ~trim_data();
      uint64_t block_index(uint32_t n) const;
      uint64_t block_pos(uint32_t n);
      fc::path block_file_name, index_file_name;        //full pathname for blocks.log and blocks.index
      uint32_t version = 0;                              //blocklog version
      uint32_t first_block = 0;                          //first block in blocks.log
      uint32_t last_block = 0;                          //last block in blocks.log
      FILE* blk_in = nullptr;                            //C style files for reading blocks.log and blocks.index
      FILE* ind_in = nullptr;                            //C style files for reading blocks.log and blocks.index
      //we use low level file IO because it is distinctly faster than C++ filebuf or iostream
      uint64_t first_block_pos = 0;                      //file position in blocks.log for the first block in the log
      chain_id_type chain_id;
   };


   bool block_log::trim_blocklog_front(const fc::path& block_dir, const fc::path& temp_dir, uint32_t truncate_at_block) {
      using namespace std;
      EOS_ASSERT( block_dir != temp_dir, block_log_exception, "block_dir and temp_dir need to be different directories" );
      ilog("In directory ${dir} will trim all blocks before block ${n} from blocks.log and blocks.index.",
           ("dir", block_dir.generic_string())("n", truncate_at_block));
      trim_data original_block_log(block_dir);
      if (truncate_at_block <= original_block_log.first_block) {
         ilog("There are no blocks before block ${n} so do nothing.", ("n", truncate_at_block));
         return false;
      }
      if (truncate_at_block > original_block_log.last_block) {
         ilog("All blocks are before block ${n} so do nothing (trim front would delete entire blocks.log).", ("n", truncate_at_block));
         return false;
      }

      // ****** create the new block log file and write out the header for the file
      fc::create_directories(temp_dir);
      fc::path new_block_filename = temp_dir / "blocks.log";
      if (fc::remove(new_block_filename)) {
         ilog("Removing old blocks.out file");
      }
      fc::cfile new_block_file;
      new_block_file.set_file_path(new_block_filename);
      // need to open as append since the file doesn't already exist, then reopen without append to allow writing the
      // file in any order
      new_block_file.open( LOG_WRITE_C );
      new_block_file.close();
      new_block_file.open( LOG_RW_C );

      static_assert( block_log::max_supported_version == 4,
                     "Code was written to support format of version 4, need to update this code for latest format." );
      uint32_t version = block_log::max_supported_version;
      new_block_file.seek(0);
      new_block_file.write((char*)&version, sizeof(version));
      new_block_file.write((char*)&truncate_at_block, sizeof(truncate_at_block));

      new_block_file << original_block_log.chain_id;

      // append a totem to indicate the division between blocks and header
      auto totem = block_log::npos;
      new_block_file.write((char*)&totem, sizeof(totem));

      const auto new_block_file_first_block_pos = new_block_file.tellp();
      // ****** end of new block log header

      // copy over remainder of block log to new block log
      auto buffer =  make_unique<char[]>(detail::reverse_iterator::_buf_len);
      char* buf =  buffer.get();

      // offset bytes to shift from old blocklog position to new blocklog position
      const uint64_t original_file_block_pos = original_block_log.block_pos(truncate_at_block);
      const uint64_t pos_delta = original_file_block_pos - new_block_file_first_block_pos;
      auto status = fseek(original_block_log.blk_in, 0, SEEK_END);
      EOS_ASSERT( status == 0, block_log_exception, "blocks.log seek failed" );

      // all blocks to copy to the new blocklog
      const uint64_t to_write = ftell(original_block_log.blk_in) - original_file_block_pos;
      const auto pos_size = sizeof(uint64_t);

      // start with the last block's position stored at the end of the block
      uint64_t original_pos = ftell(original_block_log.blk_in) - pos_size;

      const auto num_blocks = original_block_log.last_block - truncate_at_block + 1;

      fc::path new_index_filename = temp_dir / "blocks.index";
      detail::index_writer index(new_index_filename, num_blocks);

      uint64_t read_size = 0;
      for(uint64_t to_write_remaining = to_write; to_write_remaining > 0; to_write_remaining -= read_size) {
         read_size = to_write_remaining;
         if (read_size > detail::reverse_iterator::_buf_len) {
            read_size = detail::reverse_iterator::_buf_len;
         }

         // read in the previous contiguous memory into the read buffer
         const auto start_of_blk_buffer_pos = original_file_block_pos + to_write_remaining - read_size;
         status = fseek(original_block_log.blk_in, start_of_blk_buffer_pos, SEEK_SET);
         const auto num_read = fread(buf, read_size, 1, original_block_log.blk_in);
         EOS_ASSERT( num_read == 1, block_log_exception, "blocks.log read failed" );

         // walk this memory section to adjust block position to match the adjusted location
         // of the block start and store in the new index file
         while(original_pos >= start_of_blk_buffer_pos) {
            const auto buffer_index = original_pos - start_of_blk_buffer_pos;
            uint64_t& pos_content = *(uint64_t*)(buf + buffer_index);
            const auto start_of_this_block = pos_content;
            pos_content = start_of_this_block - pos_delta;
            index.write(pos_content);
            original_pos = start_of_this_block - pos_size;
         }
         new_block_file.seek(new_block_file_first_block_pos + to_write_remaining - read_size);
         new_block_file.write(buf, read_size);
      }
      index.complete();
      fclose(original_block_log.blk_in);
      original_block_log.blk_in = nullptr;
      new_block_file.flush();
      new_block_file.close();

      fc::path old_log = temp_dir / "old.log";
      rename(original_block_log.block_file_name, old_log);
      rename(new_block_filename, original_block_log.block_file_name);
      fc::path old_ind = temp_dir / "old.index";
      rename(original_block_log.index_file_name, old_ind);
      rename(new_index_filename, original_block_log.index_file_name);

      return true;
   }

   trim_data::trim_data(fc::path block_dir) {

      // code should follow logic in block_log::repair_log

      using namespace std;
      block_file_name = block_dir / "blocks.log";
      blk_in = FC_FOPEN(block_file_name.generic_string().c_str(), "rb");
      EOS_ASSERT( blk_in != nullptr, block_log_not_found, "cannot read file ${file}", ("file",block_file_name.string()) );

      auto size = fread((void*)&version,sizeof(version), 1, blk_in);
      EOS_ASSERT( size == 1, block_log_unsupported_version, "invalid format for file ${file}", ("file",block_file_name.string()));
      ilog("block log version= ${version}",("version",version));
      EOS_ASSERT( block_log::is_supported_version(version), block_log_unsupported_version, "block log version ${v} is not supported", ("v",version));

      detail::fileptr_datastream ds(blk_in, block_file_name.string());
      if (version == 1) {
         first_block = 1;
         genesis_state gs;
         fc::raw::unpack(ds, gs);
         chain_id = gs.compute_chain_id();
      }
      else {
         size = fread((void *) &first_block, sizeof(first_block), 1, blk_in);
         EOS_ASSERT(size == 1, block_log_exception, "invalid format for file ${file}",
                    ("file", block_file_name.string()));
         if (block_log::contains_genesis_state(version, first_block)) {
            genesis_state gs;
            fc::raw::unpack(ds, gs);
            chain_id = gs.compute_chain_id();
         }
         else if (block_log::contains_chain_id(version, first_block)) {
            ds >> chain_id;
         }
         else {
            EOS_THROW( block_log_exception,
                       "Block log ${file} is not supported. version: ${ver} and first_block: ${first_block} does not contain "
                       "a genesis_state nor a chain_id.",
                       ("file", block_file_name.string())("ver", version)("first_block", first_block));
         }

         const auto expected_totem = block_log::npos;
         std::decay_t<decltype(block_log::npos)> actual_totem;
         size = fread ( (char*)&actual_totem, sizeof(actual_totem), 1, blk_in);

         EOS_ASSERT(size == 1, block_log_exception,
                    "Expected to read ${size} bytes, but did not read any bytes", ("size", sizeof(actual_totem)));
         EOS_ASSERT(actual_totem == expected_totem, block_log_exception,
                    "Expected separator between block log header and blocks was not found( expected: ${e}, actual: ${a} )",
                    ("e", fc::to_hex((char*)&expected_totem, sizeof(expected_totem) ))("a", fc::to_hex((char*)&actual_totem, sizeof(actual_totem) )));
      }

      const uint64_t start_of_blocks = ftell(blk_in);

      index_file_name = block_dir / "blocks.index";
      ind_in = FC_FOPEN(index_file_name.generic_string().c_str(), "rb");
      EOS_ASSERT( ind_in != nullptr, block_log_not_found, "cannot read file ${file}", ("file",index_file_name.string()) );

      const auto status = fseek(ind_in, 0, SEEK_END);                //get length of blocks.index (gives number of blocks)
      EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} end", ("file", index_file_name.string()) );
      const uint64_t file_end = ftell(ind_in);                //get length of blocks.index (gives number of blocks)
      last_block = first_block + file_end/sizeof(uint64_t) - 1;

      first_block_pos = block_pos(first_block);
      EOS_ASSERT(start_of_blocks == first_block_pos, block_log_exception,
                 "Block log ${file} was determined to have its first block at ${determined}, but the block index "
                 "indicates the first block is at ${index}",
                 ("file", block_file_name.string())("determined", start_of_blocks)("index",first_block_pos));
      ilog("first block= ${first}",("first",first_block));
      ilog("last block= ${last}",("last",last_block));
   }

   trim_data::~trim_data() {
      if (blk_in != nullptr)
         fclose(blk_in);
      if (ind_in != nullptr)
         fclose(ind_in);
   }

   uint64_t trim_data::block_index(uint32_t n) const {
      using namespace std;
      EOS_ASSERT( first_block <= n, block_log_exception,
                  "cannot seek in ${file} to block number ${b}, block number ${first} is the first block",
                  ("file", index_file_name.string())("b",n)("first",first_block) );
      EOS_ASSERT( n <= last_block, block_log_exception,
                  "cannot seek in ${file} to block number ${b}, block number ${last} is the last block",
                  ("file", index_file_name.string())("b",n)("last",last_block) );
      return sizeof(uint64_t) * (n - first_block);
   }

   uint64_t trim_data::block_pos(uint32_t n) {
      using namespace std;
      // can indicate the location of the block after the last block
      if (n == last_block + 1) {
         return ftell(blk_in);
      }
      const uint64_t index_pos = block_index(n);
      auto status = fseek(ind_in, index_pos, SEEK_SET);
      EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file for block ${b}", ("file", index_file_name.string())("pos", index_pos)("b",n) );
      const uint64_t pos = ftell(ind_in);
      EOS_ASSERT( pos == index_pos, block_log_exception, "cannot seek to ${file} entry for block ${b}", ("file", index_file_name.string())("b",n) );
      uint64_t block_n_pos;
      auto size = fread((void*)&block_n_pos, sizeof(block_n_pos), 1, ind_in);                   //filepos of block n
      EOS_ASSERT( size == 1, block_log_exception, "cannot read ${file} entry for block ${b}", ("file", index_file_name.string())("b",n) );

      //read blocks.log and verify block number n is found at the determined file position
      const auto calc_blknum_pos = block_n_pos + detail::block_log_impl::blknum_offset_from_block_entry(version);
      status = fseek(blk_in, calc_blknum_pos, SEEK_SET);
      EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", block_file_name.string())("pos", calc_blknum_pos) );
      const uint64_t block_offset_pos = ftell(blk_in);
      EOS_ASSERT( block_offset_pos == calc_blknum_pos, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", block_file_name.string())("pos", calc_blknum_pos) );
      uint32_t prior_blknum;
      size = fread((void*)&prior_blknum, sizeof(prior_blknum), 1, blk_in);     //read bigendian block number of prior block
      EOS_ASSERT( size == 1, block_log_exception, "cannot read prior block");
      const uint32_t bnum = fc::endian_reverse_u32(prior_blknum) + 1;          //convert to little endian, add 1 since prior block
      EOS_ASSERT( bnum == n, block_log_exception,
                  "At position ${pos} in ${file} expected to find ${exp_bnum} but found ${act_bnum}",
                  ("pos",block_offset_pos)("file", block_file_name.string())("exp_bnum",n)("act_bnum",bnum) );

      return block_n_pos;
   }

   int block_log::trim_blocklog_end(fc::path block_dir, uint32_t n) {       //n is last block to keep (remove later blocks)
      trim_data td(block_dir);

      ilog("In directory ${block_dir} will trim all blocks after block ${n} from ${block_file} and ${index_file}",
         ("block_dir", block_dir.generic_string())("n", n)("block_file",td.block_file_name.generic_string())("index_file", td.index_file_name.generic_string()));

      if (n < td.first_block) {
         dlog("All blocks are after block ${n} so do nothing (trim_end would delete entire blocks.log)",("n", n));
         return 1;
      }
      if (n >= td.last_block) {
         dlog("There are no blocks after block ${n} so do nothing",("n", n));
         return 2;
      }
      const uint64_t end_of_new_file = td.block_pos(n + 1);
      boost::filesystem::resize_file(td.block_file_name, end_of_new_file);
      const uint64_t index_end= td.block_index(n) + sizeof(uint64_t);             //advance past record for block n
      boost::filesystem::resize_file(td.index_file_name, index_end);
      ilog("blocks.index has been trimmed to ${index_end} bytes", ("index_end", index_end));
      return 0;
   }

   void block_log::smoke_test(fc::path block_dir) {
      trim_data td(block_dir);
      auto status = fseek(td.blk_in, -sizeof(uint64_t), SEEK_END);             //get last_block from blocks.log, compare to from blocks.index
      EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", td.block_file_name.string())("pos", sizeof(uint64_t)) );
      uint64_t file_pos;
      auto size = fread((void*)&file_pos, sizeof(uint64_t), 1, td.blk_in);
      EOS_ASSERT( size == 1, block_log_exception, "${file} read fails", ("file", td.block_file_name.string()) );
      int blknum_offset = detail::block_log_impl::blknum_offset_from_block_entry(td.version);
      status            = fseek(td.blk_in, file_pos + blknum_offset, SEEK_SET);
      EOS_ASSERT( status == 0, block_log_exception, "cannot seek to ${file} ${pos} from beginning of file", ("file", td.block_file_name.string())("pos", file_pos + blknum_offset) );
      uint32_t bnum;
      size = fread((void*)&bnum, sizeof(uint32_t), 1, td.blk_in);
      EOS_ASSERT( size == 1, block_log_exception, "${file} read fails", ("file", td.block_file_name.string()) );
      bnum = fc::endian_reverse_u32(bnum) + 1;                       //convert from big endian to little endian and add 1
      EOS_ASSERT( td.last_block == bnum, block_log_exception, "blocks.log says last block is ${lb} which disagrees with blocks.index", ("lb", bnum) );
      ilog("blocks.log and blocks.index agree on number of blocks");
      uint32_t delta = (td.last_block + 8 - td.first_block) >> 3;
      if (delta < 1)
         delta = 1;
      for (uint32_t n = td.first_block; ; n += delta) {
         if (n > td.last_block)
            n = td.last_block;
         td.block_pos(n);                                 //check block 'n' is where blocks.index says
         if (n == td.last_block)
            break;
      }
   }
}} /// eosio::chain
