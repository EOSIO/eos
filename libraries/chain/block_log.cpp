/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fstream>
#include <fc/io/raw.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/shared_mutex.hpp>

#define LOG_READ  (std::ios::in | std::ios::binary)
#define LOG_WRITE (std::ios::out | std::ios::binary | std::ios::app)

namespace eosio { namespace chain {

   /**
    * History:
    * Version 1: complete block log from genesis
    * Version 2: adds optional partial block log, cannot be used for replay without snapshot
    *            this is in the form of an first_block_num that is written immediately after the version
    */

   namespace detail {

      using read_write_mutex = boost::shared_mutex;
      using read_lock = boost::shared_lock<read_write_mutex>;
      using write_lock = boost::unique_lock<read_write_mutex>;
      using boost::iostreams::mapped_file;
      static constexpr boost::iostreams::stream_offset min_valid_file_size = sizeof(uint32_t);

      class block_log_impl {
         public:
            signed_block_ptr         head;
            block_id_type            head_id;

            std::string              block_path;
            std::string              index_path;
            mapped_file              block_mapped_file;
            mapped_file              index_mapped_file;
            read_write_mutex         mutex;

            uint32_t                 version = 0;
            const uint32_t           min_supported_version = 1;
            const uint32_t           max_supported_version = 1;


            // TODO: removed by CyberWay
            //bool                     genesis_written_to_block_log = false;
            //uint32_t                 first_block_num = 0;

            bool has_block_records() const {
                auto size = block_mapped_file.size();
                return (size > min_valid_file_size + sizeof(version));
            }

            bool has_index_records() const {
                auto size = index_mapped_file.size();
                return (size >= min_valid_file_size);
            }

            std::size_t get_mapped_size(const boost::iostreams::mapped_file& mapped_file) const {
                auto size = mapped_file.size();
                if (size < min_valid_file_size) {
                    return 0;
                }
                return size;
            }

            uint64_t get_uint64(const boost::iostreams::mapped_file& mapped_file, std::size_t pos) const {
                uint64_t value;
                auto file_size = get_mapped_size(mapped_file);
                EOS_ASSERT(pos + sizeof(value) <= file_size,
                        block_log_exception,
                        "Reading data beyond end of file",
                        ("pos", pos)("size", sizeof(value))("file_size", file_size));

                auto* ptr = mapped_file.data() + pos;
                value = *reinterpret_cast<uint64_t*>(ptr);
                return value;
            }

            uint32_t get_uint32(const boost::iostreams::mapped_file& mapped_file, std::size_t pos) const {
                uint32_t value;
                auto file_size = get_mapped_size(mapped_file);
                EOS_ASSERT(pos + sizeof(value) <= file_size,
                        block_log_exception,
                        "Reading data beyond end of file",
                        ("pos", pos)("size", sizeof(value))("file_size", file_size));

                auto* ptr = mapped_file.data() + pos;
                value = *reinterpret_cast<uint32_t*>(ptr);
                return value;
            }

            uint64_t get_last_uint64(const boost::iostreams::mapped_file& mapped_file) const {
                uint64_t value;
                auto file_size = get_mapped_size(mapped_file);
                EOS_ASSERT(sizeof(value) <= file_size,
                        block_log_exception,
                        "Reading data beyond end of file",
                        ("size", sizeof(value))("file_size", file_size));

                auto* ptr = mapped_file.data() + file_size - sizeof(value);
                value = *reinterpret_cast<uint64_t*>(ptr);
                return value;
            }

            uint64_t get_block_pos(uint32_t block_num) const {
                if (head.get() != nullptr &&
                    block_num <= block_header::num_from_id(head_id) &&
                    block_num > 0
                ) {
                    return get_uint64(index_mapped_file, sizeof(uint64_t) * (block_num - 1));
                }
                return block_log::npos;
            }

