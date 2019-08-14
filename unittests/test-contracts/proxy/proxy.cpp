#include "proxy.hpp"
#include <eosio/transaction.hpp>

using namespace eosio;

proxy::proxy( eosio::name self, eosio::name first_receiver, eosio::datastream<const char*> ds )
:contract( self, first_receiver, ds )
,_config( get_self(), get_self().value )
{}

void proxy::setowner( name owner, uint32_t delay ) {
   require_auth( get_self() );
   auto cfg = _config.get_or_default();
   cfg.owner = owner;
   cfg.delay = delay;
   print( "Setting owner to ", owner, " with delay ", delay, "\n" );
   _config.set( cfg, get_self() );
}

void proxy::on_transfer( name from, name to, asset quantity, const std::string& memo ) {
   print("on_transfer called on ", get_self(), " contract with from = ", from, " and to = ", to, "\n" );
   check( _config.exists(), "Attempting to use unconfigured proxy" );
   auto cfg = _config.get();

   auto self = get_self();
   if( from == self ) {
      check( to == cfg.owner, "proxy may only pay its owner" );
   } else {
      check( to == self, "proxy is not involved in this transfer" );

      auto id =  cfg.next_id++;
      _config.set( cfg, self );

      transaction out;
      eosio::token::transfer_action a( "eosio.token"_n, {self, "active"_n} );
      out.actions.emplace_back( a.to_action( self, cfg.owner, quantity, memo ) );
      out.delay_sec = cfg.delay;
      out.send( id, self );
   }
}

void proxy::on_error( uint128_t sender_id, eosio::ignore<std::vector<char>> ) {
   print( "on_error called on ", get_self(), " contract with sender_id = ", sender_id, "\n" );
   check( _config.exists(), "Attempting use of unconfigured proxy" );

   auto cfg = _config.get();

   auto id = cfg.next_id;
   ++cfg.next_id;
   _config.set( cfg, same_payer );

   print("Resending Transaction: ", sender_id, " as ", id, "\n");

   unsigned_int packed_trx_size;
   get_datastream() >> packed_trx_size;
   transaction trx;
   get_datastream() >> trx;
   trx.transaction_extensions.clear();

   trx.delay_sec = cfg.delay;
   trx.send( id, get_self() );
}
