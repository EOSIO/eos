/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>
#include <eosiolib/token.hpp>
#include <eosiolib/db.hpp>
#include <eosiolib/reflect.hpp>
#include <eosiolib/print.hpp>

#include <eosiolib/generic_currency.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>

namespace eosiosystem {

   template<account_name SystemAccount>
   class contract {
      public:
         static const account_name system_account = SystemAccount;
         typedef eosio::generic_currency< eosio::token<system_account,S(4,EOS)> > currency;

         /*
         struct total_bandwidth {
            account_name owner;
            typename currency::token_type total_net_weight; 
            typename currency::token_type total_cpu_weight; 
         };
         */

//         typedef eosio::table64<SystemAccount, N(totalband), SystemAccount, total_bandwidth>      total_bandwidth;

         //eosio::multi_index< N(totalband), total_bandwidth > bandwidth_index;

         struct delegated_bandwidth {
            uint64_t     primary;
            account_name from;
            account_name to;
            typename currency::token_type net_weight; 
            typename currency::token_type cpu_weight; 

            uint32_t start_pending_net_withdraw = 0;
            typename currency::token_type pending_net_withdraw;
            uint64_t deferred_net_withdraw_handler = 0;

            uint32_t start_pending_cpu_withdraw = 0;
            typename currency::token_type pending_cpu_withdraw;
            uint64_t deferred_cpu_withdraw_handler = 0;

            
            uint64_t  primary_key()const { return primary; }
            uint128_t by_from_to()const  { return (uint128_t(from)<<64) | to; }
         };


         static eosio::multi_index< N(delband), delegated_bandwidth,
              eosio::index_by<0, N(byfromto), delegated_bandwidth, 
                             eosio::const_mem_fun<delegated_bandwidth, uint128_t, &delegated_bandwidth::by_from_to> >
         > del_bandwidth_index;


         ACTION( SystemAccount, finshundel ) {
            account_name from;
            account_name to;
         };

         ACTION( SystemAccount, regproducer ) {
            account_name producer_to_register;

            EOSLIB_SERIALIZE( regproducer, (producer_to_register) );
         };

         ACTION( SystemAccount, regproxy ) {
            account_name proxy_to_register;

            EOSLIB_SERIALIZE( regproxy, (proxy_to_register) );
         };

         ACTION( SystemAccount, delnetbw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_quantity;


            EOSLIB_SERIALIZE( delnetbw, (delegator)(receiver)(stake_quantity) )
         };

         ACTION( SystemAccount, undelnetbw ) {
            account_name                    from;
            account_name                    receiver;
            typename currency::token_type   stake_quantity;

            EOSLIB_SERIALIZE( delnetbw, (delegator)(receiver)(stake_quantity) )
         };

         ACTION( SystemAccount, nonce ) {
            eosio::string                   value;

            EOSLIB_SERIALIZE( nonce, (value) );
         };

         /// new id options:
         //  1. hash + collision 
         //  2. incrementing count  (key=> tablename 

         void on( const delnetbw& del ) {
            require_auth( del.from );
          //  require_account( receiver );

            auto idx = del_bandwidth_index.template get_index<N(byfromto)>();
            auto itr = idx.find( (uint128_t(del.from) << 64) | del.to  );
            if( itr == idx.end() ) {
               del_bandwidth_index.emplace( del.from, [&]( auto& dbo ){
                  dbo.id         = del_bandwidth_index.new_id( del.from );
                  dbo.from       = del.from;      
                  dbo.to         = del.to;      
                  dbo.net_weight = del.stake_quantity;
               });
            }
            else {
               del_bandwidth_index.update( *itr, del.from, [&]( auto& dbo ){
                  dbo.net_weight = del.stake_quantity;
               });
            }
            currency::inline_transfer( del.from, SystemAccount, del.stake_quantity, "stake bandwidth" );
         }

         void on( const regproducer& reg ) {
            require_auth( reg.producer_to_register );
         }

         void on( const regproxy& reg ) {
            require_auth( reg.proxy_to_register );
         }

         void on( const nonce& ) {
         }

         void apply( account_name code, action_name act ) {
            /*
            if( !eosio::dispatch<contract, regproducer, regproxy, nonce>( code, act) ) {
               if ( !eosio::dispatch<currency, typename currency::transfer, typename currency::issue>( code, act ) ) {
                  eosio::print("Unexpected action: ", eosio::name(act), "\n");
                  eos_assert( false, "received unexpected action");
               }
            }
            */
         } /// apply 
   };

} /// eosiosystem 