            uint64_t read_block(uint64_t pos, signed_block& block) const {
                const auto file_size = get_mapped_size(block_mapped_file);
                EOS_ASSERT(pos < file_size,
                        block_log_exception,
                        "Reading data beyond end of file",
                        ("pos", pos)("file_size", file_size));

                const auto* ptr = block_mapped_file.data() + pos;
                const auto available_size = file_size - pos;
                const auto max_block_size = std::min<std::size_t>(available_size, eosio::chain::config::maximum_block_size);

                fc::datastream<const char*> ds(ptr, max_block_size);
                fc::raw::unpack(ds, block);

                const auto end_pos = pos + ds.tellp();
                const auto block_pos = get_uint64(block_mapped_file, end_pos);
                EOS_ASSERT(block_pos == pos,
                        block_log_exception,
                        "Wrong position markers was read (read ${block_pos}, expected ${expected})",
                        ("block_pos", block_pos)("expected", pos));

                return end_pos + sizeof(uint64_t);
            }

            signed_block_ptr read_head() const {
                if (get_mapped_size(block_mapped_file) == 0) {
                    return {};
                }
                auto pos = get_last_uint64(block_mapped_file);
                signed_block_ptr block = std::make_shared<signed_block>();
                read_block(pos, *block);
                return block;
            }

            void create_nonexist_file(const std::string& path) const {
                if (!boost::filesystem::is_regular_file(path) || boost::filesystem::file_size(path) == 0) {
                    std::ofstream stream(path, std::ios::out|std::ios::binary);
                    stream << '\0';
                    stream.close();
                }
            }

            void open_block_mapped_file() {
                create_nonexist_file(block_path);
                block_mapped_file.open(block_path, boost::iostreams::mapped_file::readwrite);
            }

            void open_index_mapped_file() {
                create_nonexist_file(index_path);
                index_mapped_file.open(index_path, boost::iostreams::mapped_file::readwrite);
            }

            void construct_index() {
                ilog("Reconstructing Block Log Index...");
                index_mapped_file.close();
                boost::filesystem::remove_all(index_path);
                open_index_mapped_file();
                index_mapped_file.resize(head->block_num() * sizeof(uint64_t));

                uint64_t pos = sizeof(uint32_t); // version
                uint64_t end_pos = get_last_uint64(block_mapped_file);
                auto* idx_ptr = index_mapped_file.data();
                signed_block tmp_block;

                while (pos <= end_pos) {
                    *reinterpret_cast<uint64_t*>(idx_ptr) = pos;
                    pos = read_block(pos, tmp_block);
                    ilog("Read block: ${block_id} ${previous}", ("block_id",tmp_block.id())("previous",tmp_block.previous));
                    idx_ptr += sizeof(pos);
                }
            }

            void open(const fc::path& data_dir) { try {
                block_mapped_file.close();
                index_mapped_file.close();

                block_path = (data_dir / "blocks.log").string();
                index_path = (data_dir / "blocks.index").string();
 
                open_block_mapped_file();
                open_index_mapped_file();

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

                if (has_block_records()) {
                    ilog("Log is nonempty");

                    version = get_uint32(block_mapped_file, 0);
                    EOS_ASSERT(version >= min_supported_version && version <= max_supported_version, block_log_unsupported_version,
                            "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
                            ("version", version)("min", min_supported_version)("max", max_supported_version) );

                    head = read_head();
                    head_id = head->id();

                    if (has_index_records()) {
                        ilog("Index is nonempty");

                        auto block_pos = get_last_uint64(block_mapped_file);
                        auto index_pos = get_last_uint64(index_mapped_file);

                        if (block_pos != index_pos) {
                            ilog("block_pos != index_pos, close and reopen index_stream");
                            construct_index();
                        }
                    } else {
                        ilog("Index is empty");
                        construct_index();
                    }
                } else if (has_index_records()) {
                    ilog("Index is nonempty, remove and recreate it");
                    index_mapped_file.close();
                    block_mapped_file.close();

                    boost::filesystem::remove_all(block_path);
                    boost::filesystem::remove_all(index_path);

                    open_block_mapped_file();
                    open_index_mapped_file();
                }
            } FC_LOG_AND_RETHROW() }

