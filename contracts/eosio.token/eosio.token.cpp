/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.token/eosio.token.hpp>

namespace eosio {

void token::create( account_name issuer,
                    asset        maximum_supply,
                    uint8_t      issuer_can_freeze,
                    uint8_t      issuer_can_recall,  
                    uint8_t      issuer_can_whitelist ) 
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );

    stats statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
       s.can_freeze    = issuer_can_freeze;
       s.can_recall    = issuer_can_recall;
       s.can_whitelist = issuer_can_whitelist;
    });
}


void token::issue( account_name to, asset quantity, string memo ) 
{
    print( "issue" );
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply.amount += quantity.amount;
    });

    add_balance( st.issuer, quantity, st, st.issuer );

    if( to != st.issuer )
    {
       dispatch_inline( permission_level{st.issuer,N(active)}, _self, N(transfer), &token::transfer, { st.issuer, to, quantity, memo } );
    }
}

void token::transfer( account_name from, 
                      account_name to,
                      asset        quantity,
                      string       /*memo*/ ) 
{
    print( "transfer" );
    require_auth( from );
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );

    sub_balance( from, quantity, st );
    add_balance( to, quantity, st, from );
}


void token::sub_balance( account_name owner, asset value, const currency_stats& st ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   if( has_auth( owner ) ) {
      eosio_assert( !st.can_freeze || !from.frozen, "account is frozen by issuer" );
      eosio_assert( !st.can_freeze || !st.is_frozen, "all transfers are frozen by issuer" );
      eosio_assert( !st.enforce_whitelist || from.whitelist, "account is not white listed" );
   } else if( has_auth( st.issuer ) ) {
      eosio_assert( st.can_recall, "issuer may not recall token" );
   } else {
      eosio_assert( false, "insufficient authority" );
   }

   from_acnts.modify( from, owner, [&]( auto& a ) {
       a.balance.amount -= value.amount;
   });
}

void token::add_balance( account_name owner, asset value, const currency_stats& st, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol );
   if( to == to_acnts.end() ) {
      eosio_assert( !st.enforce_whitelist, "can only transfer to white listed accounts" );
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      eosio_assert( !st.enforce_whitelist || to->whitelist, "receiver requires whitelist by issuer" );
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance.amount += value.amount;
      });
   }
}

} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer) )
