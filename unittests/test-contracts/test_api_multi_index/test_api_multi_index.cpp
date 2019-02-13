/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include "test_api_multi_index.hpp"

using namespace eosio;

#include <cmath>
#include <limits>

namespace _test_multi_index {

   using eosio::checksum256;

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
      uint64_t    id;
      checksum256 sec;

      auto primary_key()const { return id; }
      const checksum256& get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx256, (id)(sec) )
   };

   struct record_idx_double {
      uint64_t id;
      double   sec;

      auto primary_key()const { return id; }
      double get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx_double, (id)(sec) )
   };

   struct record_idx_long_double {
      uint64_t    id;
      long double sec;

      auto primary_key()const { return id; }
      long double get_secondary()const { return sec; }

      EOSLIB_SERIALIZE( record_idx_long_double, (id)(sec) )
   };

   template<uint64_t TableName>
   void idx64_store_only( name receiver )
   {
      typedef record_idx64 record;

      record records[] = {{265, "alice"_n.value},
                          {781, "bob"_n.value},
                          {234, "charlie"_n.value},
                          {650, "allyson"_n.value},
                          {540, "bob"_n.value},
                          {976, "emily"_n.value},
                          {110, "joe"_n.value}
      };
      size_t num_records = sizeof(records)/sizeof(records[0]);

      // Construct and fill table using multi_index
      multi_index<name{TableName}, record,
         indexed_by<"bysecondary"_n, const_mem_fun<record, uint64_t, &record::get_secondary>>
      > table( receiver, receiver.value );

      auto payer = receiver;

      for ( size_t i = 0; i < num_records; ++i ) {
         table.emplace( payer, [&](auto& r) {
            r.id = records[i].id;
            r.sec = records[i].sec;
         });
      }
   }

   template<uint64_t TableName>
   void idx64_check_without_storing( name receiver )
   {
      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<name{TableName}, record,
         indexed_by<"bysecondary"_n, const_mem_fun<record, uint64_t, &record::get_secondary>>
      > table( receiver, receiver.value );

      auto payer = receiver;

      auto secondary_index = table.template get_index<"bysecondary"_n>();

      // find by primary key
      {
         auto itr = table.find(999);
         check( itr == table.end(), "idx64_general - table.find() of non-existing primary key" );

         itr = table.find(976);
         check( itr != table.end() && itr->sec == "emily"_n.value, "idx64_general - table.find() of existing primary key" );

         ++itr;
         check( itr == table.end(), "idx64_general - increment primary iterator to end" );

         itr = table.require_find(976);
         check( itr != table.end() && itr->sec == "emily"_n.value, "idx64_general - table.require_find() of existing primary key" );

         ++itr;
         check( itr == table.end(), "idx64_general - increment primary iterator to end" );
      }

      // iterate forward starting with charlie
      {
         auto itr = secondary_index.lower_bound("charlie"_n.value);
         check( itr != secondary_index.end() && itr->sec == "charlie"_n.value, "idx64_general - secondary_index.lower_bound()" );

         ++itr;
         check( itr != secondary_index.end() && itr->id == 976 && itr->sec == "emily"_n.value, "idx64_general - increment secondary iterator" );

         ++itr;
         check( itr != secondary_index.end() && itr->id == 110 && itr->sec == "joe"_n.value, "idx64_general - increment secondary iterator again" );

         ++itr;
         check( itr == secondary_index.end(), "idx64_general - increment secondary iterator to end" );
      }

      // iterate backward starting with second bob
      {
         auto pk_itr = table.find(781);
         check( pk_itr != table.end() && pk_itr->sec == "bob"_n.value, "idx64_general - table.find() of existing primary key" );

         auto itr = secondary_index.iterator_to(*pk_itr);
         check( itr->id == 781 && itr->sec == "bob"_n.value, "idx64_general - iterator to existing object in secondary index" );

         --itr;
         check( itr != secondary_index.end() && itr->id == 540 && itr->sec == "bob"_n.value, "idx64_general - decrement secondary iterator" );

         --itr;
         check( itr != secondary_index.end() && itr->id == 650 && itr->sec == "allyson"_n.value, "idx64_general - decrement secondary iterator again" );

         --itr;
         check( itr == secondary_index.begin() && itr->id == 265 && itr->sec == "alice"_n.value, "idx64_general - decrement secondary iterator to beginning" );
      }

      // iterate backward starting with emily using const_reverse_iterator
      {
         std::array<uint64_t, 6> pks{{976, 234, 781, 540, 650, 265}};

         auto pk_itr = pks.begin();

         auto itr = --std::make_reverse_iterator( secondary_index.find("emily"_n.value) );
         for( ; itr != secondary_index.rend(); ++itr ) {
            check( pk_itr != pks.end(), "idx64_general - unexpected continuation of secondary index in reverse iteration" );
            check( *pk_itr == itr->id, "idx64_general - primary key mismatch in reverse iteration" );
            ++pk_itr;
         }
         check( pk_itr == pks.end(), "idx64_general - did not iterate backwards through secondary index properly" );
      }

      // require_find secondary key
      {
         auto itr = secondary_index.require_find("bob"_n.value);
         check( itr != secondary_index.end(), "idx64_general - require_find must never return end iterator" );
         check( itr->id == 540, "idx64_general - require_find test" );

         ++itr;
         check( itr->id == 781, "idx64_general - require_find secondary key test" );
      }

      // modify and erase
      {
         const uint64_t ssn = 421;
         auto new_person = table.emplace( payer, [&](auto& r) {
            r.id = ssn;
            r.sec = "bob"_n.value;
         });

         table.modify( new_person, payer, [&](auto& r) {
            r.sec = "billy"_n.value;
         });

         auto itr1 = table.find(ssn);
         check( itr1 != table.end() && itr1->sec == "billy"_n.value, "idx64_general - table.modify()" );

         table.erase(itr1);
         auto itr2 = table.find(ssn);
         check( itr2 == table.end(), "idx64_general - table.erase()" );
      }
   }

   template<uint64_t TableName>
   void idx64_require_find_fail(name receiver)
   {
      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<name{TableName}, record> table( receiver, receiver.value );

      // make sure we're looking at the right table
      auto itr = table.require_find( 781, "table not loaded" );
      check( itr != table.end(), "table not loaded" );

      // require_find by primary key
      // should fail
      itr = table.require_find(999);
   }

   template<uint64_t TableName>
   void idx64_require_find_fail_with_msg(name receiver)
   {
      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<name{TableName}, record> table( receiver, receiver.value );

      // make sure we're looking at the right table
      auto itr = table.require_find( 234, "table not loaded" );
      check( itr != table.end(), "table not loaded" );

      // require_find by primary key
      // should fail
      itr = table.require_find( 335, "unable to find primary key in require_find" );
   }

   template<uint64_t TableName>
   void idx64_require_find_sk_fail(name receiver)
   {
      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<name{TableName}, record, indexed_by<"bysecondary"_n, const_mem_fun<record, uint64_t, &record::get_secondary>>> table( receiver, receiver.value );
      auto sec_index = table.template get_index<"bysecondary"_n>();

      // make sure we're looking at the right table
      auto itr = sec_index.require_find( "charlie"_n.value, "table not loaded" );
      check( itr != sec_index.end(), "table not loaded" );

      // require_find by secondary key
      // should fail
      itr = sec_index.require_find("bill"_n.value);
   }

   template<uint64_t TableName>
   void idx64_require_find_sk_fail_with_msg(name receiver)
   {
      typedef record_idx64 record;

      // Load table using multi_index
      multi_index<name{TableName}, record, indexed_by< "bysecondary"_n, const_mem_fun<record, uint64_t, &record::get_secondary>>> table( receiver, receiver.value );
      auto sec_index = table.template get_index<"bysecondary"_n>();

      // make sure we're looking at the right table
      auto itr = sec_index.require_find( "emily"_n.value, "table not loaded" );
      check( itr != sec_index.end(), "table not loaded" );

      // require_find by secondary key
      // should fail
      itr = sec_index.require_find( "frank"_n.value, "unable to find sec key" );
   }

   template<uint64_t TableName>
   void idx128_store_only(name receiver)
   {
      typedef record_idx128 record;


      // Construct and fill table using multi_index
      multi_index<name{TableName}, record,
         indexed_by<"bysecondary"_n, const_mem_fun<record, uint128_t, &record::get_secondary>>
      > table( receiver, receiver.value );

      auto payer = receiver;

      for (uint64_t i = 0; i < 5; ++i) {
         table.emplace( payer, [&](auto& r) {
            r.id = i;
            r.sec = static_cast<uint128_t>(1ULL << 63) * i;
         });
      }
   }

   template<uint64_t TableName>
   void idx128_check_without_storing( name receiver )
   {
      typedef record_idx128 record;

      // Load table using multi_index
      multi_index<name{TableName}, record,
         indexed_by<"bysecondary"_n, const_mem_fun<record, uint128_t, &record::get_secondary>>
      > table( receiver, receiver.value );

      auto payer = receiver;

      auto secondary_index = table.template get_index<"bysecondary"_n>();

      table.modify( table.get(3), payer, [&](auto& r) {
         r.sec *= 2;
      });

      {
         uint128_t multiplier = 1ULL << 63;

         auto itr = secondary_index.begin();
         check( itr->primary_key() == 0 && itr->get_secondary() == multiplier*0, "idx128_general - secondary key sort" );
         ++itr;
         check( itr->primary_key() == 1 && itr->get_secondary() == multiplier*1, "idx128_general - secondary key sort" );
         ++itr;
         check( itr->primary_key() == 2 && itr->get_secondary() == multiplier*2, "idx128_general - secondary key sort" );
         ++itr;
         check( itr->primary_key() == 4 && itr->get_secondary() == multiplier*4, "idx128_general - secondary key sort" );
         ++itr;
         check( itr->primary_key() == 3 && itr->get_secondary() == multiplier*6, "idx128_general - secondary key sort" );
         ++itr;
         check( itr == secondary_index.end(), "idx128_general - secondary key sort" );
      }

   }

   template<uint64_t TableName, uint64_t SecondaryIndex>
   auto idx64_table( name receiver )
   {
      typedef record_idx64 record;
      // Load table using multi_index
      multi_index<name{TableName}, record,
                  indexed_by<name{SecondaryIndex}, const_mem_fun<record, uint64_t, &record::get_secondary>>
                  > table( receiver, receiver.value );
      return table;
   }

} /// _test_multi_index

