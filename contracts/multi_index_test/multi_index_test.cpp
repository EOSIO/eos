#include <eosiolib/multi_index.hpp>

using namespace eosio;


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

extern "C" {
   /// The apply method implements the dispatch of events to this contract
   void apply( uint64_t code, uint64_t action ) {
      eosio::multi_index<N(orders), limit_order,
         indexed_by<N(byexp), const_mem_fun<limit_order, uint64_t, &limit_order::get_expiration> >,
         indexed_by<N(byprice), const_mem_fun<limit_order, uint128_t, &limit_order::get_price> >
         > orders( N(exchange), N(exchange) );

      auto payer = code;
      const auto& order = orders.emplace( payer, [&]( auto& o ) {
        o.id = 1;
        o.expiration = 3;
        o.owner = N(dan);
      });

      orders.update( order, payer, [&]( auto& o ) {
         o.expiration = 4;
      });

      const auto* o = orders.find( 1 );

      orders.remove( order );

      auto expidx = orders.get_index<N(byexp)>();

      for( const auto& item : orders ) {
         (void)item.id;
      }

      for( const auto& item : expidx ) {
         (void)item.id;
      }

      auto lower = expidx.lower_bound(4);

   }
}
