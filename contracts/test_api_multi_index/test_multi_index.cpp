#include <eosiolib/multi_index.hpp>
#include "../test_api/test_api.hpp"
#include <eosiolib/print.hpp>

namespace _test_multi_index {

   using eosio::key256;

   struct record_idx64 {
      uint64_t id;
      uint64_t sec;

      auto primary_key()const { return id; }
      uint64_t get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx64, (id)(sec) )
   };

   struct record_idx128 {
      uint64_t  id;
      uint128_t sec;

      auto primary_key()const { return id; }
      uint128_t get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx128, (id)(sec) )
   };

   struct record_idx256 {
      uint64_t id;
      key256   sec;

      auto primary_key()const { return id; }
      const key256& get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx256, (id)(sec) )
   };

   template<uint64_t TableName>
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
      > table( current_receiver(), current_receiver() );

      auto payer = current_receiver();

      for (size_t i = 0; i < num_records; ++i) {
         table.emplace( payer, [&]( auto& r ) {
            r.id = records[i].id;
            r.sec = records[i].sec;
         });
      }
   }

   template<uint64_t TableName>
   void idx64_check_without_storing()
   {
      using namespace eosio;

      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<TableName, record,
         indexed_by< N(bysecondary), const_mem_fun<record, uint64_t, &record::get_secondary> >
      > table( current_receiver(), current_receiver() );

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
         eosio_assert(itr == secondary_index.begin() && itr->id == 265 && itr->sec == N(alice), "idx64_general - decrement secondary iterator to beginning");
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
   _test_multi_index::idx64_store_only<N(indextable1)>();
}

void test_multi_index::idx64_check_without_storing()
{
   _test_multi_index::idx64_check_without_storing<N(indextable1)>();
}

void test_multi_index::idx64_general()
{
   _test_multi_index::idx64_store_only<N(indextable2)>();
   _test_multi_index::idx64_check_without_storing<N(indextable2)>();
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
   > table( current_receiver(), current_receiver() );

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
   > table( current_receiver(), current_receiver() );

   for( int i = 0; i < 3; ++i ) {
      table.emplace( payer, [&]( auto& r ) {
         r.id = table.available_primary_key();
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   table.remove(table.get(0));

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

   {
      multi_index<table_name, record,
         indexed_by< N(bysecondary), const_mem_fun<record, uint128_t, &record::get_secondary> >
      > table( current_receiver(), current_receiver() );

      eosio_assert( table.available_primary_key() == 3, "idx128_autoincrement_test_part2 - did not recover expected next primary key");
   }

   multi_index<table_name, record,
      indexed_by< N(bysecondary), const_mem_fun<record, uint128_t, &record::get_secondary> >
   > table( current_receiver(), current_receiver() );

   table.emplace( payer, [&]( auto& r) {
      r.id = 0;
      r.sec = 1000;
   });
   // Done this way to make sure that table._next_primary_key is not incorrectly set to 1.

   for( int i = 3; i < 5; ++i ) {
      table.emplace( payer, [&]( auto& r ) {
         auto itr = table.available_primary_key();
         print(itr, "\n");
         r.id = itr;
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

void test_multi_index::idx256_general()
{
   using namespace eosio;
   using namespace _test_multi_index;

   typedef record_idx256 record;

   const uint64_t table_name = N(indextable5);
   auto payer = current_receiver();

   print("Testing key256 secondary index.\n");
   multi_index<table_name, record,
      indexed_by< N(bysecondary), const_mem_fun<record, const key256&, &record::get_secondary> >
   > table( current_receiver(), current_receiver() );

   auto fourtytwo       = key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL);
   auto onetwothreefour = key256::make_from_word_sequence<uint64_t>(1ULL, 2ULL, 3ULL, 4ULL);

   const auto& entry1 = table.emplace( payer, [&]( auto& o ) {
      o.id = 1;
      o.sec = fourtytwo;
   });

   const auto& entry2 = table.emplace( payer, [&]( auto& o ) {
      o.id = 2;
      o.sec = onetwothreefour;
   });

   const auto& entry3 = table.emplace( payer, [&]( auto& o ) {
      o.id = 3;
      o.sec = fourtytwo;
   });

   const auto* e = table.find( 2 );

   print("Items sorted by primary key:\n");
   for( const auto& item : table ) {
      print(" ID=", item.primary_key(), ", secondary=", item.sec, "\n");
   }

   {
      auto itr = table.begin();
      eosio_assert( itr->primary_key() == 1 && itr->get_secondary() == fourtytwo, "idx256_general - primary key sort" );
      ++itr;
      eosio_assert( itr->primary_key() == 2 && itr->get_secondary() == onetwothreefour, "idx256_general - primary key sort" );
      ++itr;
      eosio_assert( itr->primary_key() == 3 && itr->get_secondary() == fourtytwo, "idx256_general - primary key sort" );
      ++itr;
      eosio_assert( itr == table.end(), "idx256_general - primary key sort" );
   }

   auto secidx = table.get_index<N(bysecondary)>();

   auto lower1 = secidx.lower_bound(key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 40ULL));
   print("First entry with a secondary key of at least 40 has ID=", lower1->id, ".\n");
   eosio_assert( lower1->id == 1, "idx256_general - lower_bound" );

   auto lower2 = secidx.lower_bound(key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 50ULL));
   print("First entry with a secondary key of at least 50 has ID=", lower2->id, ".\n");
   eosio_assert( lower2->id == 2, "idx256_general - lower_bound" );

   if( &*lower2 == e ) {
      print("Previously found entry is the same as the one found earlier with a primary key value of 2.\n");
   }

   print("Items sorted by secondary key (key256):\n");
   for( const auto& item : secidx ) {
      print(" ID=", item.primary_key(), ", secondary=");
      cout << item.sec << "\n";
   }

   {
      auto itr = secidx.begin();
      eosio_assert( itr->primary_key() == 1, "idx256_general - secondary key sort" );
      ++itr;
      eosio_assert( itr->primary_key() == 3, "idx256_general - secondary key sort" );
      ++itr;
      eosio_assert( itr->primary_key() == 2, "idx256_general - secondary key sort" );
      ++itr;
      eosio_assert( itr == secidx.end(), "idx256_general - secondary key sort" );
   }

   auto upper = secidx.upper_bound(key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL));

   print("First entry with a secondary key greater than 42 has ID=", upper->id, ".\n");
   eosio_assert( upper->id == 2, "idx256_general - upper_bound" );

   print("Removed entry with ID=", lower1->id, ".\n");
   table.remove( *lower1 );

   print("Items sorted by primary key:\n");
   for( const auto& item : table ) {
      print(" ID=", item.primary_key(), ", secondary=", item.sec, "\n");
   }

   {
      auto itr = table.begin();
      eosio_assert( itr->primary_key() == 2 && itr->get_secondary() == onetwothreefour, "idx256_general - primary key sort after remove" );
      ++itr;
      eosio_assert( itr->primary_key() == 3 && itr->get_secondary() == fourtytwo, "idx256_general - primary key sort after remove" );
      ++itr;
      eosio_assert( itr == table.end(), "idx256_general - primary key sort after remove" );
   }
}
