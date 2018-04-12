/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/filesystem.hpp>
#include <eosio/chain/block.hpp>

namespace eosio { namespace chain {

   namespace detail { class block_log_impl; }

   /* The block log is an external append only log of the blocks. Blocks should only be written
    * to the log after they irreverisble as the log is append only. The log is a doubly linked
    * list of blocks. There is a secondary index file of only block positions that enables O(1)
    * random access lookup by block number.
    *
    * +---------+----------------+---------+----------------+-----+------------+-------------------+
    * | Block 1 | Pos of Block 1 | Block 2 | Pos of Block 2 | ... | Head Block | Pos of Head Block |
    * +---------+----------------+---------+----------------+-----+------------+-------------------+
    *
    * +----------------+----------------+-----+-------------------+
    * | Pos of Block 1 | Pos of Block 2 | ... | Pos of Head Block |
    * +----------------+----------------+-----+-------------------+
    *
    * The block log can be walked in order by deserializing a block, skipping 8 bytes, deserializing a
    * block, repeat... The head block of the file can be found by seeking to the position contained
    * in the last 8 bytes the file. The block log can be read backwards by jumping back 8 bytes, following
    * the position, reading the block, jumping back 8 bytes, etc.
    *
    * Blocks can be accessed at random via block number through the index file. Seek to 8 * (block_num - 1)
    * to find the position of the block in the main file.
    *
    * The main file is the only file that needs to persist. The index file can be reconstructed during a
    * linear scan of the main file.
    */

   class block_log {
      public:
         block_log(const fc::path& data_dir);
         block_log(block_log&& other);
         ~block_log();

         uint64_t append(const signed_block& b);
         void flush();
         std::pair<signed_block, uint64_t> read_block(uint64_t file_pos)const;
         optional<signed_block> read_block_by_num(uint32_t block_num)const;
         optional<signed_block> read_block_by_id(const block_id_type& id)const {
            return read_block_by_num(block_header::num_from_id(id));
         }

         /**
          * Return offset of block in file, or block_log::npos if it does not exist.
          */
         uint64_t get_block_pos(uint32_t block_num) const;
         optional<signed_block> read_head()const;
         const optional<signed_block>& head()const;

         static const uint64_t npos = std::numeric_limits<uint64_t>::max();

      private:
         void open(const fc::path& data_dir);
         void construct_index();

         std::unique_ptr<detail::block_log_impl> my;
   };

} }
