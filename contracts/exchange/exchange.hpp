/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/generic_currency.hpp>
#include <eosiolib/multi_index.hpp>

using eosio::asset;
using eosio::symbol_name;
using eosio::indexed_by;
using eosio::const_mem_fun;
using eosio::price_ratio;
using eosio::price;



struct global_margin_state {
   extended_asset lent; /// the total quantity of tokens lent for this collateral type.
   extended_asset collateral; /// lent * call_price => amount of collateral that must be able to buy back lent
   double         call_price = 0; ///< the least collateralized position
   uint16_t       interest_rate = 0; ///< annual interest rate as percent where 10000 = 1% and 1 = .00001% 

   EOSLIB_SERIALIZE( global_margin_state, (lent)(collateral)(call_price)(interest_rate) )
};

struct exchange_state {
   symbol_name  symbol;
   int64_t      supply = 0; /// balance for purpose of market making

   struct connector {
      account_name issuer;
      symbol_name  symbol;
      int64_t      balance; /// balance for purpose of market making
      int64_t      lendable; ///< total funds available for lending
      int64_t      lent; ///< total actually lent;
      int64_t      interest_accumulated; ///< total interest accumulated
      int128_t     interest_shares; ///< total shares in the accumulated interest, works like VSHARES on steem
   };

   connector base;
   connector quote;
   global_margin_state margin_state[6];

   EOSLIB_SERIALIZE( exchange_state, (symbol)(supply)(base)(quote)(margin_state) )
};


/**
 *  This is used to log transient side effects of temporary conversions
 */
struct pending_state : exchange_state 
{
   struct balance_key {
      account_name owner;
      account_name issuer;
      symbol_type  symbol;

      friend bool operator < ( const balance_key& a, const balance_key& b ) {
         return std::tie( a.owner, a.issuer, a.symbol ) < std::tie( b.owner, b.issuer, b.symbol );
      }
      friend bool operator == ( const balance_key& a, const balance_key& b ) {
         return std::tie( a.owner, a.issuer, a.symbol ) == std::tie( b.owner, b.issuer, b.symbol );
      }
   };

   asset convert_to_exchange( connector& con, const extended_asset& input );
   asset convert_from_exchange( connector& con, const extended_asset& input );

   void transfer( account_name user, extended_asset q ) {
      output[balance_key{user,q.symbol}] += q.amount;
   }

   map<balance_key, token_type> output; 
};



//template<account_name ExchangeAccount, symbol_name ExchangeSymbol>
class exchange {
   public:
      struct account {
         account_name       owner;
         extended_asset     base_balance;
         extended_asset     quote_balance;
         extended_asset     ex_balance;

         uint64_t primary_key()const { return owner; }

         EOSLIB_SERIALIZE( account, (owner)(base_balance)(quote_balance)  )
      };
      typedef eosio::multi_index< N(accounts), account>       account_index_type;

      struct limit_order {
         typedef eosio::price<BaseTokenType, QuoteTokenType> price_type;

         uint64_t                                   primary;
         account_name                               owner;
         uint32_t                                   id;
         uint32_t                                   expiration;
         extended_asset                             for_sale;
         extended_price                             price;

         uint64_t primary_key()const { return primary; }

         uint128_t by_owner_id()const   { return get_owner_id( owner, id ); }
         uint64_t  by_expiration()const { return expiration; }
         double    by_price()const      { return min_to_receive.amount / double(for_sale.amount); }

         static uint128_t get_owner_id( account_name owner, uint32_t id ) { return (uint128_t( owner ) << 64) | id; }

         EOSLIB_SERIALIZE( limit_order, (primary)(owner)(id)(expiration)(for_sale)(sell_price) )
      };

      typedef limit_order<base_token_type, quote_token_type> limit_base_quote;
      typedef eosio::multi_index< N(sellbq), limit_base_quote,
              indexed_by< N(price),   const_mem_fun<limit_base_quote, uint128_t, &limit_base_quote::by_price > >,
              indexed_by< N(ownerid), const_mem_fun<limit_base_quote, uint128_t, &limit_base_quote::by_owner_id> >,
              indexed_by< N(expire),  const_mem_fun<limit_base_quote, uint64_t,  &limit_base_quote::by_expiration> >
      > limit_base_quote_index;

      typedef limit_order<quote_token_type, base_token_type> limit_quote_base;
      typedef eosio::multi_index< N(sellqb), limit_quote_base,
              indexed_by< N(price),   const_mem_fun<limit_quote_base, uint128_t, &limit_quote_base::by_price > >,
              indexed_by< N(ownerid), const_mem_fun<limit_quote_base, uint128_t, &limit_quote_base::by_owner_id> >,
              indexed_by< N(expire),  const_mem_fun<limit_quote_base, uint64_t,  &limit_quote_base::by_expiration> >
      > limit_quote_base_index;