void test_api_multi_index::idx64_store_only()
{
   _test_multi_index::idx64_store_only<"indextable1"_n.value>( get_self() );
}

void test_api_multi_index::idx64_check_without_storing()
{
   _test_multi_index::idx64_check_without_storing<"indextable1"_n.value>( get_self() );
}

void test_api_multi_index::idx64_general()
{
   _test_multi_index::idx64_store_only<"indextable2"_n.value>( get_self() );
   _test_multi_index::idx64_check_without_storing<"indextable2"_n.value>( get_self() );
}

void test_api_multi_index::idx128_store_only()
{
   _test_multi_index::idx128_store_only<"indextable3"_n.value>( get_self() );
}

void test_api_multi_index::idx128_check_without_storing()
{
   _test_multi_index::idx128_check_without_storing<"indextable3"_n.value>( get_self() );
}

void test_api_multi_index::idx128_general()
{
   _test_multi_index::idx128_store_only<"indextable4"_n.value>( get_self() );
   _test_multi_index::idx128_check_without_storing<"indextable4"_n.value>( get_self() );
}

void test_api_multi_index::idx64_require_find_fail()
{
   _test_multi_index::idx64_store_only<"indextable5"_n.value>( get_self() );
   _test_multi_index::idx64_require_find_fail<"indextable5"_n.value>( get_self() );
}

