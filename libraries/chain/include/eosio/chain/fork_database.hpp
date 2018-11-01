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

         fork_database( const fc::path& data_dir );
         ~fork_database();

         void close();

         block_state_ptr  get_block(const block_id_type& id)const;
         block_state_ptr  get_block_in_current_chain_by_num( uint32_t n )const;
//         vector<block_state_ptr>    get_blocks_by_number(uint32_t n)const;

         /**
          *  Provides a "valid" blockstate upon which other forks may build.
          */
         void            set( block_state_ptr s );

         /** this method will attempt to append the block to an existing
          * block_state and will return a pointer to the new block state or
          * throw on error.
          */
         block_state_ptr add( signed_block_ptr b, bool skip_validate_signee );
         block_state_ptr add( const block_state_ptr& next_block, bool skip_validate_previous );
         void            remove( const block_id_type& id );

         void            add( const header_confirmation& c );

         const block_state_ptr& head()const;

         /**
          *  Given two head blocks, return two branches of the fork graph that
          *  end with a common ancestor (same prior block)
          */
         pair< branch_type, branch_type >  fetch_branch_from( const block_id_type& first,
                                                              const block_id_type& second )const;


         /**
          * If the block is invalid, it will be removed. If it is valid, then blocks older
          * than the LIB are pruned after emitting irreversible signal.
          */
         void set_validity( const block_state_ptr& h, bool valid );
         void mark_in_current_chain( const block_state_ptr& h, bool in_current_chain );
         void prune( const block_state_ptr& h );

         /**
          * This signal is emited when a block state becomes irreversible, once irreversible
          * it is removed unless it is the head block.
          */
         signal<void(block_state_ptr)> irreversible;

      private:
         void set_bft_irreversible( block_id_type id );
         unique_ptr<fork_database_impl> my;
   };

} } /// eosio::chain
