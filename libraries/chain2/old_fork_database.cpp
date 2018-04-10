/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/fork_database.hpp>
#include <eosio/chain/exceptions.hpp>
#include <fc/smart_ref_impl.hpp>

namespace eosio { namespace chain {
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

void     fork_database::start_block(signed_block b)
{
   auto item = std::make_shared<header_state>(std::move(b));
   _index.insert(item);
   _head = item;
}

/**
 * Pushes the block into the fork database and caches it if it doesn't link
 *
 */
shared_ptr<header_state>  fork_database::push_block( const signed_block& b )
{
   auto item = push_block_header( b );
   item->data = std::make_shared<signed_block>(b);
   return item;
}

shared_ptr<header_state> fork_database::push_block_header( const signed_block_header& b ) { try {
   const auto& by_id_idx = _index.get<by_block_id>();
   auto existing = by_id_idx.find( b.id() );

   if( existing != by_id_idx.end() ) return *existing;

   auto previous = by_id_idx.find( b.previous );
   if( previous == by_id_idx.end() ) {
      wlog( "Pushing block to fork database that failed to link: ${id}, ${num}", ("id",b.id())("num",b.block_num()) );
      wlog( "Head: ${num}, ${id}", ("num",_head->num)("id",_head->id) );
      EOS_ASSERT( previous != by_id_idx.end(), unlinkable_block_exception, "block does not link to known chain");
   }

   EOS_ASSERT( !(*previous)->invalid, unlinkable_block_exception, "unable to link to a known invalid block" );

   auto next = std::make_shared<header_state>( **previous );
   next->confirmations.clear();
   next->data.reset();

   next->prev      = *previous;
   next->header    = b;
   next->id        = b.id();
   next->invalid   = false;
   next->num       = (*previous)->num + 1;
   next->last_block_per_producer[b.producer] = next->num;

   FC_ASSERT( b.timestamp > (*previous)->header.timestamp, "block must advance time" );

   // next->last_irreversible_block = next->calculate_last_irr

   if( next->last_irreversible_block >= next->pending_schedule_block )
      next->active_schedule = std::move(next->pending_schedule );

   if( b.new_producers ) {
      FC_ASSERT( !next->pending_schedule, "there is already an unconfirmed pending schedule" );
      next->pending_schedule = std::make_shared<producer_schedule_type>( *b.new_producers );
   }

   FC_ASSERT( b.schedule_version == next->active_schedule->version, "wrong schedule version provided" );

   auto schedule_hash = fc::sha256::hash( *next->active_schedule );
   auto signee = b.signee( schedule_hash );

   auto num_producers = next->active_schedule->producers.size();
   vector<uint32_t>                    ibb;
   flat_map<account_name, uint32_t>    new_block_per_producer;
   ibb.reserve( num_producers );

   size_t offset = EOS_PERCENT(ibb.size(), config::percent_100- config::irreversible_threshold_percent);

   for( const auto& item : next->active_schedule->producers ) {
      ibb.push_back( next->last_block_per_producer[item.producer_name] );
      new_block_per_producer[item.producer_name] = ibb.back();
   }
   next->last_block_per_producer = move(new_block_per_producer);

   std::nth_element( ibb.begin(), ibb.begin() + offset, ibb.end() );
   next->last_irreversible_block = ibb[offset];

   auto index = b.timestamp.slot % (num_producers * config::producer_repetitions);
   index /= config::producer_repetitions;

   auto prod =  next->active_schedule->producers[index];
   FC_ASSERT( prod.producer_name == b.producer, "unexpected producer specified" );
   FC_ASSERT( prod.block_signing_key == signee, "block not signed by expected key" );

   _push_block( next );
   return _head;
} FC_CAPTURE_AND_RETHROW( (b) ) }

void  fork_database::_push_block(const item_ptr& item )
{
   if( _head ) // make sure the block is within the range that we are caching
   {
      FC_ASSERT( item->num > std::max<int64_t>( 0, int64_t(_head->num) - (_max_size) ),
                 "attempting to push a block that is too old", 
                 ("item->num",item->num)("head",_head->num)("max_size",_max_size));
   }

   if( _head && item->previous_id() != block_id_type() )
   {
      auto& index = _index.get<by_block_id>();
      auto itr = index.find(item->previous_id());
      EOS_ASSERT(itr != index.end(), unlinkable_block_exception, "block does not link to known chain");
      FC_ASSERT(!(*itr)->invalid);

      item->prev = *itr;
   }

   _index.insert(item);
   if( !_head ) _head = item;
   else if( item->num > _head->num )
   {
      uint32_t delta = item->data->timestamp.slot - _head->data->timestamp.slot;
      if (delta > 1)
         wlog("Number of missed blocks: ${num}", ("num", delta-1));
      _head = item;

      uint32_t min_num = _head->last_irreversible_block - 1; //_head->num - std::min( _max_size, _head->num );
//      ilog( "min block in fork DB ${n}, max_size: ${m}", ("n",min_num)("m",_max_size) );
      auto& num_idx = _index.get<by_block_num>();
      while( num_idx.size() && (*num_idx.begin())->num < min_num )
         num_idx.erase( num_idx.begin() );
   }
}


void fork_database::set_max_size( uint32_t s )
{
   _max_size = s;
   if( !_head ) return;

   { /// index
      auto& by_num_idx = _index.get<by_block_num>();
      auto itr = by_num_idx.begin();
      while( itr != by_num_idx.end() )
      {
         if( (*itr)->num < std::max(int64_t(0),int64_t(_head->num) - _max_size) )
            by_num_idx.erase(itr);
         else
            break;
         itr = by_num_idx.begin();
      }
   }

/*
   { /// unlinked_index

      auto& by_num_idx = _unlinked_index.get<by_block_num>();
      auto itr = by_num_idx.begin();
      while( itr != by_num_idx.end() )
      {
         if( (*itr)->num < std::max(int64_t(0),int64_t(_head->num) - _max_size) )
            by_num_idx.erase(itr);
         else
            break;
         itr = by_num_idx.begin();
      }
   }
*/
}

bool fork_database::is_known_block(const block_id_type& id)const
{
   auto& index = _index.get<by_block_id>();
   auto itr = index.find(id);
   if( itr != index.end() )
      return true;
   return false;
  // auto& unlinked_index = _unlinked_index.get<by_block_id>();
   //auto unlinked_itr = unlinked_index.find(id);
 //  return unlinked_itr != unlinked_index.end();
}

item_ptr fork_database::fetch_block(const block_id_type& id)const
{
   auto& index = _index.get<by_block_id>();
   auto itr = index.find(id);
   if( itr != index.end() )
      return *itr;
   /*
   auto& unlinked_index = _unlinked_index.get<by_block_id>();
   auto unlinked_itr = unlinked_index.find(id);
   if( unlinked_itr != unlinked_index.end() )
      return *unlinked_itr;
      */
   return item_ptr();
}

vector<item_ptr> fork_database::fetch_block_by_number(uint32_t num)const
{
   vector<item_ptr> result;
   auto itr = _index.get<by_block_num>().find(num);
   while( itr != _index.get<by_block_num>().end() )
   {
      if( (*itr)->num == num )
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
   // This function gets a branch (i.e. vector<header_state>) leading
   // back to the most recent common ancestor.
   pair<branch_type,branch_type> result;
   auto first_branch_itr = _index.get<by_block_id>().find(first);
   FC_ASSERT(first_branch_itr != _index.get<by_block_id>().end());
   auto first_branch = *first_branch_itr;

   auto second_branch_itr = _index.get<by_block_id>().find(second);
   FC_ASSERT(second_branch_itr != _index.get<by_block_id>().end());
   auto second_branch = *second_branch_itr;


   while( first_branch->num > second_branch->num )
   {
      result.first.push_back(first_branch);
      first_branch = first_branch->prev.lock();
      FC_ASSERT(first_branch);
   }
   while( second_branch->num > first_branch->num )
   {
      result.second.push_back( second_branch );
      second_branch = second_branch->prev.lock();
      FC_ASSERT(second_branch);
   }
   while( first_branch->data->previous != second_branch->data->previous )
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

void fork_database::set_head(shared_ptr<header_state> h)
{
   wdump(("set head"));
   _head = h;
}

void fork_database::remove(block_id_type id)
{
   _index.get<by_block_id>().erase(id);
}

shared_ptr<header_state>   fork_database::push_confirmation( const producer_confirmation& c ) {
   /*
   auto b = fetch_block( c.block_id );
   if( !b->schedule ) return shared_ptr<header_state>();

   bool found = false;
   for( const auto& pro : b->schedule->producers ) {
      if( pro.producer_name == c.producer ){ 
         public_key_type pub( c.digest, c.sig );
         FC_ASSERT( pub == pro.block_signing_key, "not signed by expected key" );
         /// TODO: recover key and compare keys
         found = true; break;  
      }
   }

   FC_ASSERT( found, "Producer signed who wasn't in schedule" );

   for( const auto& con : b->confirmations ) {
      if( con == c ) return shared_ptr<header_state>();
      FC_ASSERT( con.producer != c.producer, "DOUBLE SIGN FOUND" ); 
   }
   b->confirmations.push_back(c);
   return b;
   */
   return shared_ptr<header_state>();
}

} } // eosio::chain