void test_api_multi_index::idx64_require_find_fail_with_msg()
{
   _test_multi_index::idx64_store_only<"indextablea"_n.value>( get_self() ); // Making the name smaller fixes this?
   _test_multi_index::idx64_require_find_fail_with_msg<"indextablea"_n.value>( get_self() ); // Making the name smaller fixes this?
}

void test_api_multi_index::idx64_require_find_sk_fail()
{
   _test_multi_index::idx64_store_only<"indextableb"_n.value>( get_self() );
   _test_multi_index::idx64_require_find_sk_fail<"indextableb"_n.value>( get_self() );
}

void test_api_multi_index::idx64_require_find_sk_fail_with_msg()
{
   _test_multi_index::idx64_store_only<"indextablec"_n.value>( get_self() );
   _test_multi_index::idx64_require_find_sk_fail_with_msg<"indextablec"_n.value>( get_self() );
}

void test_api_multi_index::idx128_autoincrement_test()
{
   using namespace _test_multi_index;

   typedef record_idx128 record;

   auto payer = get_self();

   multi_index<"autoinctbl1"_n, record,
      indexed_by<"bysecondary"_n, const_mem_fun<record, uint128_t, &record::get_secondary>>
   > table( get_self(), get_self().value );

   for( int i = 0; i < 5; ++i ) {
      table.emplace( payer, [&](auto& r) {
         r.id = table.available_primary_key();
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   uint64_t expected_key = 4;
   for( const auto& r : table.get_index<"bysecondary"_n>() )
   {
      check( r.primary_key() == expected_key, "idx128_autoincrement_test - unexpected primary key" );
      --expected_key;
   }
   check( expected_key == static_cast<uint64_t>(-1), "idx128_autoincrement_test - did not iterate through secondary index properly" );

   auto itr = table.find(3);
   check( itr != table.end(), "idx128_autoincrement_test - could not find object with primary key of 3" );

   // The modification below would trigger an error:
   /*
   table.modify(itr, payer, [&](auto& r) {
      r.id = 100;
   });
   */

   table.emplace( payer, [&](auto& r) {
      r.id  = 100;
      r.sec = itr->sec;
   });
   table.erase(itr);

   check( table.available_primary_key() == 101, "idx128_autoincrement_test - next_primary_key was not correct after record modify" );
}

void test_api_multi_index::idx128_autoincrement_test_part1()
{
   using namespace _test_multi_index;

   typedef record_idx128 record;

   auto payer = get_self();

   multi_index<"autoinctbl2"_n, record,
      indexed_by<"bysecondary"_n, const_mem_fun<record, uint128_t, &record::get_secondary>>
   > table( get_self(), get_self().value );

   for( int i = 0; i < 3; ++i ) {
      table.emplace( payer, [&](auto& r) {
         r.id = table.available_primary_key();
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   table.erase(table.get(0));

   uint64_t expected_key = 2;
   for( const auto& r : table.get_index<"bysecondary"_n>() )
   {
      check( r.primary_key() == expected_key, "idx128_autoincrement_test_part1 - unexpected primary key" );
      --expected_key;
   }
   check( expected_key == 0, "idx128_autoincrement_test_part1 - did not iterate through secondary index properly" );

}

void test_api_multi_index::idx128_autoincrement_test_part2()
{
   using namespace _test_multi_index;

   typedef record_idx128 record;

   const name::raw table_name = "autoinctbl2"_n;
   auto payer = get_self();

   {
      multi_index<table_name, record,
         indexed_by<"bysecondary"_n, const_mem_fun<record, uint128_t, &record::get_secondary>>
      > table( get_self(), get_self().value );

      check( table.available_primary_key() == 3, "idx128_autoincrement_test_part2 - did not recover expected next primary key" );
   }

   multi_index<table_name, record,
      indexed_by<"bysecondary"_n, const_mem_fun<record, uint128_t, &record::get_secondary>>
   > table( get_self(), get_self().value );

   table.emplace( payer, [&](auto& r) {
      r.id = 0;
      r.sec = 1000;
   });
   // Done this way to make sure that table._next_primary_key is not incorrectly set to 1.

   for( int i = 3; i < 5; ++i ) {
      table.emplace( payer, [&](auto& r) {
         auto itr = table.available_primary_key();
         r.id = itr;
         r.sec = 1000 - static_cast<uint128_t>(r.id);
      });
   }

   uint64_t expected_key = 4;
   for( const auto& r : table.get_index<"bysecondary"_n>() )
   {
      check( r.primary_key() == expected_key, "idx128_autoincrement_test_part2 - unexpected primary key" );
      --expected_key;
   }
   check( expected_key == static_cast<uint64_t>(-1), "idx128_autoincrement_test_part2 - did not iterate through secondary index properly" );

   auto itr = table.find(3);
   check( itr != table.end(), "idx128_autoincrement_test_part2 - could not find object with primary key of 3" );

   table.emplace( payer, [&](auto& r) {
      r.id  = 100;
      r.sec = itr->sec;
   });
   table.erase(itr);

   check( table.available_primary_key() == 101, "idx128_autoincrement_test_part2 - next_primary_key was not correct after record update" );
}

void test_api_multi_index::idx256_general()
{
   using namespace _test_multi_index;

   typedef record_idx256 record;

   auto payer = get_self();

   print("Testing checksum256 secondary index.\n");
   multi_index<"indextable5"_n, record,
      indexed_by<"bysecondary"_n, const_mem_fun<record, const checksum256&, &record::get_secondary>>
   > table( get_self(), get_self().value );

   auto fourtytwo       = checksum256::make_from_word_sequence<uint64_t>( 0ULL, 0ULL, 0ULL, 42ULL );
   //auto onetwothreefour = checksum256::make_from_word_sequence<uint64_t>(1ULL, 2ULL, 3ULL, 4ULL);
   auto onetwothreefour = checksum256{std::array<uint32_t, 8>{ {0,1, 0,2, 0,3, 0,4} }};

   table.emplace( payer, [&](auto& o) {
      o.id = 1;
      o.sec = fourtytwo;
   });

   table.emplace( payer, [&](auto& o) {
      o.id = 2;
      o.sec = onetwothreefour;
   });

   table.emplace( payer, [&](auto& o) {
      o.id = 3;
      o.sec = fourtytwo;
   });

   auto e = table.find(2);

   print("Items sorted by primary key:\n");
   for( const auto& item : table ) {
      print(" ID=", item.primary_key(), ", secondary=", item.sec, "\n");
   }

   {
      auto itr = table.begin();
      check( itr->primary_key() == 1 && itr->get_secondary() == fourtytwo, "idx256_general - primary key sort" );
      ++itr;
      check( itr->primary_key() == 2 && itr->get_secondary() == onetwothreefour, "idx256_general - primary key sort" );
      ++itr;
      check( itr->primary_key() == 3 && itr->get_secondary() == fourtytwo, "idx256_general - primary key sort" );
      ++itr;
      check( itr == table.end(), "idx256_general - primary key sort" );
   }

   auto secidx = table.get_index<"bysecondary"_n>();

   auto lower1 = secidx.lower_bound( checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 40ULL) );
   print("First entry with a secondary key of at least 40 has ID=", lower1->id, ".\n");
   check( lower1->id == 1, "idx256_general - lower_bound" );

   auto lower2 = secidx.lower_bound( checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 50ULL) );
   print("First entry with a secondary key of at least 50 has ID=", lower2->id, ".\n");
   check( lower2->id == 2, "idx256_general - lower_bound" );

   if( table.iterator_to(*lower2) == e ) {
      print("Previously found entry is the same as the one found earlier with a primary key value of 2.\n");
   }

   print("Items sorted by secondary key (checksum256):\n");
   for( const auto& item : secidx ) {
      print(" ID=", item.primary_key(), ", secondary=", item.sec, "\n");
   }

   {
      auto itr = secidx.begin();
      check( itr->primary_key() == 1, "idx256_general - secondary key sort" );
      ++itr;
      check( itr->primary_key() == 3, "idx256_general - secondary key sort" );
      ++itr;
      check( itr->primary_key() == 2, "idx256_general - secondary key sort" );
      ++itr;
      check( itr == secidx.end(), "idx256_general - secondary key sort" );
   }

   auto upper = secidx.upper_bound( checksum256{std::array<uint64_t,4>{{0, 0, 0, 42}}} );

   print("First entry with a secondary key greater than 42 has ID=", upper->id, ".\n");
   check( upper->id == 2, "idx256_general - upper_bound" );
   check( upper->id == secidx.get(onetwothreefour).id, "idx256_general - secondary index get" );

   print("Removed entry with ID=", lower1->id, ".\n");
   secidx.erase( lower1 );

   print("Items reverse sorted by primary key:\n");
   for( auto itr = table.rbegin(); itr != table.rend(); ++itr ) {
      const auto& item = *itr;
      print(" ID=", item.primary_key(), ", secondary=", item.sec, "\n");
   }

   {
      auto itr = table.rbegin();
      check( itr->primary_key() == 3 && itr->get_secondary() == fourtytwo, "idx256_general - primary key sort after remove" );
      ++itr;
      check( itr->primary_key() == 2 && itr->get_secondary() == onetwothreefour, "idx256_general - primary key sort after remove" );
      ++itr;
      check( itr == table.rend(), "idx256_general - primary key sort after remove" );
   }
}

void test_api_multi_index::idx_double_general()
{
   using namespace _test_multi_index;

   typedef record_idx_double record;

   auto payer = get_self();

   print("Testing double secondary index.\n");
   multi_index<"floattable1"_n, record,
      indexed_by<"bysecondary"_n, const_mem_fun<record, double, &record::get_secondary>>
   > table( get_self(), get_self().value );

   auto secidx = table.get_index<"bysecondary"_n>();

   double tolerance = std::numeric_limits<double>::epsilon();
   print("tolerance = ", tolerance, "\n");

   for( uint64_t i = 1; i <= 10; ++i ) {
      table.emplace( payer, [&]( auto& o ) {
         o.id = i;
         o.sec = 1.0 / (i * 1000000.0);
      });
   }

   double expected_product = 1.0 / 1000000.0;
   print( "expected_product = ", expected_product, "\n" );

   uint64_t expected_key = 10;
   for( const auto& obj : secidx ) {
      check( obj.primary_key() == expected_key, "idx_double_general - unexpected primary key" );

      double prod = obj.sec * obj.id;

      print(" id = ", obj.id, ", sec = ", obj.sec, ", sec * id = ", prod, "\n");

      check( std::abs(prod - expected_product) <= tolerance,
                    "idx_double_general - product of secondary and id not equal to expected_product within tolerance" );

      --expected_key;
   }
   check( expected_key == 0, "idx_double_general - did not iterate through secondary index properly" );

   {
      auto itr = secidx.lower_bound( expected_product / 5.5 );
      check( std::abs(1.0 / itr->sec - 5000000.0) <= tolerance, "idx_double_general - lower_bound" );

      itr = secidx.upper_bound( expected_product / 5.0 );
      check( std::abs(1.0 / itr->sec - 4000000.0) <= tolerance, "idx_double_general - upper_bound" );

   }
}

void test_api_multi_index::idx_long_double_general()
{
   using namespace _test_multi_index;

   typedef record_idx_long_double record;

   auto payer = get_self();

   print("Testing long double secondary index.\n");
   multi_index<"floattable2"_n, record,
      indexed_by<"bysecondary"_n, const_mem_fun<record, long double, &record::get_secondary>>
   > table( get_self(), get_self().value );

   auto secidx = table.get_index<"bysecondary"_n>();

   long double tolerance = std::min( static_cast<long double>(std::numeric_limits<double>::epsilon()),
                                     std::numeric_limits<long double>::epsilon() * 1e7l );
   print("tolerance = ", tolerance, "\n");

   long double f = 1.0l;
   for( uint64_t i = 1; i <= 10; ++i, f += 1.0l ) {
      table.emplace( payer, [&](auto& o) {
         o.id = i;
         o.sec = 1.0l / (i * 1000000.0l);
      });
   }

   long double expected_product = 1.0l / 1000000.0l;
   print( "expected_product = ", expected_product, "\n" );

   uint64_t expected_key = 10;
   for( const auto& obj : secidx ) {
      check( obj.primary_key() == expected_key, "idx_long_double_general - unexpected primary key" );

      long double prod = obj.sec * obj.id;

      print(" id = ", obj.id, ", sec = ", obj.sec, ", sec * id = ", prod, "\n");

      check( std::abs(prod - expected_product) <= tolerance,
                    "idx_long_double_general - product of secondary and id not equal to expected_product within tolerance" );

      --expected_key;
   }
   check( expected_key == 0, "idx_long_double_general - did not iterate through secondary index properly" );

   {
      auto itr = secidx.lower_bound( expected_product / 5.5l );
      check( std::abs(1.0l / itr->sec - 5000000.0l) <= tolerance, "idx_long_double_general - lower_bound" );

      itr = secidx.upper_bound( expected_product / 5.0l );
      check( std::abs(1.0l / itr->sec - 4000000.0l) <= tolerance, "idx_long_double_general - upper_bound" );

   }
}

void test_api_multi_index::idx64_pk_iterator_exceed_end()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto end_itr = table.end();
   // Should fail
   ++end_itr;
}

void test_api_multi_index::idx64_sk_iterator_exceed_end()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto end_itr = table.get_index<"bysecondary"_n>().end();
   // Should fail
   ++end_itr;
}

void test_api_multi_index::idx64_pk_iterator_exceed_begin()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto begin_itr = table.begin();
   // Should fail
   --begin_itr;
}

void test_api_multi_index::idx64_sk_iterator_exceed_begin()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto begin_itr = table.get_index<"bysecondary"_n>().begin();
   // Should fail
   --begin_itr;
}

