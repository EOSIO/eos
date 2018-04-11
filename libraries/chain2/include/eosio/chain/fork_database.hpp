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

         fork_database();
         ~fork_database();

         block_state_ptr  get_block(const block_id_type& id)const;
//         vector<block_state_ptr>    get_blocks_by_number(uint32_t n)const;

         /**
          *  Provides a "valid" blockstate upon which other forks may build.
          */
         void            set( block_state_ptr s );

         /** this method will attempt to append the block to an exsting
          * block_state and will return a pointer to the head block or
          * throw on error.
          */
         block_state_ptr add( signed_block_ptr b ); 

         const block_state_ptr& head()const;

         /**
          *  Given two head blocks, return two branches of the fork graph that
          *  end with a common ancestor (same prior block)
          */
         pair< branch_type, branch_type >  fetch_branch_from( block_id_type first,
                                                              block_id_type second )const;


         /**
          * If the block is invalid, it will be removed. If it is valid, then blocks older
          * than the LIB are pruned after emitting irreversible signal.
          */
         void set_validity( const block_state_ptr& h, bool valid );

         /**
          * This signal is emited when a block state becomes irreversible, once irreversible
          * it is removed;
          */
         signal<void(block_state_ptr)> irreverisble;

      private:
         unique_ptr<fork_database_impl> my;
   };

} } /// eosio::chain
