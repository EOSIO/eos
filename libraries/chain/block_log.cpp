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

   namespace detail {
      class block_log_impl {
         public:
            optional<signed_block>   head;
            block_id_type            head_id;
            std::fstream             block_stream;
            std::fstream             index_stream;
            fc::path                 block_file;
            fc::path                 index_file;
            bool                     block_write;
            bool                     index_write;

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

      ilog("Opening block log at ${path}", ("path", my->block_file.generic_string()));
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
         my->head = read_head();
         my->head_id = my->head->id();

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

   uint64_t block_log::append(const signed_block& b) {
      try {
         my->check_block_write();
         my->check_index_write();

         uint64_t pos = my->block_stream.tellp();
         FC_ASSERT(my->index_stream.tellp() == sizeof(uint64_t) * (b.block_num() - 1),
                   "Append to index file occuring at wrong position.",
                   ("position", (uint64_t) my->index_stream.tellp())
                   ("expected", (b.block_num() - 1) * sizeof(uint64_t)));
         auto data = fc::raw::pack(b);
         my->block_stream.write(data.data(), data.size());
         my->block_stream.write((char*)&pos, sizeof(pos));
         my->index_stream.write((char*)&pos, sizeof(pos));
         my->head = b;
         my->head_id = b.id();

         return pos;
      }
      FC_LOG_AND_RETHROW()
   }

   void block_log::flush() {
      my->block_stream.flush();
      my->index_stream.flush();
   }

   std::pair<signed_block, uint64_t> block_log::read_block(uint64_t pos)const {
      my->check_block_read();

      my->block_stream.seekg(pos);
      std::pair<signed_block,uint64_t> result;
      fc::raw::unpack(my->block_stream, result.first);
      result.second = uint64_t(my->block_stream.tellg()) + 8;
      return result;
   }

   optional<signed_block> block_log::read_block_by_num(uint32_t block_num)const {
      try {
         optional<signed_block> b;
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

      if (!(my->head.valid() && block_num <= block_header::num_from_id(my->head_id) && block_num > 0))
         return npos;
      my->index_stream.seekg(sizeof(uint64_t) * (block_num - 1));
      uint64_t pos;
      my->index_stream.read((char*)&pos, sizeof(pos));
      return pos;
   }

   optional<signed_block> block_log::read_head()const {
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

   const optional<signed_block>& block_log::head()const {
      return my->head;
   }

   void block_log::construct_index() {
      ilog("Reconstructing Block Log Index...");
      my->index_stream.close();
      fc::remove_all(my->index_file);
      my->index_stream.open(my->index_file.generic_string().c_str(), LOG_WRITE);
      my->index_write = true;

      uint64_t pos = 0;
      uint64_t end_pos;
      my->check_block_read();

      my->block_stream.seekg(-sizeof( uint64_t), std::ios::end);
      my->block_stream.read((char*)&end_pos, sizeof(end_pos));
      signed_block tmp;

      my->block_stream.seekg(pos);

      while (pos < end_pos) {
         fc::raw::unpack(my->block_stream, tmp);
         my->block_stream.read((char*)&pos, sizeof(pos));
         my->index_stream.write((char*)&pos, sizeof(pos));
      }
   }
} }