void test_api_multi_index::idx64_pass_pk_ref_to_other_table()
{
   auto table1 = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto table2 = _test_multi_index::idx64_table<"indextable2"_n.value, "bysecondary"_n.value>( get_self() );

   auto table1_pk_itr = table1.find(781);
   check( table1_pk_itr != table1.end() && table1_pk_itr->sec == "bob"_n.value, "idx64_pass_pk_ref_to_other_table - table.find() of existing primary key" );

   // Should fail
   table2.iterator_to(*table1_pk_itr);
}

void test_api_multi_index::idx64_pass_sk_ref_to_other_table()
{
   auto table1 = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto table2 = _test_multi_index::idx64_table<"indextable2"_n.value, "bysecondary"_n.value>( get_self() );

   auto table1_pk_itr = table1.find(781);
   check( table1_pk_itr != table1.end() && table1_pk_itr->sec == "bob"_n.value, "idx64_pass_sk_ref_to_other_table - table.find() of existing primary key" );

   auto table2_sec_index = table2.get_index<"bysecondary"_n>();
   // Should fail
   table2_sec_index.iterator_to(*table1_pk_itr);
}

void test_api_multi_index::idx64_pass_pk_end_itr_to_iterator_to()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto end_itr = table.end();
   // Should fail
   table.iterator_to(*end_itr);
}

