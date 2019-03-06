#pragma once
#include <eosio/chain/block_state.hpp>
#include <boost/signals2/signal.hpp>

namespace eosio { namespace chain {

   using boost::signals2::signal;

   struct fork_database_impl;

   typedef vector<block_state_ptr> branch_type;

   /**
    * @class fork_database
    * @brief manages light-weight state for all potential unconfirmed forks
    *
    * As new blocks are received, they are pushed into the fork database. The fork
    * database tracks the longest chain and the last irreversible block number. All
    * blocks older than the last irreversible block are freed after emitting the
    * irreversible signal.
    */
   class fork_database {
      public:

         explicit fork_database( const fc::path& data_dir );
         ~fork_database();

         void open( const std::function<void( block_timestamp_type,
                                              const flat_set<digest_type>&,
                                              const vector<digest_type>& )>& validator );
         void close();

         block_header_state_ptr  get_block_header( const block_id_type& id )const;
         block_state_ptr         get_block( const block_id_type& id )const;

         /**
          *  Purges any existing blocks from the fork database and resets the root block_header_state to the provided value.
          *  The head will also be reset to point to the root.
          */
         void            reset( const block_header_state& root_bhs );

         /**
          *  Removes validated flag from all blocks in fork database and resets head to point to the root.
          */
         void            rollback_head_to_root();

         /**
          *  Advance root block forward to some other block in the tree.
          */
         void            advance_root( const block_id_type& id );

         /**
          *  Add block state to fork database.
          *  Must link to existing block in fork database or the root.
          */
         void            add( const block_state_ptr& next_block, bool ignore_duplicate = false );

         void            remove( const block_id_type& id );

         const block_state_ptr& root()const;
         const block_state_ptr& head()const;
         block_state_ptr        pending_head()const;

         /**
          *  Returns the sequence of block states resulting from trimming the branch from the
          *  root block (exclusive) to the block with an id of `h` (inclusive) by removing any
          *  block states corresponding to block numbers greater than `trim_after_block_num`.
          *
          *  The order of the sequence is in descending block number order.
          *  A block with an id of `h` must exist in the fork database otherwise this method will throw an exception.
          */
         branch_type     fetch_branch( const block_id_type& h, uint32_t trim_after_block_num = std::numeric_limits<uint32_t>::max() )const;


         /**
          *  Returns the block state with a block number of `block_num` that is on the branch that
          *  contains a block with an id of`h`, or the empty shared pointer if no such block can be found.
          */
         block_state_ptr search_on_branch( const block_id_type& h, uint32_t block_num )const;

         /**
          *  Given two head blocks, return two branches of the fork graph that
          *  end with a common ancestor (same prior block)
          */
         pair< branch_type, branch_type >  fetch_branch_from( const block_id_type& first,
                                                              const block_id_type& second )const;


         void mark_valid( const block_state_ptr& h );

         static const uint32_t magic_number;

         static const uint32_t min_supported_version;
         static const uint32_t max_supported_version;

      private:
         unique_ptr<fork_database_impl> my;
   };

} } /// eosio::chain
