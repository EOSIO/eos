#include <eosiolib/multi_index.hpp>

using namespace eosio;


struct limit_order {
   uint64_t     id;
   uint128_t    price;
   uint64_t     expiration;
   account_name owner;

   auto primary_key()const { return id; }
   uint64_t get_expiration()const { return expiration; }
   uint128_t get_price()const { return price; }

   EOSLIB_SERIALIZE( limit_order, (id)(price)(expiration)(owner) )
};

extern "C" {
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t code, uint64_t action ) {
      eosio::multi_index<N(orders), limit_order,
         index_by<0, N(byexp), limit_order, const_mem_fun<limit_order, uint64_t, &limit_order::get_expiration> >,
         index_by<1, N(byprice), limit_order, const_mem_fun<limit_order, uint128_t, &limit_order::get_price> >
         > orders( N(exchange), N(exchange) );

      auto payer = code;
      const auto& order = orders.emplace( payer, [&]( auto& o ) {
        o.id = 1;
        o.expiration = 3;
        o.owner = N(dan);
      });

      orders.update( order, payer, [&]( auto& o ) {
      });

      const auto* o = orders.find( 1 );

      orders.remove( order );

      auto expidx = orders.get_index<N(byexp)>();

      for( const auto& item : orders ) {
      }

      for( const auto& item : expidx ) {
      }

      auto lower = expidx.lower_bound(4);

   }
}