            void reset( const signed_block_ptr& first_block, const std::vector<char>& first_block_data ) try {
                block_mapped_file.close();
                index_mapped_file.close();

                fc::remove_all(block_path);
                fc::remove_all(index_path);

                open_block_mapped_file();
                open_index_mapped_file();

                block_mapped_file.resize(sizeof(uint32_t));
                auto* ptr = block_mapped_file.data();
                *reinterpret_cast<uint32_t*>(ptr) = max_supported_version;

                if (first_block) {
                    append(first_block, first_block_data);
                }
            } FC_LOG_AND_RETHROW()

            void close() {
                block_mapped_file.close();
                index_mapped_file.close();
                head.reset();
                head_id = block_id_type();
            }

            uint64_t append(const signed_block_ptr& block, const std::vector<char>& data) { try {
                const auto index_pos = get_mapped_size(index_mapped_file);

                EOS_ASSERT(data.size() <= eosio::chain::config::maximum_block_size,
                    block_log_exception,
                    "Block size to large (current ${size}, maximum ${max})",
                    ("size", data.size())("max", eosio::chain::config::maximum_block_size));

                EOS_ASSERT(index_pos == sizeof(uint64_t) * (block->block_num() - 1),
                    block_log_exception,
                    "Append to index file occuring at wrong position.",
                    ("position", index_pos)
                    ("expected", (block->block_num() - 1) * sizeof(uint64_t)));

                uint64_t block_pos = get_mapped_size(block_mapped_file);

                block_mapped_file.resize(block_pos + data.size() + sizeof(block_pos));
                auto* ptr = block_mapped_file.data() + block_pos;
                std::memcpy(ptr, data.data(), data.size());
                ptr += data.size();
                *reinterpret_cast<uint64_t*>(ptr) = block_pos;

                index_mapped_file.resize(index_pos + sizeof(index_pos));
                ptr = index_mapped_file.data() + index_pos;
                *reinterpret_cast<uint64_t*>(ptr) = block_pos;

                head = block;
                head_id = block->id();
                return block_pos;
            } FC_LOG_AND_RETHROW() }
      };
   }

   block_log::block_log(const fc::path& data_dir)
   : my(std::make_unique<detail::block_log_impl>()) {
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
        detail::write_lock lock(my->mutex);
        my->open(data_dir);
   }

   uint64_t block_log::append(const signed_block_ptr& block) try {
        auto data = fc::raw::pack(*block);
        detail::write_lock lock(my->mutex);
        return my->append(block, data);
   } FC_LOG_AND_RETHROW()

   void block_log::flush() {
       // it isn't needed, because all data is already in page cache
   }

   void block_log::reset( const genesis_state& gs, const signed_block_ptr& first_block, uint32_t first_block_num ) {
       EOS_ASSERT(first_block_num == 1, block_log_exception,
            "Unsupported first_block_num (can be only 1)");

       std::vector<char> data;
       if (first_block) {
           data = fc::raw::pack(*first_block);
       }

       detail::write_lock lock(my->mutex);
       my->reset(first_block, data);

// TODO: removed by CyberWay
//      auto data = fc::raw::pack(gs);
//      my->version = 0; // version of 0 is invalid; it indicates that the genesis was not properly written to the block log
//      my->first_block_num = first_block_num;
//      my->block_stream.write((char*)&my->version, sizeof(my->version));
//      my->block_stream.write((char*)&my->first_block_num, sizeof(my->first_block_num));
//      my->block_stream.write(data.data(), data.size());
//      my->genesis_written_to_block_log = true;
//
//      // append a totem to indicate the division between blocks and header
//      auto totem = npos;
//      my->block_stream.write((char*)&totem, sizeof(totem));
//
//      if (first_block) {
//         append(first_block);
//      }
//
//      auto pos = my->block_stream.tellp();
//
//      my->block_stream.close();
//      my->block_stream.open(my->block_file.generic_string().c_str(), std::ios::in | std::ios::out | std::ios::binary ); // Bypass append-only writing just once
//
//      static_assert( block_log::max_supported_version > 0, "a version number of zero is not supported" );
//      my->version = block_log::max_supported_version;
//      my->block_stream.seekp( 0 );
//      my->block_stream.write( (char*)&my->version, sizeof(my->version) );
//      my->block_stream.seekp( pos );
//      flush();
//
//      my->block_write = false;
//      my->check_block_write(); // Reset to append-only writing.
   }

