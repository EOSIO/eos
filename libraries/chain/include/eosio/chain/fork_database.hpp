/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/block.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>


namespace eosio { namespace chain {
   using boost::multi_index_container;
   using namespace boost::multi_index;

   struct header_state
   {
      header_state( signed_block d )
      :num(d.block_num()),id(d.id()),data( std::make_shared<signed_block>(std::move(d)) ){}

      header_state( const header_state& cpy ) = default;

      block_id_type previous_id()const { return data->previous; }
      bool is_irreversible()const {
         return num < last_irreversible_block;
      }

      weak_ptr< header_state >                prev;
      shared_ptr< producer_schedule_type >    active_schedule;
      shared_ptr< producer_schedule_type >    pending_schedule;
      uint32_t                                pending_schedule_block;
      uint32_t                                last_irreversible_block = 0;
      flat_map<account_name,uint32_t>         last_block_per_producer;
      digest_type                             active_schedule_digest;


      uint32_t                                num;    // initialized in ctor
      /**
       * Used to flag a block as invalid and prevent other blocks from
       * building on top of it.
       */
      bool                          invalid = false;
      block_id_type                 id;
      block_header                  header;
      shared_ptr<signed_block>      data;
      vector<producer_confirmation> confirmations;
   };

   typedef shared_ptr<header_state> item_ptr;


   /**
    *  As long as blocks are pushed in order the fork
    *  database will maintain a linked tree of all blocks
    *  that branch from the start_block.  The tree will
    *  have a maximum depth of 1024 blocks after which
    *  the database will start lopping off forks.
    *
    *  Every time a block is pushed into the fork DB the
    *  block with the highest block_num will be returned.
    */
   class fork_database
   {
      public:
         typedef vector<item_ptr> branch_type;
         /// The maximum number of blocks that may be skipped in an out-of-order push
         const static int MAX_BLOCK_REORDERING = 1024;

         fork_database();
         void reset();

         void                             start_block(signed_block b);
         void                             remove(block_id_type b);
         void                             set_head(shared_ptr<header_state> h);
         bool                             is_known_block(const block_id_type& id)const;
         shared_ptr<header_state>         fetch_block(const block_id_type& id)const;
         vector<item_ptr>                 fetch_block_by_number(uint32_t n)const;

         /**
          * @return the block that the signature was added to
          * throw exception if signature was not from producer in schedule for said block
          */
         shared_ptr<header_state>         push_confirmation( const producer_confirmation& c );

         /**
          *  @return the new head block ( the longest fork )
          */
         shared_ptr<header_state>          push_block( const signed_block& b );
         shared_ptr<header_state>          push_block_header( const signed_block_header& b );

         shared_ptr<header_state>          head()const { return _head; }
         void                              pop_block();

         /**
          *  Given two head blocks, return two branches of the fork graph that
          *  end with a common ancestor (same prior block)
          */
         pair< branch_type, branch_type >  fetch_branch_from(block_id_type first,
                                                             block_id_type second)const;

         struct by_block_id;
         struct by_block_num;
         struct by_previous;
         typedef multi_index_container<
            item_ptr,
            indexed_by<
               hashed_unique<tag<by_block_id>, member<header_state, block_id_type, &header_state::id>, std::hash<block_id_type>>,
               hashed_non_unique<tag<by_previous>, const_mem_fun<header_state, block_id_type, &header_state::previous_id>, std::hash<block_id_type>>,
               ordered_non_unique<tag<by_block_num>, member<header_state,uint32_t,&header_state::num>>
            >
         > fork_multi_index_type;

         void set_max_size( uint32_t s );

      private:
         /** @return a pointer to the newly pushed item */
         void _push_block( const item_ptr& b );

         uint32_t                 _max_size = 1024;

         fork_multi_index_type       _index;
         shared_ptr<header_state>    _head;
   };
} } // eosio::chain
