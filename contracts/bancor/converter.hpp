#pragma once
#include <eoslib/types.hpp>
#include <eoslib/raw.hpp>

namespace bancor {
   template<typename ConverterCurrency, typename FirstCurrency, typename SecondCurrency>
   class converter_contract {
      public:
         typedef ConverterCurrency   converter_currency;
         typedef FirstCurrency       first_currency;
         typedef SecondCurrency      second_currency;


         static const account_name   converter_account = converter_currency::code;

         struct converter_state {
            typename converter_currency::token_type supply; /// total supply held by all users
            typename converter_currency::token_type balance; /// supply held by converter in its own balance
         };

         struct converter_args {
            eosio::account_name    to_currency_account;
            eosio::symbol_name     to_currency_symbol;
            eosio::asset           min_return_currency;

             template<typename DataStream>
             friend DataStream& operator << ( DataStream& ds, const converter_args& t ){
                return ds << t.to_currency_account << t.to_currency_symbol << t.min_return_currency;
             }
             template<typename DataStream>
             friend DataStream& operator >> ( DataStream& ds, converter_args& t ){
                return ds >> t.to_currency_account >> t.to_currency_symbol >> t.min_return_currency;
             }

         };


         template<typename CurrencyType, uint32_t Weight=500000, uint32_t Base=1000000> 
         struct connector {
            typedef CurrencyType                            currency_type;
            typedef typename converter_currency::token_type converter_token_type;
            typedef typename currency_type::token_type      in_token_type;

            static converter_token_type convert_to_converter( in_token_type in, converter_state& state ) {
               in_token_type balance = currency_type::get_balance( converter_account );

               /// balance already changed when transfer executed, get pre-transfer balance
               in_token_type previous_balance = balance - in; 

               auto init_price = (previous_balance * Base) / (state.supply * Weight);
               auto init_out   = init_price * in;

               auto out_price  = (balance*Base) / ( (state.supply+init_out)*Weight );
               auto final_out  = out_price * in;

               state.balance += final_out;
               state.supply  += final_out;

               return final_out;
            }


            static in_token_type convert_from_converter( converter_token_type converter_in, converter_state& state ) {
               in_token_type  to_balance = CurrencyType::get_balance( converter_account );

               auto          init_price = (to_balance * Base) / (state.supply * Weight);
               in_token_type init_out   = init_price * converter_in;

               state.supply  -= converter_in;
               state.balance -= converter_in;

               auto out_price  = ((to_balance-init_out) * Base) / ( (state.supply) * Weight );

               return out_price * converter_in;
            }

         };

         typedef connector<first_currency>  first_connector;
         typedef connector<second_currency> second_connector;



         /**
          * This is called when we receive RELAY tokens from user and wish to
          * convert to one of the connector currencies.
          */
         static void on_convert( const typename converter_currency::transfer& trans, 
                                       const converter_args& args, 
                                       converter_state& state ) {

            if( args.to_currency_symbol== first_currency::symbol && trans.get_account() == first_currency::code ) {
               auto output = first_connector::convert_from_converter( trans.quantity, state );
               save_and_send( trans.from, state, output, args.min_return_currency );
            }
            else if( args.to_currency_symbol == second_currency::symbol && trans.get_account() == second_currency::code) {
               auto output = second_connector::convert_from_converter( trans.quantity, state );
               save_and_send( trans.from, state, output, args.min_return_currency );
            } 
            else {
               assert( false, "invalid to currency" );
            }
         }

         template<typename Output>
         static void save_and_send( account_name from, converter_state& state, Output out, eosio::asset min_return ) {
         }


         /**
          *  This is called when the converter receives one of the connector currencies and it 
          *  will send either converter tokens or a different connector currency in response.
          */
         template<typename ConnectorType>
         static void on_convert( const typename ConnectorType::currency_type::transfer& trans,
                                       const converter_args& args, 
                                       converter_state& state ) 
         {
            /// convert to converter
            auto converter_out = ConnectorType::convert_to_converter( trans.quantity, state );

            if( args.to_currency_type == converter_currency::symbol_name )
            {
               save_and_send( trans.from, state, converter_out, args.min_return );
            }
            else 
            {
               auto output = ConnectorType::convert_from_converter( converter_out, state ); 
               save_and_send( trans.from, state, output, args.min_return );
            }
         }

         static converter_state read_converter_state() {
            return converter_state();
         }

         /**
          *  This method factors out the boiler plate for parsing args and loading the
          *  initial state before dispatching to the proper on_convert case
          */
         template<typename TransferMemo>
         static void start_convert( const TransferMemo& trans ) {
            eosio::datastream<const char*> ds( trans.memo.get_data(), trans.memo.get_size() );
            converter_args args; 
            ds >> args;
            assert( args.to_currency_account != TransferMemo::get_account() &&
                    args.to_currency_symbol  != trans.quantity.symbol, "cannot convert to self" );

            auto state = read_converter_state();
            on_convert( trans, args, state );
         }


         /**
          * converter_account first needs to call the currency handler to perform
          * user-to-user transfers of the converter token, then if a transfer is sending
          * the token back to the converter contract, it should convert like everything else.
          *
          *  This method should be called from apply( code, action ) for each of the
          *  transfer types that we support (for each currency)
          */
         static void on( const typename converter_currency::transfer_memo& trans ) {
            converter_currency::on( trans );
            if( trans.to == converter_account ) {
               start_convert( trans );
            }
         }

         /**
          *  All other currencies simply call start_convert if to == converter_account
          */
         template<typename Currency>
         static void on( const typename Currency::transfer_memo& trans ) {
            if( trans.to == converter_account ) {
               start_convert( trans );
            } else {
               assert( trans.from == converter_account, 
                       "received unexpected notification of transfer" );
            }
         }

         static void apply( eosio::account_name c, action_name a ) {
            if( !eosio::dispatch< converter_contract, 
                           typename converter_currency::transfer_memo,
                           typename converter_currency::issue,
                           typename first_currency::transfer_memo,
                           typename second_currency::transfer_memo>(c, a) ) {
               assert( false, "received unexpected action" );
            }
         }
   }; /// converter_contract
} /// namespace bancor