   std::pair<signed_block_ptr, uint64_t> block_log::read_block(uint64_t pos) const {
        detail::read_lock lock(my->mutex);
        std::pair<signed_block_ptr, uint64_t> result;
        result.first = std::make_shared<signed_block>();
        result.second = my->read_block(pos, *result.first);
        return result;
    }

   signed_block_ptr block_log::read_block_by_num(uint32_t block_num) const try {
       detail::read_lock lock(my->mutex);
       signed_block_ptr block;
       uint64_t pos = my->get_block_pos(block_num);
       if (pos != npos) {
           block = std::make_shared<signed_block>();
           my->read_block(pos, *block);
           EOS_ASSERT(block->block_num() == block_num,
                   block_log_exception,
                   "Wrong block was read from block log (read ${block_num}, expected ${expected}).",
                   ("block_num", block->block_num())("expected", block_num));
       }
       return block;
   } FC_LOG_AND_RETHROW()

   uint64_t block_log::get_block_pos(uint32_t block_num) const {
        detail::read_lock lock(my->mutex);
        return my->get_block_pos(block_num);
   }

   signed_block_ptr block_log::read_head()const {
       detail::read_lock lock(my->mutex);
       return my->read_head();
   }

   const signed_block_ptr& block_log::head()const {
      detail::read_lock lock(my->mutex);
      return my->head;
   }

// TODO: removed by CyberWay
//   uint32_t block_log::first_block_num() const {
//      return my->first_block_num;
//   }
//
//   void block_log::construct_index() {
//      ilog("Reconstructing Block Log Index...");
//      my->index_stream.close();
//      fc::remove_all(my->index_file);
//      my->index_stream.open(my->index_file.generic_string().c_str(), LOG_WRITE);
//      my->index_write = true;
//
//      uint64_t end_pos;
//      my->check_block_read();
//
//      my->block_stream.seekg(-sizeof( uint64_t), std::ios::end);
//      my->block_stream.read((char*)&end_pos, sizeof(end_pos));
//      signed_block tmp;
//
//      uint64_t pos = 0;
//      if (my->version == 1) {
//         pos = 4; // Skip version which should have already been checked.
//      } else {
//         pos = 8; // Skip version and first block offset which should have already been checked
//      }
//      my->block_stream.seekg(pos);
//
//      genesis_state gs;
//      fc::raw::unpack(my->block_stream, gs);
//
//      // skip the totem
//      if (my->version > 1) {
//         uint64_t totem;
//         my->block_stream.read((char*) &totem, sizeof(totem));
//      }
//
//      while( pos < end_pos ) {
//         fc::raw::unpack(my->block_stream, tmp);
//         my->block_stream.read((char*)&pos, sizeof(pos));
//         my->index_stream.write((char*)&pos, sizeof(pos));
//      }
//   } // construct_index
//
//   fc::path block_log::repair_log( const fc::path& data_dir, uint32_t truncate_at_block ) {
//      ilog("Recovering Block Log...");
//      EOS_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"), block_log_not_found,
//                 "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );
//
//      auto now = fc::time_point::now();
//
//      auto blocks_dir = fc::canonical( data_dir );
//      if( blocks_dir.filename().generic_string() == "." ) {
//         blocks_dir = blocks_dir.parent_path();
//      }
//      auto backup_dir = blocks_dir.parent_path();
//      auto blocks_dir_name = blocks_dir.filename();
//      EOS_ASSERT( blocks_dir_name.generic_string() != ".", block_log_exception, "Invalid path to blocks directory" );
//      backup_dir = backup_dir / blocks_dir_name.generic_string().append("-").append( now );
//
//      EOS_ASSERT( !fc::exists(backup_dir), block_log_backup_dir_exist,
//                 "Cannot move existing blocks directory to already existing directory '${new_blocks_dir}'",
//                 ("new_blocks_dir", backup_dir) );
//
//      fc::rename( blocks_dir, backup_dir );
//      ilog( "Moved existing blocks directory to backup location: '${new_blocks_dir}'", ("new_blocks_dir", backup_dir) );
//
//      fc::create_directories(blocks_dir);
//      auto block_log_path = blocks_dir / "blocks.log";
//
//      ilog( "Reconstructing '${new_block_log}' from backed up block log", ("new_block_log", block_log_path) );
//
//      std::fstream  old_block_stream;
//      std::fstream  new_block_stream;
//
//      old_block_stream.open( (backup_dir / "blocks.log").generic_string().c_str(), LOG_READ );
//      new_block_stream.open( block_log_path.generic_string().c_str(), LOG_WRITE );
//
//      old_block_stream.seekg( 0, std::ios::end );
//      uint64_t end_pos = old_block_stream.tellg();
//      old_block_stream.seekg( 0 );
//
//      uint32_t version = 0;
//      old_block_stream.read( (char*)&version, sizeof(version) );
//      EOS_ASSERT( version > 0, block_log_exception, "Block log was not setup properly" );
//      EOS_ASSERT( version >= min_supported_version && version <= max_supported_version, block_log_unsupported_version,
//                 "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
//                 ("version", version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );
//
//      new_block_stream.write( (char*)&version, sizeof(version) );
//
//      uint32_t first_block_num = 1;
//      if (version != 1) {
//         old_block_stream.read ( (char*)&first_block_num, sizeof(first_block_num) );
//         new_block_stream.write( (char*)&first_block_num, sizeof(first_block_num) );
//      }
//
//      genesis_state gs;
//      fc::raw::unpack(old_block_stream, gs);
//
//      auto data = fc::raw::pack( gs );
//      new_block_stream.write( data.data(), data.size() );
//
//      if (version != 1) {
//         auto expected_totem = npos;
//         std::decay_t<decltype(npos)> actual_totem;
//         old_block_stream.read ( (char*)&actual_totem, sizeof(actual_totem) );
//
//         EOS_ASSERT(actual_totem == expected_totem, block_log_exception,
//                    "Expected separator between block log header and blocks was not found( expected: ${e}, actual: ${a} )",
//                    ("e", fc::to_hex((char*)&expected_totem, sizeof(expected_totem) ))("a", fc::to_hex((char*)&actual_totem, sizeof(actual_totem) )));
//
//         new_block_stream.write( (char*)&actual_totem, sizeof(actual_totem) );
//      }
//
//      std::exception_ptr     except_ptr;
//      vector<char>           incomplete_block_data;
//      optional<signed_block> bad_block;
//      uint32_t               block_num = 0;
//
//      block_id_type previous;
//
//      uint64_t pos = old_block_stream.tellg();
//      while( pos < end_pos ) {
//         signed_block tmp;
//
//         try {
//            fc::raw::unpack(old_block_stream, tmp);
//         } catch( ... ) {
//            except_ptr = std::current_exception();
//            incomplete_block_data.resize( end_pos - pos );
//            old_block_stream.read( incomplete_block_data.data(), incomplete_block_data.size() );
//            break;
//         }
//
//         auto id = tmp.id();
//         if( block_header::num_from_id(previous) + 1 != block_header::num_from_id(id) ) {
//            elog( "Block ${num} (${id}) skips blocks. Previous block in block log is block ${prev_num} (${previous})",
//                  ("num", block_header::num_from_id(id))("id", id)
//                  ("prev_num", block_header::num_from_id(previous))("previous", previous) );
//         }
//         if( previous != tmp.previous ) {
//            elog( "Block ${num} (${id}) does not link back to previous block. "
//                  "Expected previous: ${expected}. Actual previous: ${actual}.",
//                  ("num", block_header::num_from_id(id))("id", id)("expected", previous)("actual", tmp.previous) );
//         }
//         previous = id;
//
//         uint64_t tmp_pos = std::numeric_limits<uint64_t>::max();
//         if( (static_cast<uint64_t>(old_block_stream.tellg()) + sizeof(pos)) <= end_pos ) {
//            old_block_stream.read( reinterpret_cast<char*>(&tmp_pos), sizeof(tmp_pos) );
//         }
//         if( pos != tmp_pos ) {
//            bad_block.emplace(std::move(tmp));
//            break;
//         }
//
//         auto data = fc::raw::pack(tmp);
//         new_block_stream.write( data.data(), data.size() );
//         new_block_stream.write( reinterpret_cast<char*>(&pos), sizeof(pos) );
//         block_num = tmp.block_num();
//         pos = new_block_stream.tellp();
//         if( block_num == truncate_at_block )
//            break;
//      }
//
//      if( bad_block.valid() ) {
//         ilog( "Recovered only up to block number ${num}. Last block in block log was not properly committed:\n${last_block}",
//               ("num", block_num)("last_block", *bad_block) );
//      } else if( except_ptr ) {
//         std::string error_msg;
//
//         try {
//            std::rethrow_exception(except_ptr);
//         } catch( const fc::exception& e ) {
//            error_msg = e.what();
//         } catch( const std::exception& e ) {
//            error_msg = e.what();
//         } catch( ... ) {
//            error_msg = "unrecognized exception";
//         }
//
//         ilog( "Recovered only up to block number ${num}. "
//               "The block ${next_num} could not be deserialized from the block log due to error:\n${error_msg}",
//               ("num", block_num)("next_num", block_num+1)("error_msg", error_msg) );
//
//         auto tail_path = blocks_dir / std::string("blocks-bad-tail-").append( now ).append(".log");
//         if( !fc::exists(tail_path) && incomplete_block_data.size() > 0 ) {
//            std::fstream tail_stream;
//            tail_stream.open( tail_path.generic_string().c_str(), LOG_WRITE );
//            tail_stream.write( incomplete_block_data.data(), incomplete_block_data.size() );
//
//            ilog( "Data at tail end of block log which should contain the (incomplete) serialization of block ${num} "
//                  "has been written out to '${tail_path}'.",
//                  ("num", block_num+1)("tail_path", tail_path) );
//         }
//      } else if( block_num == truncate_at_block && pos < end_pos ) {
//         ilog( "Stopped recovery of block log early at specified block number: ${stop}.", ("stop", truncate_at_block) );
//      } else {
//         ilog( "Existing block log was undamaged. Recovered all irreversible blocks up to block number ${num}.", ("num", block_num) );
//      }
//
//      return backup_dir;
//   }

//   genesis_state block_log::extract_genesis_state( const fc::path& data_dir ) {
//      EOS_ASSERT( fc::is_directory(data_dir) && fc::is_regular_file(data_dir / "blocks.log"), block_log_not_found,
//                 "Block log not found in '${blocks_dir}'", ("blocks_dir", data_dir)          );
//
//      std::fstream  block_stream;
//      block_stream.open( (data_dir / "blocks.log").generic_string().c_str(), LOG_READ );
//
//      uint32_t version = 0;
//      block_stream.read( (char*)&version, sizeof(version) );
//      EOS_ASSERT( version > 0, block_log_exception, "Block log was not setup properly." );
//      EOS_ASSERT( version >= min_supported_version && version <= max_supported_version, block_log_unsupported_version,
//                 "Unsupported version of block log. Block log version is ${version} while code supports version(s) [${min},${max}]",
//                 ("version", version)("min", block_log::min_supported_version)("max", block_log::max_supported_version) );
//
//      uint32_t first_block_num = 1;
//      if (version != 1) {
//         block_stream.read ( (char*)&first_block_num, sizeof(first_block_num) );
//      }
//
//      genesis_state gs;
//      fc::raw::unpack(block_stream, gs);
//      return gs;
//   }

} } /// eosio::chain
