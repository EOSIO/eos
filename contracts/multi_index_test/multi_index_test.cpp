#include <eosiolib/eosio.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/multi_index.hpp>

using namespace eosio;

namespace multi_index_test {

struct limit_order {
   uint64_t     id;
   uint64_t     padding = 0;
   uint128_t    price;
   uint64_t     expiration;
   account_name owner;

      auto primary_key()const { return id; }
      uint64_t get_expiration()const { return expiration; }
      uint128_t get_price()const { return price; }

      EOSLIB_SERIALIZE( limit_order, (id)(price)(expiration)(owner) )
   };

   struct test_k256 {
      uint64_t     id;
      key256      val;

      auto primary_key()const { return id; }
      key256 get_val()const { return val; }

      EOSLIB_SERIALIZE( test_k256, (id)(val) )
   };

   class multi_index_test {
      public:

         ACTION(N(multitest), trigger) {
            trigger(): what(0) {}
            trigger(uint32_t w): what(w) {}

            uint32_t what;

            EOSLIB_SERIALIZE(trigger, (what))
         };

         static void on(const trigger& act)
         {
            auto payer = act.get_account();
            print("Acting on trigger action.\n");
            switch(act.what)
            {
               case 0:
               {
                  print("Testing uint128_t secondary index.\n");
                  eosio::multi_index<N(orders), limit_order,
                     indexed_by< N(byexp),   const_mem_fun<limit_order, uint64_t, &limit_order::get_expiration> >,
                     indexed_by< N(byprice), const_mem_fun<limit_order, uint128_t, &limit_order::get_price> >
                     > orders( N(multitest), N(multitest) );

                  const auto& order1 = orders.emplace( payer, [&]( auto& o ) {
                    o.id = 1;
                    o.expiration = 300;
                    o.owner = N(dan);
                  });

                  const auto& order2 = orders.emplace( payer, [&]( auto& o ) {
                     o.id = 2;
                     o.expiration = 200;
                     o.owner = N(alice);
                  });

                  print("Items sorted by primary key:\n");
                  for( const auto& item : orders ) {
                     print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name(item.owner), "\n");
                  }

                  auto expidx = orders.get_index<N(byexp)>();

                  print("Items sorted by expiration:\n");
                  for( const auto& item : expidx ) {
                     print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name(item.owner), "\n");
                  }

                  print("Updating expiration of order with ID=2 to 400.\n");
                  orders.update( order2, payer, [&]( auto& o ) {
                     o.expiration = 400;
                  });

                  print("Items sorted by expiration:\n");
                  for( const auto& item : expidx ) {
                     print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name(item.owner), "\n");
                  }

                  auto lower = expidx.lower_bound(100);

                  print("First order with an expiration of at least 100 has ID=", lower->id, " and expiration=", lower->get_expiration(), "\n");

               }
               break;
               case 1: // Test key265 secondary index
               {
                  print("Testing key256 secondary index.\n");
                  eosio::multi_index<N(test1), test_k256,
                     indexed_by< N(byval), const_mem_fun<test_k256, key256, &test_k256::get_val> >
                  > testtable( N(multitest), N(exchange) ); // Code must be same as the receiver? Scope doesn't have to be.

                  const auto& entry1 = testtable.emplace( payer, [&]( auto& o ) {
                     o.id = 1;
                     o.val = key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL);
                  });

                  const auto& entry2 = testtable.emplace( payer, [&]( auto& o ) {
                     o.id = 2;
                     o.val = key256::make_from_word_sequence<uint64_t>(1ULL, 2ULL, 3ULL, 4ULL);
                  });

                  const auto& entry3 = testtable.emplace( payer, [&]( auto& o ) {
                     o.id = 3;
                     o.val = key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL);
                  });

                  const auto* e = testtable.find( 2 );

                  print("Items sorted by primary key:\n");
                  for( const auto& item : testtable ) {
                     print(" ID=", item.primary_key(), ", val=", item.val, "\n");
                  }

                  auto validx = testtable.get_index<N(byval)>();

                  auto lower1 = validx.lower_bound(key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 40ULL));
                  print("First entry with a val of at least 40 has ID=", lower1->id, ".\n");

                  auto lower2 = validx.lower_bound(key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 50ULL));
                  print("First entry with a val of at least 50 has ID=", lower2->id, ".\n");

                  if( &*lower2 == e ) {
                     print("Previously found entry is the same as the one found earlier with a primary key value of 2.\n");
                  }

                  print("Items sorted by val (secondary key):\n");
                  for( const auto& item : validx ) {
                     print(" ID=", item.primary_key(), ", val=");
                     cout << item.val << "\n";
                  }

                  auto upper = validx.upper_bound(key256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL));

                  print("First entry with a val greater than 42 has ID=", upper->id, ".\n");

                  print("Removed entry with ID=", lower1->id, ".\n");
                  testtable.remove( *lower1 );

                  print("Items sorted by primary key:\n");
                  for( const auto& item : testtable ) {
                     print(" ID=", item.primary_key(), ", val=");
                     cout << item.val << "\n";
                  }

               }
               break;
               default:
                  eosio_assert(0, "Given what code is not supported.");
               break;
            }
         }
   };

} /// multi_index_test

namespace multi_index_test {
   extern "C" {
      /// The apply method implements the dispatch of events to this contract
      void apply( uint64_t code, uint64_t action ) {
         eosio_assert(eosio::dispatch<multi_index_test, multi_index_test::trigger>(code, action),
                      "Could not dispatch");
      }
   }
}
