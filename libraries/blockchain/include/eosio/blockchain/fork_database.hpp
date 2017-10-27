/*
 * Copyright (c) 2017, Respective Authors.
 */
#pragma once
#include <eosio/blockchain/block.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>


namespace eosio { namespace blockchain {
   using boost::multi_index_container;
   using namespace boost::multi_index;


   /**
    *  As long as blocks are pushed in order the fork
    *  database will maintain a linked tree of all blocks
    *  that branch from the start_block.  
    *
    *  Every time a block is pushed into the fork DB the
    *  block with the highest block_num will be returned.
    */
   class fork_database
   {
      public:
         typedef vector<meta_block_ptr> branch_type;
         /// The maximum number of blocks that may be skipped in an out-of-order push
         const static int MAX_BLOCK_REORDERING = 1024*256; /// at 500ms blocks this is about a day and a half

         fork_database();
         void reset();

         void                      start_block( block_data_ptr  b);
         void                      remove(block_id_type b);
         void                      set_head(meta_block_ptr h);
         bool                      is_known_block(const block_id_type& id)const;
         meta_block_ptr            fetch_block(const block_id_type& id)const;
         vector<meta_block_ptr>    fetch_block_by_number(uint32_t n)const;

         /**
          *  @return the new head block ( the longest fork )
          */
         meta_block_ptr            push_block( block_data_ptr b );
         meta_block_ptr            push_block( meta_block_ptr b );
         meta_block_ptr            head()const { return _head; }
         void                      pop_block();

         /**
          *  Given two head blocks, return two branches of the fork graph that
          *  end with a common ancestor (same prior block)
          */
         pair< branch_type, branch_type >  fetch_branch_from(block_id_type first,
                                                             block_id_type second)const;

         void set_irreversible( block_id_type id );
      private:
         void _push_block(const meta_block_ptr& b );

         struct block_id;
         struct block_num;
         struct by_previous;
         typedef multi_index_container<
            meta_block_ptr,
            indexed_by<
               hashed_unique<tag<block_id>, member<meta_block, block_id_type, &meta_block::id>, std::hash<block_id_type>>,
               hashed_non_unique<tag<by_previous>, member<meta_block, block_id_type, &meta_block::previous_id>, std::hash<block_id_type>>,
               ordered_non_unique<tag<block_num>, member<meta_block,block_num_type,&meta_block::blocknum>>
            >
         > fork_multi_index_type;

         fork_multi_index_type    _index;
         meta_block_ptr           _head;
         block_num_type           _max_size = 1024;
   };
} } // eosio::blockchain