void test_api_multi_index::idx64_pass_pk_end_itr_to_modify()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto end_itr = table.end();

   // Should fail
   table.modify( end_itr, get_self(), [](auto&){} );
}

void test_api_multi_index::idx64_pass_pk_end_itr_to_erase()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto end_itr = table.end();

   // Should fail
   table.erase(end_itr);
}

void test_api_multi_index::idx64_pass_sk_end_itr_to_iterator_to()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto sec_index = table.get_index<"bysecondary"_n>();
   auto end_itr = sec_index.end();

   // Should fail
   sec_index.iterator_to(*end_itr);
}

void test_api_multi_index::idx64_pass_sk_end_itr_to_modify()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto sec_index = table.get_index<"bysecondary"_n>();
   auto end_itr = sec_index.end();

   // Should fail
   sec_index.modify( end_itr, get_self(), [](auto&){} );
}


void test_api_multi_index::idx64_pass_sk_end_itr_to_erase()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );
   auto sec_index = table.get_index<"bysecondary"_n>();
   auto end_itr = sec_index.end();

   // Should fail
   sec_index.erase(end_itr);
}

void test_api_multi_index::idx64_modify_primary_key()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );

   auto pk_itr = table.find(781);
   check( pk_itr != table.end() && pk_itr->sec == "bob"_n.value, "idx64_modify_primary_key - table.find() of existing primary key" );

   // Should fail
   table.modify( pk_itr, get_self(), [](auto& r){
      r.id = 1100;
   });
}

