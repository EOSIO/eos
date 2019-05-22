/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "fork_test_utilities.hpp"

private_key_type get_private_key( name keyname, string role ) {
   return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(keyname.to_string()+role));
}

public_key_type  get_public_key( name keyname, string role ){
   return get_private_key( keyname, role ).get_public_key();
}

void push_blocks( tester& from, tester& to, uint32_t block_num_limit ) {
   while( to.control->fork_db_pending_head_block_num()
            < std::min( from.control->fork_db_pending_head_block_num(), block_num_limit ) )
   {
      auto fb = from.control->fetch_block_by_number( to.control->fork_db_pending_head_block_num()+1 );
      to.push_block( fb );
   }
}

bool produce_empty_blocks_until( tester& t,
                                 account_name last_producer,
                                 account_name next_producer,
                                 uint32_t max_num_blocks_to_produce )
{
   auto condition_satisfied = [&t, last_producer, next_producer]() {
      return t.control->pending_block_producer() == next_producer && t.control->head_block_producer() == last_producer;
   };

   for( uint32_t blocks_produced = 0;
        blocks_produced < max_num_blocks_to_produce;
        t.produce_block(), ++blocks_produced )
   {
      if( condition_satisfied() )
         return true;
   }

   return condition_satisfied();
}