      account_index_type       _accounts;
      limit_base_quote_index   _base_quote_orders;
      limit_quote_base_index   _quote_base_orders;

      exchange( account_name contract = current_receiver(), account_name exchange_token_contract, symbol_name symbol )
      :_accounts( contract, contract )
       _base_quote_orders( contract, contract ),
       _quote_base_orders( contract, contract )
      {
      }

      ACTION( ExchangeAccount, deposit ) {
         account_name from;
         asset        amount;

         EOSLIB_SERIALIZE( deposit, (from)(amount) )
      };

      void on( const deposit& d ) {
         require_auth( d.from );

         auto owner = _accounts.find( d.from );
         if( owner == _accounts.end() ) {
            owner = _accounts.emplace( d.from, [&]( auto& a ) {
              a.owner = d.from;
            });
         }

         switch( d.amount.symbol ) {
            case base_token_type::symbol:
               BaseCurrency::inline_transfer( d.from, ExchangeAccount, base_token_type(d.amount.amount) );
               _accounts.modify( owner, 0, [&]( auto& a ) {
                  a.base_balance += base_token_type(d.amount);
               });
               break;
            case quote_token_type::symbol:
               QuoteCurrency::inline_transfer( d.from, ExchangeAccount, quote_token_type(d.amount.amount) );
               _accounts.modify( owner, 0, [&]( auto& a ) {
                  a.quote_balance += quote_token_type(d.amount);
               });
               break;
            default:
               eosio_assert( false, "invalid symbol" );
         }
      }

      ACTION( ExchangeAccount, withdraw ) {
         account_name to;
         asset        amount;

         EOSLIB_SERIALIZE( withdraw, (to)(amount) )
      };

      void on( const withdraw& w ) {
         require_auth( w.to );

         auto owner = _accounts.find( w.to );
         eosio_assert( owner != _accounts.end(), "unknown exchange account" );

         switch( w.amount.symbol ) {
            case base_token_type::symbol:
               eosio_assert( owner->base_balance >= base_token_type(w.amount), "insufficient balance" );

               _accounts.modify( owner, 0, [&]( auto& a ) {
                  a.base_balance -= base_token_type(w.amount);
               });

               BaseCurrency::inline_transfer( ExchangeAccount, w.to, base_token_type(w.amount.amount) );
               break;
            case quote_token_type::symbol:
               eosio_assert( owner->quote_balance >= quote_token_type(w.amount), "insufficient balance" );

               _accounts.modify( owner, 0, [&]( auto& a ) {
                  a.quote_balance -= quote_token_type(w.amount);
               });

               QuoteCurrency::inline_transfer( ExchangeAccount, w.to, quote_token_type(w.amount.amount) );
               break;
            default:
               eosio_assert( false, "invalid symbol" );
         }
      }

      ACTION( ExchangeAccount, neworder ) {
         account_name owner;
         uint32_t     id;
         asset        amount_to_sell;
         bool         fill_or_kill;
         uint128_t    sell_price;
         uint32_t     expiration;

         EOSLIB_SERIALIZE( neworder, (owner)(id)(amount_to_sell)(fill_or_kill)(sell_price)(expiration) )
      };

      void on( const neworder& order ) {
         require_auth( order.owner );

         if( order.amount_to_sell.symbol == base_token_type::symbol ) {
            _base_quote_orders.emplace( order.owner, [&]( auto& o ) {
               o.primary      = 0;// _base_quote_orders.next_available_id()
               o.owner        = order.owner;
               o.id           = order.id;
               o.expiration   = order.expiration;
               o.for_sale     = order.amount_to_sell;
               o.sell_price   = order.sell_price;
            });
         }
         else if( order.amount_to_sell.symbol == quote_token_type::symbol ) {
            _quote_base_orders.emplace( order.owner, [&]( auto& o ) {
               o.primary      = 0;// _base_quote_orders.next_available_id()
               o.owner        = order.owner;
               o.id           = order.id;
               o.expiration   = order.expiration;
               o.for_sale     = order.amount_to_sell;
               o.sell_price   = order.sell_price;
            });
         }
      }

      ACTION( ExchangeAccount, cancelorder ) {
         account_name owner;
         uint32_t     id;

         EOSLIB_SERIALIZE( cancelorder, (owner)(id) )
      };

      void on( const cancelorder& order ) {
         require_auth( order.owner );

         auto idx = _base_quote_orders.template get_index<N(ownerid)>();
         auto itr = idx.find( limit_base_quote::get_owner_id( order.owner, order.id ) );
         if( itr != idx.end() ) {
            idx.erase(itr);
         }
      }


      static void apply( uint64_t code, uint64_t act ) {
         if( !eosio::dispatch<exchange,
                deposit, withdraw,
                neworder, cancelorder
             >( code, act ) ) {
            exchange::exchange_currency::apply( code, act );
         }
      }
};
