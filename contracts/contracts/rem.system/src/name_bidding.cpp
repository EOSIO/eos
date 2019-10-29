#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>

#include <eosio/transaction.hpp>

namespace eosiosystem {

   using eosio::current_time_point;
   using eosio::token;

   void system_contract::bidname( const name& bidder, const name& newname, const asset& bid ) {
      check( false, "name auction is not supported" );

      require_auth( bidder );
      check( newname.suffix() == newname, "you can only bid on top-level suffix" );

      check( (bool)newname, "the empty name is not a valid account name to bid on" );
      check( (newname.value & 0xFull) == 0, "13 character names are not valid account names to bid on" );
      check( (newname.value & 0x1F0ull) == 0, "accounts with 12 character names and no dots can be created without bidding required" );
      check( !is_account( newname ), "account already exists" );
      check( bid.symbol == core_symbol(), "asset must be system token" );
      check( bid.amount > 0, "insufficient bid" );
      token::transfer_action transfer_act{ token_account, { {bidder, active_permission} } };
      transfer_act.send( bidder, names_account, bid, std::string("bid name ")+ newname.to_string() );
      name_bid_table bids(get_self(), get_self().value);
      print( name{bidder}, " bid ", bid, " on ", name{newname}, "\n" );
      auto current = bids.find( newname.value );
      if( current == bids.end() ) {
         bids.emplace( bidder, [&]( auto& b ) {
            b.newname = newname;
            b.high_bidder = bidder;
            b.high_bid = bid.amount;
            b.last_bid_time = current_time_point();
         });
      } else {
         check( current->high_bid > 0, "this auction has already closed" );
         check( bid.amount - current->high_bid > (current->high_bid / 10), "must increase bid by 10%" );
         check( current->high_bidder != bidder, "account is already highest bidder" );

         bid_refund_table refunds_table(get_self(), newname.value);

         auto it = refunds_table.find( current->high_bidder.value );
         if ( it != refunds_table.end() ) {
            refunds_table.modify( it, same_payer, [&](auto& r) {
                  r.amount += asset( current->high_bid, core_symbol() );
               });
         } else {
            refunds_table.emplace( bidder, [&](auto& r) {
                  r.bidder = current->high_bidder;
                  r.amount = asset( current->high_bid, core_symbol() );
               });
         }

         eosio::transaction t;
         t.actions.emplace_back( permission_level{get_self(), active_permission},
                                 get_self(), "bidrefund"_n,
                                 std::make_tuple( current->high_bidder, newname )
         );
         t.delay_sec = 0;
         uint128_t deferred_id = (uint128_t(newname.value) << 64) | current->high_bidder.value;
         eosio::cancel_deferred( deferred_id );
         t.send( deferred_id, bidder );

         bids.modify( current, bidder, [&]( auto& b ) {
            b.high_bidder = bidder;
            b.high_bid = bid.amount;
            b.last_bid_time = current_time_point();
         });
      }
   }

   void system_contract::bidrefund( const name& bidder, const name& newname ) {
      check( false, "name auction is not supported" );

      bid_refund_table refunds_table(get_self(), newname.value);
      auto it = refunds_table.find( bidder.value );
      check( it != refunds_table.end(), "refund not found" );

      token::transfer_action transfer_act{ token_account, { {names_account, active_permission}, {bidder, active_permission} } };
      transfer_act.send( names_account, bidder, asset(it->amount), std::string("refund bid on name ")+(name{newname}).to_string() );
      refunds_table.erase( it );
   }

}
