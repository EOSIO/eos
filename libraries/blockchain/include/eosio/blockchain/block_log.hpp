#pragma once
#include <fc/filesystem.hpp>
#include <eosio/blockchain/block.hpp>

namespace eosio { namespace blockchain {

   namespace detail { class block_log_impl; }

   typedef uint64_t log_pos_type;

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

         /**
          * @param packed is a serialized packed_block 
          * @return the position in the file at the start of the block
          */
         log_pos_type append(const vector<char>& packed );
         log_pos_type append(const packed_block& b);

         void flush();
         std::pair<packed_block, log_pos_type> read_block(log_pos_type file_pos)const;
         optional<packed_block> read_block_by_num(block_num_type block_num)const;
         optional<packed_block> read_block_by_id(const block_id_type& id)const {
            return read_block_by_num(block_header::num_from_id(id));
         }

         /**
          * Return offset of block in file, or block_log::npos if it does not exist.
          */
         log_pos_type get_block_pos(block_num_type block_num) const;
         optional<packed_block>                 read_head()const;
         const optional<signed_block_header>&   head()const;

         static const log_pos_type npos = std::numeric_limits<log_pos_type>::max();

      private:
         void open(const fc::path& data_dir);
         void construct_index();

         std::unique_ptr<detail::block_log_impl> my;
   };

} } /// eosio::blockchain
