/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include "multi_index_test.hpp"

using namespace eosio;

void multi_index_test::test_128bit_secondary_index() {
   print( "Testing uint128_t secondary index.\n" );

   orders_table orders( get_self(), get_self().value );
   name payer = get_self();

   orders.emplace( payer, [&]( auto& o ) {
      o.id = 1;
      o.expiration = 300;
      o.owner = "dan"_n;
   } );

   auto order2 = orders.emplace( payer, [&]( auto& o ) {
      o.id = 2;
      o.expiration = 200;
      o.owner = "alice"_n;
   } );

   print("Items sorted by primary key:\n");
   for( const auto& item : orders ) {
      print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name{item.owner}, "\n");
   }

   auto expidx = orders.get_index<"byexp"_n>();

   print("Items sorted by expiration:\n");
   for( const auto& item : expidx ) {
      print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name{item.owner}, "\n");
   }

   print("Modifying expiration of order with ID=2 to 400.\n");
   orders.modify( order2, payer, [&]( auto& o ) {
      o.expiration = 400;
   } );

   print("Items sorted by expiration:\n");
   for( const auto& item : expidx ) {
      print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name{item.owner}, "\n");
   }

   auto lower = expidx.lower_bound( 100 );

   print("First order with an expiration of at least 100 has ID=", lower->id, " and expiration=", lower->get_expiration(), "\n");
}

void multi_index_test::test_256bit_secondary_index() {
   print( "Testing key256 secondary index.\n" );

   k256_table testtable( get_self(), get_self().value );
   name payer = get_self();

   testtable.emplace( payer, [&]( auto& o ) {
      o.id = 1;
      o.val = checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL);
   } );

   testtable.emplace( payer, [&]( auto& o ) {
      o.id = 2;
      o.val = checksum256::make_from_word_sequence<uint64_t>(1ULL, 2ULL, 3ULL, 4ULL);
   } );

   testtable.emplace( payer, [&]( auto& o ) {
      o.id = 3;
      o.val = checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL);
   } );

   auto itr = testtable.find( 2 );

   print("Items sorted by primary key:\n");
   for( const auto& item : testtable ) {
      print(" ID=", item.primary_key(), ", val=", item.val, "\n");
   }

   auto validx = testtable.get_index<"byval"_n>();

   auto lower1 = validx.lower_bound( checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 40ULL) );
   print("First entry with a val of at least 40 has ID=", lower1->id, ".\n");

   auto lower2 = validx.lower_bound( checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 50ULL) );
   print("First entry with a val of at least 50 has ID=", lower2->id, ".\n");

   if( testtable.iterator_to(*lower2) == itr ) {
      print("Previously found entry is the same as the one found earlier with a primary key value of 2.\n");
   }

   print("Items sorted by val (secondary key):\n");
   for( const auto& item : validx ) {
      print(" ID=", item.primary_key(), ", val=", item.val, "\n");
   }

   auto upper = validx.upper_bound( checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL) );

   print("First entry with a val greater than 42 has ID=", upper->id, ".\n");

   print("Removed entry with ID=", lower1->id, ".\n");
   validx.erase( lower1 );

   print("Items sorted by primary key:\n");
   for( const auto& item : testtable ) {
      print(" ID=", item.primary_key(), ", val=", item.val, "\n");
   }
}
