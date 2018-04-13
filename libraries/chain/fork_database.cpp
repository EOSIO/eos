#include <eosio/chain/fork_database.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace eosio { namespace chain {
   using boost::multi_index_container;
   using namespace boost::multi_index;


   struct by_block_id;
   struct by_block_num;
   typedef multi_index_container<
      block_state_ptr,
      indexed_by<
         hashed_unique<tag<by_block_id>, member<block_header_state, block_id_type, &block_header_state::id>, std::hash<block_id_type>>,
         ordered_non_unique<tag<by_block_num>, member<block_header_state,uint32_t,&block_header_state::block_num>>
      >
   > fork_multi_index_type;


   struct fork_database_impl {
      fork_multi_index_type index;
      block_state_ptr       head;
   };


   fork_database::fork_database():my( new fork_database_impl() ) {
   }

   fork_database::~fork_database() {
   }

   void fork_database::set( block_state_ptr s ) {
      auto result = my->index.insert( s );
      FC_ASSERT( result.second, "unable to insert block state, duplicate state detected" );
      if( !my->head ) {
         my->head =  s;
      } else if( my->head->block_num < s->block_num ) {
         my->head =  s;
      }
   }

   block_state_ptr fork_database::add( signed_block_ptr b ) {
      FC_ASSERT( my->head, "no head block set" );

      const auto& by_id_idx = my->index.get<by_block_id>();
      auto existing = by_id_idx.find( b->id() );
      FC_ASSERT( existing == by_id_idx.end(), "we already know about this block" );

      auto prior = by_id_idx.find( b->previous );
      FC_ASSERT( prior != by_id_idx.end(), "unlinkable block" );

      auto result = std::make_shared<block_state>( **prior, move(b) );
      my->index.insert(result);

      if( result->block_num > my->head->block_num ) {
         my->head = result;
      }

      return my->head;
   }
   const block_state_ptr& fork_database::head()const { return my->head; }

   /**
    *  Given two head blocks, return two branches of the fork graph that
    *  end with a common ancestor (same prior block)
    */
   pair< branch_type, branch_type >  fork_database::fetch_branch_from( block_id_type first,
                                                                       block_id_type second )const {
      pair<branch_type,branch_type> result;
      auto first_branch = get_block(first);
      auto second_branch = get_block(second);

      while( first_branch->block_num> second_branch->block_num )
      {
         result.first.push_back(first_branch);
         first_branch = get_block( first_branch->header.previous );
         FC_ASSERT( first_branch );
      }

      while( second_branch->block_num > first_branch->block_num )
      {
         result.second.push_back( second_branch );
         second_branch = get_block( second_branch->header.previous );
         FC_ASSERT( second_branch );
      }

      while( first_branch->header.previous != second_branch->header.previous )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
         first_branch = get_block( first_branch->header.previous );
         second_branch = get_block( second_branch->header.previous );
         FC_ASSERT( first_branch && second_branch );
      }

      if( first_branch && second_branch )
      {
         result.first.push_back(first_branch);
         result.second.push_back(second_branch);
      }
      return result;
   } /// fetch_branch_from

   void fork_database::set_validity( const block_state_ptr& h, bool valid ) {
      if( !valid ) {
         auto itr = my->index.find( h->id );
         if( itr != my->index.end() )
            my->index.erase(itr);
      } else {
         /// remove older than irreversible and mark block as valid
         h->validated = true;
         auto& idx = my->index.get<by_block_num>();
         for( auto itr = idx.begin(); itr != idx.end(); ++itr ) {
            if( (*itr)->block_num < h->dpos_last_irreversible_blocknum ) {
               /// TODO: if block is part of head chain, then emit irreversible, we don't
               /// currently have an easy way to track which blocks are part of the head chain,
               /// to implement that we would need to track every time the head changes and update
               /// all blocks not on the head.
               idx.erase( itr );
               itr = idx.begin();
            } else {
               break;
            }
         }
      }
   }

   block_state_ptr   fork_database::get_block(const block_id_type& id)const {
      auto itr = my->index.find( id );
      if( itr != my->index.end() ) 
         return *itr;
      return block_state_ptr();
   }



} } /// eosio::chain
