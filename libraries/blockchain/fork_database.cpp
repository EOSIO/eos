/*
 * Copyright (c) 2017, Respective Authors.
 */
#include <eosio/blockchain/fork_database.hpp>
#include <eosio/blockchain/exceptions.hpp>
//#include <fc/smart_ref_impl.hpp>

namespace eosio { namespace blockchain {
fork_database::fork_database()
{
}
void fork_database::reset()
{
   _head.reset();
   _index.clear();
}

void fork_database::pop_block()
{
   FC_ASSERT( _head, "no blocks to pop" );
   auto prev = _head->prev.lock();
   FC_ASSERT( prev, "poping block would leave head block null" );
    _head = prev;
}

void  fork_database::start_block( block_data_ptr  b)
{
   auto item = std::make_shared<meta_block>( move(b) );
   _index.insert(item);
   _head = item;
}

/**
 * Pushes the block into the fork database and caches it if it doesn't link
 *
 */
meta_block_ptr  fork_database::push_block( block_data_ptr b )
{
   auto item = std::make_shared<meta_block>( move(b) );
   try {
      _push_block(item);
   }
   catch ( const unlinkable_block_exception& e )
   {
      wlog( "Pushing block to fork database that failed to link: ${id}, ${num}", ("id",item->id)("num",item->blocknum) );
      wlog( "Head: ${num}, ${id}", ("num",_head->blocknum)("id",_head->id) );
      throw;
   }
   return _head;
}

meta_block_ptr fork_database::push_block( meta_block_ptr blk ) {
   try { _push_block( blk ); }
   catch ( const unlinkable_block_exception& e )
   {
      wlog( "Pushing block to fork database that failed to link: ${id}, ${num}", ("id",blk->id)("num",blk->blocknum) );
      wlog( "Head: ${num}, ${id}", ("num",_head->blocknum)("id",_head->id) );
      throw;
   }
   return _head;
}

void  fork_database::_push_block(const meta_block_ptr& item)
{
   if( _head ) // make sure the block is within the range that we are caching
   {
      FC_ASSERT( item->blocknum > std::max<int64_t>( 0, int64_t(_head->blocknum) - (_max_size) ),
                 "attempting to push a block that is too old", 
                 ("item->blocknum",item->blocknum)("head",_head->blocknum)("max_size",_max_size));
   }

   if( _head && item->previous_id != block_id_type() )
   {
      auto& index = _index.get<block_id>();
      auto itr = index.find(item->previous_id);
      EOS_ASSERT(itr != index.end(), unlinkable_block_exception, "block does not link to known chain");
      FC_ASSERT(!(*itr)->invalid);
      item->prev = *itr;
   }

   _index.insert(item);
   if( !_head ) _head = item;
   else if( item->blocknum > _head->blocknum )
   {
      _head = item;
      uint32_t min_num = _head->blocknum - std::min( _max_size, _head->blocknum );
//      ilog( "min block in fork DB ${n}, max_size: ${m}", ("n",min_num)("m",_max_size) );
      auto& num_idx = _index.get<block_num>();
      while( num_idx.size() && (*num_idx.begin())->blocknum < min_num )
         num_idx.erase( num_idx.begin() );
   }
}


/**
 * Marks a particular block as irreversible and removes all prior blocks
 * or parallel forks with different ids.
 */
void fork_database::set_irreversible( block_id_type id ) {
   if( !_head ) return;

   auto n = block_data::num_from_id(id);

   auto& by_num_idx = _index.get<block_num>();
   auto itr = by_num_idx.begin();
   while( itr != by_num_idx.end() )
   {
      const auto& meta = **itr;
      if( meta.blocknum < n )
         by_num_idx.erase(itr);
      else if( meta.blocknum == n && meta.id != id )
      {
         /// TODO: we need to remove all forks that build off of meta.id in order to maintain the
         /// invariant that only linkable blocks can be pushed into the fork database, there is no
         /// need to do this for blocks prior to n because all forks are covered by previous clause and
         /// those forks will havea a parallel chain for 'n' which we will pick up here.
         by_num_idx.erase(itr);
      }
      else 
         break;
      itr = by_num_idx.begin();
   }

}

bool fork_database::is_known_block(const block_id_type& id)const
{
   auto& index = _index.get<block_id>();
   auto itr = index.find(id);
   return itr != index.end();
}

meta_block_ptr fork_database::fetch_block(const block_id_type& id)const
{
   auto& index = _index.get<block_id>();
   auto itr = index.find(id);
   if( itr != index.end() )
      return *itr;
   return meta_block_ptr();
}

vector<meta_block_ptr> fork_database::fetch_block_by_number(uint32_t num)const
{
   vector<meta_block_ptr> result;
   auto itr = _index.get<block_num>().find(num);
   while( itr != _index.get<block_num>().end() )
   {
      if( (*itr)->blocknum == num )
         result.push_back( *itr );
      else
         break;
      ++itr;
   }
   return result;
}

pair<fork_database::branch_type,fork_database::branch_type>
  fork_database::fetch_branch_from(block_id_type first, block_id_type second)const
{ try {
   // This function gets a branch (i.e. vector<meta_block>) leading
   // back to the most recent common ancestor.
   pair<branch_type,branch_type> result;
   auto first_branch_itr = _index.get<block_id>().find(first);
   FC_ASSERT(first_branch_itr != _index.get<block_id>().end());
   auto first_branch = *first_branch_itr;

   auto second_branch_itr = _index.get<block_id>().find(second);
   FC_ASSERT(second_branch_itr != _index.get<block_id>().end());
   auto second_branch = *second_branch_itr;


   while( first_branch->blocknum > second_branch->blocknum )
   {
      result.first.push_back(first_branch);
      first_branch = first_branch->prev.lock();
      FC_ASSERT(first_branch);
   }
   while( second_branch->blocknum > first_branch->blocknum )
   {
      result.second.push_back( second_branch );
      second_branch = second_branch->prev.lock();
      FC_ASSERT(second_branch);
   }
   while( first_branch->previous_id != second_branch->previous_id )
   {
      result.first.push_back(first_branch);
      result.second.push_back(second_branch);
      first_branch = first_branch->prev.lock();
      FC_ASSERT(first_branch);
      second_branch = second_branch->prev.lock();
      FC_ASSERT(second_branch);
   }
   if( first_branch && second_branch )
   {
      result.first.push_back(first_branch);
      result.second.push_back(second_branch);
   }
   return result;
} FC_CAPTURE_AND_RETHROW( (first)(second) ) }

void fork_database::set_head( meta_block_ptr  h)
{
   _head = h;
}

void fork_database::remove(block_id_type id)
{
   _index.get<block_id>().erase(id);
}

} } // eosio::blockchain
