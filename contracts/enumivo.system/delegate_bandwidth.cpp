/**
 *  @file
 *  @copyright defined in enumivo/LICENSE.txt
 */
#include "enumivo.system.hpp"

#include <enumivolib/enumivo.hpp>
#include <enumivolib/print.hpp>
#include <enumivolib/datastream.hpp>
#include <enumivolib/serialize.hpp>
#include <enumivolib/multi_index.hpp>
#include <enumivolib/privileged.h>
#include <enumivolib/transaction.hpp>

#include <enumivo.coin/enumivo.coin.hpp>


#include <cmath>
#include <map>

namespace enumivosystem {
   using enumivo::asset;
   using enumivo::indexed_by;
   using enumivo::const_mem_fun;
   using enumivo::bytes;
   using enumivo::print;
   using enumivo::permission_level;
   using std::map;
   using std::pair;

   static constexpr time refund_delay = 3*24*3600;
   static constexpr time refund_expiration_time = 3600;

   struct user_resources {
      account_name  owner;
      asset         net_weight;
      asset         cpu_weight;
      int64_t       ram_bytes = 0;

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ENULIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes) )
   };


   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct delegated_bandwidth {
      account_name  from;
      account_name  to;
      asset         net_weight;
      asset         cpu_weight;

      uint64_t  primary_key()const { return to; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ENULIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight) )

   };

   struct refund_request {
      account_name  owner;
      time          request_time;
      enumivo::asset  amount;

      uint64_t  primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      ENULIB_SERIALIZE( refund_request, (owner)(request_time)(amount) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this 
    *  facilitates simpler API for per-user queries
    */
   typedef enumivo::multi_index< N(userres), user_resources>      user_resources_table;
   typedef enumivo::multi_index< N(delband), delegated_bandwidth> del_bandwidth_table;
   typedef enumivo::multi_index< N(refunds), refund_request>      refunds_table;


   /**
    *  Called after a new account is created. This code enforces resource-limits rules
    *  for new accounts as well as new account naming conventions.
    *
    *  1. accounts cannot contain '.' symbols which forces all acccounts to be 12
    *  characters long without '.' until a future account auction process is implemented
    *  which prevents name squatting.
    *
    *  2. new accounts must stake a minimal number of tokens (as set in system parameters)
    *     therefore, this method will execute an inline buyram from receiver for newacnt in
    *     an amount equal to the current new account creation fee. 
    */
   void native::newaccount( account_name     creator,
                            account_name     newact
                            /*  no need to parse authorites
                            const authority& owner,
                            const authority& active*/ ) {
      auto name_str = enumivo::name{newact}.to_string();

      enumivo_assert( name_str.size() == 12 || creator == N(enumivo), "account names must be 12 chars long" );
      enumivo_assert( name_str.find_first_of('.') == std::string::npos  || creator == N(enumivo), "account names cannot contain '.' character");

      user_resources_table  userres( _self, newact);

      userres.emplace( newact, [&]( auto& res ) {
        res.owner = newact;
      });

      set_resource_limits( newact, 
                          0,//  r->ram_bytes, 
                           0, 0 );
                    //       r->net_weight.amount, 
                    //       r->cpu_weight.amount );
   }

   /**
    *  This action will buy an exact amount of ram and bill the payer the current market price.
    */
   void system_contract::buyrambytes( account_name payer, account_name receiver, uint32_t bytes ) {
      auto itr = _rammarket.find(S(4,RAMEOS));
      auto tmp = *itr;
      auto eosout = tmp.convert( asset(bytes,S(0,RAM)), S(4,EOS) );

      buyram( payer, receiver, eosout );
   }


   /**
    *  When buying ram the payer irreversiblly transfers quant to system contract and only
    *  the receiver may reclaim the tokens via the sellram action. The receiver pays for the
    *  storage of all database records associated with this action.
    *
    *  RAM is a scarce resource whose supply is defined by global properties max_ram_size. RAM is
    *  priced using the bancor algorithm such that price-per-byte with a constant reserve ratio of 100:1. 
    */
   void system_contract::buyram( account_name payer, account_name receiver, asset quant ) 
   {
      require_auth( payer );
      enumivo_assert( quant.amount > 0, "must purchase a positive amount" );

      if( payer != N(enumivo) ) {
         INLINE_ACTION_SENDER(enumivo::token, transfer)( N(enumivo.coin), {payer,N(active)},
                                                       { payer, N(enumivo), quant, std::string("buy ram") } );
      }


      int64_t bytes_out;

      auto itr = _rammarket.find(S(4,RAMEOS));
      _rammarket.modify( itr, 0, [&]( auto& es ) {
          bytes_out = es.convert( quant,  S(0,RAM) ).amount;
      });


      enumivo_assert( bytes_out > 0, "must reserve a positive amount" );

      _gstate.total_ram_bytes_reserved += uint64_t(bytes_out);
      _gstate.total_ram_stake          += quant.amount;

      user_resources_table  userres( _self, receiver );
      auto res_itr = userres.find( receiver );
      if( res_itr ==  userres.end() ) {
         res_itr = userres.emplace( receiver, [&]( auto& res ) {
               res.owner = receiver;
               res.ram_bytes = bytes_out;
            });
      } else {
         userres.modify( res_itr, receiver, [&]( auto& res ) {
               res.ram_bytes += bytes_out;
            });
      }
      set_resource_limits( res_itr->owner, res_itr->ram_bytes, res_itr->net_weight.amount, res_itr->cpu_weight.amount );
   }


   /**
    *  While buying ram uses the current market price according to the bancor-algorithm, selling ram only
    *  refunds the purchase price to the account. In this way there is no profit to be made through buying
    *  and selling ram.
    */
   void system_contract::sellram( account_name account, int64_t bytes ) {
      require_auth( account );
      enumivo_assert( bytes > 0, "cannot sell negative byte" );

      user_resources_table  userres( _self, account );
      auto res_itr = userres.find( account );
      enumivo_assert( res_itr != userres.end(), "no resource row" );
      enumivo_assert( res_itr->ram_bytes >= bytes, "insufficient quota" );

      asset tokens_out;
      auto itr = _rammarket.find(S(4,RAMEOS));
      _rammarket.modify( itr, 0, [&]( auto& es ) {
          /// the cast to int64_t of bytes is safe because we certify bytes is <= quota which is limited by prior purchases
          tokens_out = es.convert( asset(bytes,S(0,RAM)),  S(4,EOS) );
      });

      _gstate.total_ram_bytes_reserved -= bytes;
      _gstate.total_ram_stake          -= tokens_out.amount;

      //// this shouldn't happen, but just in case it does we should prevent it
      enumivo_assert( _gstate.total_ram_stake >= 0, "error, attempt to unstake more tokens than previously staked" );

      userres.modify( res_itr, account, [&]( auto& res ) {
          res.ram_bytes -= bytes;
      });
      set_resource_limits( res_itr->owner, res_itr->ram_bytes, res_itr->net_weight.amount, res_itr->cpu_weight.amount );

      if( N(enumivo) != account ) {
         INLINE_ACTION_SENDER(enumivo::token, transfer)( N(enumivo.coin), {N(enumivo),N(active)},
                                                       { N(enumivo), account, asset(tokens_out), std::string("sell ram") } );
      }
   }

   void system_contract::delegatebw( account_name from, account_name receiver,
                                     asset stake_net_quantity, 
                                     asset stake_cpu_quantity, bool transfer )
                                    
   {
      require_auth( from );

      enumivo_assert( stake_cpu_quantity >= asset(0), "must stake a positive amount" );
      enumivo_assert( stake_net_quantity >= asset(0), "must stake a positive amount" );

      auto total_stake = stake_cpu_quantity.amount + stake_net_quantity.amount;
      enumivo_assert( total_stake > 0, "must stake a positive amount" );

      account_name source_stake_from = from;

      if( transfer ) from = receiver;

      del_bandwidth_table     del_tbl( _self, from);
      auto itr = del_tbl.find( receiver );
      if( itr == del_tbl.end() ) {
         del_tbl.emplace( from, [&]( auto& dbo ){
               dbo.from          = from;
               dbo.to            = receiver;
               dbo.net_weight    = stake_net_quantity;
               dbo.cpu_weight    = stake_cpu_quantity;
            });
      }
      else {
         del_tbl.modify( itr, 0, [&]( auto& dbo ){
               dbo.net_weight    += stake_net_quantity;
               dbo.cpu_weight    += stake_cpu_quantity;
            });
      }

      user_resources_table   totals_tbl( _self, receiver );
      auto tot_itr = totals_tbl.find( receiver );
      if( tot_itr ==  totals_tbl.end() ) {
         tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
               tot.owner = receiver;
               tot.net_weight    = stake_net_quantity;
               tot.cpu_weight    = stake_cpu_quantity;
            });
      } else {
         totals_tbl.modify( tot_itr, from == receiver ? from : 0, [&]( auto& tot ) {
               tot.net_weight    += stake_net_quantity;
               tot.cpu_weight    += stake_cpu_quantity;
            });
      }

      set_resource_limits( tot_itr->owner, tot_itr->ram_bytes, tot_itr->net_weight.amount, tot_itr->cpu_weight.amount );

      if( N(enumivo) != source_stake_from ) {
         INLINE_ACTION_SENDER(enumivo::token, transfer)( N(enumivo.coin), {from,N(active)},
                                                       { source_stake_from, N(enumivo), asset(total_stake), std::string("stake bandwidth") } );
      }

      auto from_voter = _voters.find(from);
      if( from_voter == _voters.end() ) {
         from_voter = _voters.emplace( from, [&]( auto& v ) {
            v.owner  = from;
            v.staked = total_stake;
         });
      } else {
         _voters.modify( from_voter, 0, [&]( auto& v ) {
            v.staked += total_stake;
         });
      }

      if( from_voter->producers.size() || from_voter->proxy ) {
         voteproducer( from, from_voter->proxy, from_voter->producers );
      }
   } // delegatebw


   void validate_b1_vesting( int64_t stake ) {
      const int64_t seconds_per_year = 60*60*24*365;
      const int64_t base_time = 1527811200; /// 2018-06-01
      const int64_t max_claimable = 100'000'000'0000ll;
      const int64_t claimable = int64_t(max_claimable * double(now()-base_time) / (10*seconds_per_year) );

      enumivo_assert( max_claimable - claimable <= stake, "b1 can only claim their tokens over 10 years" );
   }

   void system_contract::undelegatebw( account_name from, account_name receiver,
                                       asset unstake_net_quantity, asset unstake_cpu_quantity )
   {
      enumivo_assert( unstake_cpu_quantity >= asset(), "must unstake a positive amount" );
      enumivo_assert( unstake_net_quantity >= asset(), "must unstake a positive amount" );

      require_auth( from );

      del_bandwidth_table     del_tbl( _self, from );
      const auto& dbw = del_tbl.get( receiver );
      enumivo_assert( dbw.net_weight.amount >= unstake_net_quantity.amount, "insufficient staked net bandwidth" );
      enumivo_assert( dbw.cpu_weight.amount >= unstake_cpu_quantity.amount, "insufficient staked cpu bandwidth" );

      auto total_refund = unstake_cpu_quantity.amount + unstake_net_quantity.amount;

      _voters.modify( _voters.get(from), 0, [&]( auto& v ) {
         v.staked -= total_refund;
         if( from == N(b1) ) {
            validate_b1_vesting( v.staked );
         }
      });


      enumivo_assert( total_refund > 0, "must unstake a positive amount" );

      if( dbw.net_weight == unstake_net_quantity && dbw.cpu_weight == unstake_cpu_quantity ) {
         del_tbl.erase( dbw );
      } else { 
         del_tbl.modify( dbw, from, [&]( auto& dbo ){
               dbo.net_weight -= unstake_net_quantity;
               dbo.cpu_weight -= unstake_cpu_quantity;
         });
      }

      user_resources_table totals_tbl( _self, receiver );

      const auto& totals = totals_tbl.get( receiver );
      totals_tbl.modify( totals, 0, [&]( auto& tot ) {
            tot.net_weight -= unstake_net_quantity;
            tot.cpu_weight -= unstake_cpu_quantity;
      });

      set_resource_limits( receiver, totals.ram_bytes, totals.net_weight.amount, totals.cpu_weight.amount );

      refunds_table refunds_tbl( _self, from );
      //create refund request
      auto req = refunds_tbl.find( from );
      if ( req != refunds_tbl.end() ) {
         refunds_tbl.modify( req, 0, [&]( refund_request& r ) {
               r.amount += unstake_net_quantity + unstake_cpu_quantity;
               r.request_time = now();
            });
      } else {
         refunds_tbl.emplace( from, [&]( refund_request& r ) {
               r.owner = from;
               r.amount = unstake_net_quantity + unstake_cpu_quantity;
               r.request_time = now();
            });
      }

      //create or replace deferred transaction
      //refund act;
      //act.owner = from;
      enumivo::transaction out;
      out.actions.emplace_back( permission_level{ from, N(active) }, _self, N(refund), from );
      out.delay_sec = refund_delay;
      out.send( from, receiver, true );

      const auto& fromv = _voters.get( from );

      if( fromv.producers.size() || fromv.proxy ) {
         voteproducer( from, fromv.proxy, fromv.producers );
      }
   } // undelegatebw


   void system_contract::refund( const account_name owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( _self, owner );
      auto req = refunds_tbl.find( owner );
      enumivo_assert( req != refunds_tbl.end(), "refund request not found" );
      enumivo_assert( req->request_time + refund_delay <= now(), "refund is not available yet" );
      // Until now() becomes NOW, the fact that now() is the timestamp of the previous block could in theory
      // allow people to get their tokens earlier than the 3 day delay if the unstake happened immediately after many
      // consecutive missed blocks.

      INLINE_ACTION_SENDER(enumivo::token, transfer)( N(enumivo.coin), {N(enumivo),N(active)},
                                                    { N(enumivo), req->owner, req->amount, std::string("unstake") } );

      refunds_tbl.erase( req );
   }


} //namespace enumivosystem
