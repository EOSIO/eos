/*
 * Copyright (c) 2017, Respective Authors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <eos/chain/block.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>


namespace eos { namespace chain {
   using boost::multi_index_container;
   using namespace boost::multi_index;

   struct fork_item
   {
      fork_item( signed_block d )
      :num(d.block_num()),id(d.id()),data( std::move(d) ){}

      block_id_type previous_id()const { return data.previous; }

      weak_ptr< fork_item > prev;
      uint32_t              num;    // initialized in ctor
      /**
       * Used to flag a block as invalid and prevent other blocks from
       * building on top of it.
       */
      bool                  invalid = false;
      block_id_type         id;
      signed_block          data;
   };
   typedef shared_ptr<fork_item> item_ptr;


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
         void                             set_head(shared_ptr<fork_item> h);
         bool                             is_known_block(const block_id_type& id)const;
         shared_ptr<fork_item>            fetch_block(const block_id_type& id)const;
         vector<item_ptr>                 fetch_block_by_number(uint32_t n)const;

         /**
          *  @return the new head block ( the longest fork )
          */
         shared_ptr<fork_item>            push_block(const signed_block& b);
         shared_ptr<fork_item>            head()const { return _head; }
         void                             pop_block();

         /**
          *  Given two head blocks, return two branches of the fork graph that
          *  end with a common ancestor (same prior block)
          */
         pair< branch_type, branch_type >  fetch_branch_from(block_id_type first,
                                                             block_id_type second)const;

         struct block_id;
         struct block_num;
         struct by_previous;
         typedef multi_index_container<
            item_ptr,
            indexed_by<
               hashed_unique<tag<block_id>, member<fork_item, block_id_type, &fork_item::id>, std::hash<block_id_type>>,
               hashed_non_unique<tag<by_previous>, const_mem_fun<fork_item, block_id_type, &fork_item::previous_id>, std::hash<block_id_type>>,
               ordered_non_unique<tag<block_num>, member<fork_item,uint32_t,&fork_item::num>>
            >
         > fork_multi_index_type;

         void set_max_size( uint32_t s );

      private:
         /** @return a pointer to the newly pushed item */
         void _push_block(const item_ptr& b );
         void _push_next(const item_ptr& newly_inserted);

         uint32_t                 _max_size = 1024;

         fork_multi_index_type    _unlinked_index;
         fork_multi_index_type    _index;
         shared_ptr<fork_item>    _head;
   };
} } // eos::chain
