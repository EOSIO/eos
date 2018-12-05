/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>

using namespace eosio;

namespace multi_index_test {

   CONTRACT snapshot_test : public contract {
   public:
      using contract::contract;

      struct limit_order {
         uint64_t  id;
         uint64_t  padding = 0;
         uint128_t price;
         uint64_t  expiration;
         name      owner;

         auto primary_key()const { return id; }         
         uint64_t get_expiration()const { return expiration; }
         uint128_t get_price()const { return price; }

         EOSLIB_SERIALIZE( limit_order, (id)(price)(expiration)(owner) )
      };

      struct test_k256 {
         uint64_t    id;
         checksum256 val;

         auto primary_key()const { return id; }
         checksum256 get_val()const { return val; }

         EOSLIB_SERIALIZE( test_k256, (id)(val) )
      };

      struct trigger {
         trigger( uint32_t w = 0 )
            : what(w)
         {}
         
         uint32_t what;
      };

      ACTION multitest( uint32_t what ) {
         trigger t( what );

         on( t, _self );
      }

      static void on( const trigger& act, name _payer )
      {
         name payer = _payer;
         print("Acting on trigger action.\n");
         switch(act.what)
         {
             case 0:
             {
                print("Testing uint128_t secondary index.\n");
                eosio::multi_index<"orders"_n, limit_order,
                                   indexed_by< "byexp"_n,   const_mem_fun<limit_order, uint64_t,  &limit_order::get_expiration> >,
                                   indexed_by< "byprice"_n, const_mem_fun<limit_order, uint128_t, &limit_order::get_price> >
                                   > orders( name{"multitest"}, name{"multitest"}.value );

                orders.emplace( payer, [&]( auto& o ) {
                                          o.id = 1;
                                          o.expiration = 300;
                                          o.owner = "dan"_n; });

                auto order2 = orders.emplace( payer, [&]( auto& o ) {
                                                        o.id = 2;
                                                        o.expiration = 200;
                                                        o.owner = "alice"_n; });

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
                                                 o.expiration = 400; });

                print("Items sorted by expiration:\n");
                for( const auto& item : expidx ) {
                   print(" ID=", item.id, ", expiration=", item.expiration, ", owner=", name{item.owner}, "\n");
                }

                auto lower = expidx.lower_bound( 100 );

                print("First order with an expiration of at least 100 has ID=", lower->id, " and expiration=", lower->get_expiration(), "\n");

             }
             break;
             case 1: // Test key265 secondary index
             {
                print("Testing key256 secondary index.\n");
                eosio::multi_index<"test1"_n, test_k256,
                                   indexed_by< "byval"_n, const_mem_fun<test_k256, checksum256, &test_k256::get_val> >
                                   > testtable( "multitest"_n, "exchange"_n.value ); // Code must be same as the receiver? Scope doesn't have to be.

                testtable.emplace( payer, [&]( auto& o ) {
                                             o.id = 1;
                                             o.val = checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL); });

                testtable.emplace( payer, [&]( auto& o ) {
                                             o.id = 2;
                                             o.val = checksum256::make_from_word_sequence<uint64_t>(1ULL, 2ULL, 3ULL, 4ULL); });

                testtable.emplace( payer, [&]( auto& o ) {
                                             o.id = 3;
                                             o.val = checksum256::make_from_word_sequence<uint64_t>(0ULL, 0ULL, 0ULL, 42ULL); });

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
             break;
             default:
                eosio_assert( 0, "Given what code is not supported." );
                break;
         }
      }
   };
} /// multi_index_test

namespace multi_index_test {
   extern "C" {
      /// The apply method implements the dispatch of events to this contract
      void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
         require_auth( code );
         if( code == receiver ) {
            if( action == "multitest"_n.value ) {
               size_t size = action_data_size();
               void* buffer = nullptr;
               buffer = alloca( 512 );
               read_action_data( buffer, size );
               datastream<const char*> ds( (char*)buffer, size );
               
               uint32_t w;
               ds >> w;
                    
               snapshot_test test( name{receiver}, name{code}, ds );
               test.multitest( w );
            }
            else {
               eosio_assert( false, "Could not dispatch" );
            }
         }
      }
   }
}
