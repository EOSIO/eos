#include <eosiolib/multi_index.hpp>
#include "../test_api/test_api.hpp"

namespace _test_multi_index {

   struct record_idx64 {
      uint64_t id;
      uint64_t sec;

      auto primary_key()const { return id; }
      uint64_t get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx64, (id)(sec) )
   };

   struct record_idx128 {
      uint64_t id;
      uint128_t sec;

      auto primary_key()const { return id; }
      uint128_t get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx128, (id)(sec) )
   };

   template<uint64_t ScopeName, uint64_t TableName>
   void idx64_store_only()
   {
      using namespace eosio;

      typedef record_idx64 record;

      record records[] = {{265, N(alice)},
                          {781, N(bob)},
                          {234, N(charlie)},
                          {650, N(allyson)},
                          {540, N(bob)},
                          {976, N(emily)},
                          {110, N(joe)}
      };
      size_t num_records = sizeof(records)/sizeof(records[0]);

      // Construct and fill table using multi_index
      multi_index<TableName, record,
         indexed_by< N(bysecondary), const_mem_fun<record, uint64_t, &record::get_secondary> >
      > table( current_receiver(), ScopeName );

      auto payer = current_receiver();

      for (size_t i = 0; i < num_records; ++i) {
         table.emplace( payer, [&]( auto& r ) {
            r.id = records[i].id;
            r.sec = records[i].sec;
         });
      }
   }

   template<uint64_t ScopeName, uint64_t TableName>
   void idx64_check_without_storing()
   {
      using namespace eosio;

      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<TableName, record,
         indexed_by< N(bysecondary), const_mem_fun<record, uint64_t, &record::get_secondary> >
      > table( current_receiver(), ScopeName );

      auto payer = current_receiver();

      auto secondary_index = table.template get_index<N(bysecondary)>();

      // find by primary key
      {
         auto ptr = table.find(999);
         eosio_assert(ptr == nullptr, "idx64_general - table.find() of non-existing primary key");

         ptr = table.find(976);
         eosio_assert(ptr != nullptr && ptr->sec == N(emily), "idx64_general - table.find() of existing primary key");

         // Workaround: would prefer to instead receive iterator (rather than pointer) from find().
         auto itr = table.lower_bound(976);
         eosio_assert(itr != table.end() && itr->id == 976 && itr->sec == N(emily), "idx64_general - iterator to existing object in primary index");

         ++itr;
         eosio_assert(itr == table.end(), "idx64_general - increment primary iterator to end");
      }

      // iterate forward starting with charlie
      {
         auto itr = secondary_index.lower_bound(N(charlie));
         eosio_assert(itr != secondary_index.end() && itr->sec == N(charlie), "idx64_general - secondary_index.lower_bound()");

         ++itr;
         eosio_assert(itr != secondary_index.end() && itr->id == 976 && itr->sec == N(emily), "idx64_general - increment secondary iterator");

         ++itr;
         eosio_assert(itr != secondary_index.end() && itr->id == 110 && itr->sec == N(joe), "idx64_general - increment secondary iterator again");

         ++itr;
         eosio_assert(itr == secondary_index.end(), "idx64_general - increment secondary iterator to end");
      }

      // iterate backward staring with second bob
      {
         auto ptr = table.find(781);
         eosio_assert(ptr != nullptr && ptr->sec == N(bob), "idx64_general - table.find() of existing primary key");

         // Workaround: need to add find_primary wrapper support in secondary indices of multi_index
         auto itr = secondary_index.upper_bound(ptr->sec); --itr;
         eosio_assert(itr->id == 781 && itr->sec == N(bob), "idx64_general - iterator to existing object in secondary index");

         --itr;
         eosio_assert(itr != secondary_index.end() && itr->id == 540 && itr->sec == N(bob), "idx64_general - decrement secondary iterator");

         --itr;
         eosio_assert(itr != secondary_index.end() && itr->id == 650 && itr->sec == N(allyson), "idx64_general - decrement secondary iterator again");

         --itr;
         eosio_assert(itr != secondary_index.end() && itr->id == 265 && itr->sec == N(alice), "idx64_general - decrement secondary iterator to beginning");

         --itr; // Decrementing an iterator at the beginning (technically undefined behavior) turns it into end(). Is this desired?
         eosio_assert(itr == secondary_index.end(), "idx64_general - decrement secondary iterator that was already at beginning");
      }

      // update and remove
      {
         const uint64_t ssn = 421;
         const auto& new_person = table.emplace( payer, [&]( auto& r ) {
            r.id = ssn;
            r.sec = N(bob);
         });

         table.update(new_person, payer, [&]( auto& r ) {
            r.sec = N(billy);
         });

         auto ptr = table.find(ssn);
         eosio_assert(ptr != nullptr && ptr->sec == N(billy), "idx64_general - table.update()");

         table.remove(*ptr);
         auto ptr2 = table.find(ssn);
         eosio_assert( ptr2 == nullptr, "idx64_general - table.remove()");
      }
   }

} /// _test_multi_index