void test_api_multi_index::idx64_run_out_of_avl_pk()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );

   auto pk_itr = table.find(781);
   check( pk_itr != table.end() && pk_itr->sec == "bob"_n.value, "idx64_modify_primary_key - table.find() of existing primary key" );

   auto payer = get_self();

   table.emplace( payer, [&](auto& r) {
      r.id = static_cast<uint64_t>(-4);
      r.sec = "alice"_n.value;
   });
   check( table.available_primary_key() == static_cast<uint64_t>(-3), "idx64_run_out_of_avl_pk - incorrect available primary key" );

   table.emplace( payer, [&](auto& r) {
      r.id = table.available_primary_key();
      r.sec = "bob"_n.value;
   });

   // Should fail
   table.available_primary_key();
}

void test_api_multi_index::idx64_sk_cache_pk_lookup()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );

   auto sec_index = table.get_index<"bysecondary"_n>();
   auto sk_itr = sec_index.find("bob"_n.value);
   check( sk_itr != sec_index.end() && sk_itr->id == 540, "idx64_sk_cache_pk_lookup - sec_index.find() of existing secondary key" );

   auto pk_itr = table.iterator_to(*sk_itr);
   auto prev_itr = --pk_itr;
   check( prev_itr->id == 265 && prev_itr->sec == "alice"_n.value, "idx64_sk_cache_pk_lookup - previous record" );
}

void test_api_multi_index::idx64_pk_cache_sk_lookup()
{
   auto table = _test_multi_index::idx64_table<"indextable1"_n.value, "bysecondary"_n.value>( get_self() );


   auto pk_itr = table.find(540);
   check( pk_itr != table.end() && pk_itr->sec == "bob"_n.value, "idx64_pk_cache_sk_lookup - table.find() of existing primary key" );

   auto sec_index = table.get_index<"bysecondary"_n>();
   auto sk_itr = sec_index.iterator_to(*pk_itr);
   auto next_itr = ++sk_itr;
   check( next_itr->id == 781 && next_itr->sec == "bob"_n.value, "idx64_pk_cache_sk_lookup - next record" );
}