void test_multi_index::idx64_store_only()
{
   _test_multi_index::idx64_store_only<N(idx1.step), N(indextable1)>();
}

void test_multi_index::idx64_check_without_storing()
{
   _test_multi_index::idx64_check_without_storing<N(indx1.step), N(indextable1)>();
}

void test_multi_index::idx64_general()
{
   _test_multi_index::idx64_store_only<N(idx1.comb), N(indextable2)>();
   _test_multi_index::idx64_check_without_storing<N(idx1.comb), N(indextable2)>();
}

void test_multi_index::idx128_autoincrement_test()
{
   using namespace eosio;
   using namespace _test_multi_index;

   typedef record_idx128 record;

   const uint64_t table_name = N(indextable3);
   auto payer = current_receiver();

   multi_index<table_name, record,
      indexed_by< N(bysecondary), const_mem_fun<record, uint128_t, &record::get_secondary> >
   > table( current_receiver(), N(inc.comb) );

   for( int i = 0; i < 5; ++i ) {
      table.emplace( payer, [&]( auto& r ) {
         r.id = table.available_primary_key();
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   uint64_t expected_key = 4;
   for( const auto& r : table.get_index<N(bysecondary)>() )
   {
      eosio_assert( r.primary_key() == expected_key, "idx128_autoincrement_test - unexpected primary key" );
      --expected_key;
   }

   auto ptr = table.find(3);
   eosio_assert( ptr != nullptr, "idx128_autoincrement_test - could not find object with primary key of 3" );

   table.update(*ptr, payer, [&]( auto& r ) {
      r.id = 100;
   });

   eosio_assert( table.available_primary_key() == 101, "idx128_autoincrement_test - next_primary_key was not correct after record update" );

}

void test_multi_index::idx128_autoincrement_test_part1()
{
   using namespace eosio;
   using namespace _test_multi_index;

   typedef record_idx128 record;

   const uint64_t table_name = N(indextable4);
   auto payer = current_receiver();

   multi_index<table_name, record,
      indexed_by< N(bysecondary), const_mem_fun<record, uint128_t, &record::get_secondary> >
   > table( current_receiver(), N(inc.step) );

   for( int i = 0; i < 3; ++i ) {
      table.emplace( payer, [&]( auto& r ) {
         r.id = table.available_primary_key();
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   uint64_t expected_key = 2;
   for( const auto& r : table.get_index<N(bysecondary)>() )
   {
      eosio_assert( r.primary_key() == expected_key, "idx128_autoincrement_test_part1 - unexpected primary key" );
      --expected_key;
   }

}

void test_multi_index::idx128_autoincrement_test_part2()
{
   using namespace eosio;
   using namespace _test_multi_index;

   typedef record_idx128 record;

   const uint64_t table_name = N(indextable4);
   auto payer = current_receiver();

   multi_index<table_name, record,
      indexed_by< N(bysecondary), const_mem_fun<record, uint128_t, &record::get_secondary> >
   > table( current_receiver(), N(inc.step) );

   eosio_assert( table.available_primary_key() == 3, "idx128_autoincrement_test_part2 - did not recover expected next primary key");

   for( int i = 3; i < 5; ++i ) {
      table.emplace( payer, [&]( auto& r ) {
         r.id = table.available_primary_key();
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   uint64_t expected_key = 4;
   for( const auto& r : table.get_index<N(bysecondary)>() )
   {
      eosio_assert( r.primary_key() == expected_key, "idx128_autoincrement_test_part2 - unexpected primary key" );
      --expected_key;
   }

   auto ptr = table.find(3);
   eosio_assert( ptr != nullptr, "idx128_autoincrement_test_part2 - could not find object with primary key of 3" );

   table.update(*ptr, payer, [&]( auto& r ) {
      r.id = 100;
   });

   eosio_assert( table.available_primary_key() == 101, "idx128_autoincrement_test_part2 - next_primary_key was not correct after record update" );

}
