/**
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.system/eosio.system.hpp>
#include <eosio.system/rex.results.hpp>

namespace eosiosystem {

   void system_contract::deposit( const name& owner, const asset& amount )
   {
      require_auth( owner );

      check( amount.symbol == core_symbol(), "must deposit core token" );
      check( 0 < amount.amount, "must deposit a positive amount" );
      // inline transfer from owner's token balance
      {
         token::transfer_action transfer_act{ token_account, { owner, active_permission } };
         transfer_act.send( owner, rex_account, amount, "deposit to REX fund" );
      }
      transfer_to_fund( owner, amount );
   }

   void system_contract::withdraw( const name& owner, const asset& amount )
   {
      require_auth( owner );

      check( amount.symbol == core_symbol(), "must withdraw core token" );
      check( 0 < amount.amount, "must withdraw a positive amount" );
      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
      transfer_from_fund( owner, amount );
      // inline transfer to owner's token balance
      {
         token::transfer_action transfer_act{ token_account, { rex_account, active_permission } };
         transfer_act.send( rex_account, owner, amount, "withdraw from REX fund" );
      }
   }

   void system_contract::buyrex( const name& from, const asset& amount )
   {
      require_auth( from );

      check( amount.symbol == core_symbol(), "asset must be core token" );
      check( 0 < amount.amount, "must use positive amount" );
      check_voting_requirement( from );
      transfer_from_fund( from, amount );
      const asset rex_received    = add_to_rex_pool( amount );
      const asset delta_rex_stake = add_to_rex_balance( from, amount, rex_received );
      runrex(2);
      update_rex_account( from, asset( 0, core_symbol() ), delta_rex_stake );
      // dummy action added so that amount of REX tokens purchased shows up in action trace 
      rex_results::buyresult_action buyrex_act( rex_account, std::vector<eosio::permission_level>{ } );
      buyrex_act.send( rex_received );
   }

   void system_contract::unstaketorex( const name& owner, const name& receiver, const asset& from_net, const asset& from_cpu )
   {
      require_auth( owner );

      check( from_net.symbol == core_symbol() && from_cpu.symbol == core_symbol(), "asset must be core token" );
      check( (0 <= from_net.amount) && (0 <= from_cpu.amount) && (0 < from_net.amount || 0 < from_cpu.amount),
             "must unstake a positive amount to buy rex" );
      check_voting_requirement( owner );

      {
         del_bandwidth_table dbw_table( _self, owner.value );
         auto del_itr = dbw_table.require_find( receiver.value, "delegated bandwidth record does not exist" );
         check( from_net.amount <= del_itr->net_weight.amount, "amount exceeds tokens staked for net");
         check( from_cpu.amount <= del_itr->cpu_weight.amount, "amount exceeds tokens staked for cpu");
         dbw_table.modify( del_itr, same_payer, [&]( delegated_bandwidth& dbw ) {
            dbw.net_weight.amount -= from_net.amount;
            dbw.cpu_weight.amount -= from_cpu.amount;
         });
         if ( del_itr->is_empty() ) {
            dbw_table.erase( del_itr );
         }
      }

      update_resource_limits( name(0), receiver, -from_net.amount, -from_cpu.amount );

      const asset payment = from_net + from_cpu;
      // inline transfer from stake_account to rex_account
      {
         token::transfer_action transfer_act{ token_account, { stake_account, active_permission } };
         transfer_act.send( stake_account, rex_account, payment, "buy REX with staked tokens" );
      }
      const asset rex_received = add_to_rex_pool( payment );
      add_to_rex_balance( owner, payment, rex_received );
      runrex(2);
      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ), true );
      // dummy action added so that amount of REX tokens purchased shows up in action trace
      rex_results::buyresult_action buyrex_act( rex_account, std::vector<eosio::permission_level>{ } );
      buyrex_act.send( rex_received );
   }

   void system_contract::sellrex( const name& from, const asset& rex )
   {
      require_auth( from );

      runrex(2);

      auto bitr = _rexbalance.require_find( from.value, "user must first buyrex" );
      check( rex.amount > 0 && rex.symbol == bitr->rex_balance.symbol,
             "asset must be a positive amount of (REX, 4)" );
      process_rex_maturities( bitr );
      check( rex.amount <= bitr->matured_rex, "insufficient available rex" );

      const auto current_order = fill_rex_order( bitr, rex );
      if ( current_order.success && current_order.proceeds.amount == 0 ) {
         check( false, "proceeds are negligible" );
      }
      asset pending_sell_order = update_rex_account( from, current_order.proceeds, current_order.stake_change );
      if ( !current_order.success ) {
         if ( from == "b1"_n ) {
            check( false, "b1 sellrex orders should not be queued" );
         }
         /**
          * REX order couldn't be filled and is added to queue.
          * If account already has an open order, requested rex is added to existing order.
          */
         auto oitr = _rexorders.find( from.value );
         if ( oitr == _rexorders.end() ) {
            oitr = _rexorders.emplace( from, [&]( auto& order ) {
               order.owner         = from;
               order.rex_requested = rex;
               order.is_open       = true;
               order.proceeds      = asset( 0, core_symbol() );
               order.stake_change  = asset( 0, core_symbol() );
               order.order_time    = current_time_point();
            });
         } else {
            _rexorders.modify( oitr, same_payer, [&]( auto& order ) {
               order.rex_requested.amount += rex.amount;
            });
         }
         pending_sell_order.amount = oitr->rex_requested.amount;
      }
      check( pending_sell_order.amount <= bitr->matured_rex, "insufficient funds for current and scheduled orders" );
      // dummy action added so that sell order proceeds show up in action trace
      if ( current_order.success ) {
         rex_results::sellresult_action sellrex_act( rex_account, std::vector<eosio::permission_level>{ } );
         sellrex_act.send( current_order.proceeds );
      }
   }

   void system_contract::cnclrexorder( const name& owner )
   {
      require_auth( owner );

      auto itr = _rexorders.require_find( owner.value, "no sellrex order is scheduled" );
      check( itr->is_open, "sellrex order has been filled and cannot be canceled" );
      _rexorders.erase( itr );
   }

   void system_contract::rentcpu( const name& from, const name& receiver, const asset& loan_payment, const asset& loan_fund )
   {
      require_auth( from );

      rex_cpu_loan_table cpu_loans( _self, _self.value );
      int64_t rented_tokens = rent_rex( cpu_loans, from, receiver, loan_payment, loan_fund );
      update_resource_limits( from, receiver, 0, rented_tokens );
   }

   void system_contract::rentnet( const name& from, const name& receiver, const asset& loan_payment, const asset& loan_fund )
   {
      require_auth( from );

      rex_net_loan_table net_loans( _self, _self.value );
      int64_t rented_tokens = rent_rex( net_loans, from, receiver, loan_payment, loan_fund );
      update_resource_limits( from, receiver, rented_tokens, 0 );
   }

   void system_contract::fundcpuloan( const name& from, uint64_t loan_num, const asset& payment )
   {
      require_auth( from );

      rex_cpu_loan_table cpu_loans( _self, _self.value );
      fund_rex_loan( cpu_loans, from, loan_num, payment  );
   }

   void system_contract::fundnetloan( const name& from, uint64_t loan_num, const asset& payment )
   {
      require_auth( from );

      rex_net_loan_table net_loans( _self, _self.value );
      fund_rex_loan( net_loans, from, loan_num, payment );
   }

   void system_contract::defcpuloan( const name& from, uint64_t loan_num, const asset& amount )
   {
      require_auth( from );

      rex_cpu_loan_table cpu_loans( _self, _self.value );
      defund_rex_loan( cpu_loans, from, loan_num, amount );
   }

   void system_contract::defnetloan( const name& from, uint64_t loan_num, const asset& amount )
   {
      require_auth( from );

      rex_net_loan_table net_loans( _self, _self.value );
      defund_rex_loan( net_loans, from, loan_num, amount );
   }

   void system_contract::updaterex( const name& owner )
   {
      require_auth( owner );

      runrex(2);

      auto itr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      const asset init_stake = itr->vote_stake;

      auto rexp_itr = _rexpool.begin();
      const int64_t total_rex      = rexp_itr->total_rex.amount;
      const int64_t total_lendable = rexp_itr->total_lendable.amount;
      const int64_t rex_balance    = itr->rex_balance.amount;

      asset current_stake( 0, core_symbol() );
      if ( total_rex > 0 ) {
         current_stake.amount = ( uint128_t(rex_balance) * total_lendable ) / total_rex;
      }
      _rexbalance.modify( itr, same_payer, [&]( auto& rb ) {
         rb.vote_stake = current_stake;
      });

      update_rex_account( owner, asset( 0, core_symbol() ), current_stake - init_stake, true );
      process_rex_maturities( itr );
   }

   void system_contract::setrex( const asset& balance )
   {
      require_auth( "eosio"_n );

      check( balance.amount > 0, "balance must be set to have a positive amount" );
      check( balance.symbol == core_symbol(), "balance symbol must be core symbol" );
      check( rex_system_initialized(), "rex system is not initialized" );
      _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& pool ) {
         pool.total_rent = balance;
      });
   }

   void system_contract::rexexec( const name& user, uint16_t max )
   {
      require_auth( user );

      runrex( max );
   }

   void system_contract::consolidate( const name& owner )
   {
      require_auth( owner );

      runrex(2);

      auto bitr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      asset rex_in_sell_order = update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
      consolidate_rex_balance( bitr, rex_in_sell_order );
   }

   void system_contract::mvtosavings( const name& owner, const asset& rex )
   {
      require_auth( owner );

      runrex(2);

      auto bitr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      check( rex.amount > 0 && rex.symbol == bitr->rex_balance.symbol, "asset must be a positive amount of (REX, 4)" );
      const asset   rex_in_sell_order = update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
      const int64_t rex_in_savings    = read_rex_savings( bitr );
      check( rex.amount + rex_in_sell_order.amount + rex_in_savings <= bitr->rex_balance.amount,
             "insufficient REX balance" );
      process_rex_maturities( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         int64_t moved_rex = 0;
         while ( !rb.rex_maturities.empty() && moved_rex < rex.amount) {
            const int64_t drex = std::min( rex.amount - moved_rex, rb.rex_maturities.back().second );
            rb.rex_maturities.back().second -= drex;
            moved_rex                       += drex;
            if ( rb.rex_maturities.back().second == 0 ) {
               rb.rex_maturities.pop_back();
            }
         }
         if ( moved_rex < rex.amount ) {
            const int64_t drex = rex.amount - moved_rex;
            rb.matured_rex    -= drex;
            moved_rex         += drex;
            check( rex_in_sell_order.amount <= rb.matured_rex, "logic error in mvtosavings" );
         }
         check( moved_rex == rex.amount, "programmer error in mvtosavings" );
      });
      put_rex_savings( bitr, rex_in_savings + rex.amount );
   }

   void system_contract::mvfrsavings( const name& owner, const asset& rex )
   {
      require_auth( owner );

      runrex(2);

      auto bitr = _rexbalance.require_find( owner.value, "account has no REX balance" );
      check( rex.amount > 0 && rex.symbol == bitr->rex_balance.symbol, "asset must be a positive amount of (REX, 4)" );
      const int64_t rex_in_savings = read_rex_savings( bitr );
      check( rex.amount <= rex_in_savings, "insufficient REX in savings" );
      process_rex_maturities( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         const time_point_sec maturity = get_rex_maturity();
         if ( !rb.rex_maturities.empty() && rb.rex_maturities.back().first == maturity ) {
            rb.rex_maturities.back().second += rex.amount;
         } else {
            rb.rex_maturities.emplace_back( maturity, rex.amount );
         }
      });
      put_rex_savings( bitr, rex_in_savings - rex.amount );
      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );
   }

   void system_contract::closerex( const name& owner )
   {
      require_auth( owner );

      if ( rex_system_initialized() )
         runrex(2);

      update_rex_account( owner, asset( 0, core_symbol() ), asset( 0, core_symbol() ) );

      /// check for any outstanding loans or rex fund
      {
         rex_cpu_loan_table cpu_loans( _self, _self.value );
         auto cpu_idx = cpu_loans.get_index<"byowner"_n>();
         bool no_outstanding_cpu_loans = ( cpu_idx.find( owner.value ) == cpu_idx.end() );

         rex_net_loan_table net_loans( _self, _self.value );
         auto net_idx = net_loans.get_index<"byowner"_n>();
         bool no_outstanding_net_loans = ( net_idx.find( owner.value ) == net_idx.end() );

         auto fund_itr = _rexfunds.find( owner.value );
         bool no_outstanding_rex_fund = ( fund_itr != _rexfunds.end() ) && ( fund_itr->balance.amount == 0 );

         if ( no_outstanding_cpu_loans && no_outstanding_net_loans && no_outstanding_rex_fund ) {
            _rexfunds.erase( fund_itr );
         }
      }

      /// check for remaining rex balance
      {
         auto rex_itr = _rexbalance.find( owner.value );
         if ( rex_itr != _rexbalance.end() ) {
            check( rex_itr->rex_balance.amount == 0, "account has remaining REX balance, must sell first");
            _rexbalance.erase( rex_itr );
         }
      }
   }

   /**
    * @brief Updates account NET and CPU resource limits
    *
    * @param from - account charged for RAM if there is a need
    * @param receiver - account whose resource limits are updated
    * @param delta_net - change in NET bandwidth limit
    * @param delta_cpu - change in CPU bandwidth limit
    */
   void system_contract::update_resource_limits( const name& from, const name& receiver, int64_t delta_net, int64_t delta_cpu )
   {
      if ( delta_cpu == 0 && delta_net == 0 ) { // nothing to update
         return;
      }

      user_resources_table totals_tbl( _self, receiver.value );
      auto tot_itr = totals_tbl.find( receiver.value );
      if ( tot_itr == totals_tbl.end() ) {
         check( 0 <= delta_net && 0 <= delta_cpu, "logic error, should not occur");
         tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
            tot.owner      = receiver;
            tot.net_weight = asset( delta_net, core_symbol() );
            tot.cpu_weight = asset( delta_cpu, core_symbol() );
         });
      } else {
         totals_tbl.modify( tot_itr, same_payer, [&]( auto& tot ) {
            tot.net_weight.amount += delta_net;
            tot.cpu_weight.amount += delta_cpu;
         });
      }
      check( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
      check( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );

      {
         bool net_managed = false;
         bool cpu_managed = false;

         auto voter_itr = _voters.find( receiver.value );
         if( voter_itr != _voters.end() ) {
            net_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::net_managed );
            cpu_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::cpu_managed );
         }

         if( !(net_managed && cpu_managed) ) {
            int64_t ram_bytes = 0, net = 0, cpu = 0;
            get_resource_limits( receiver, ram_bytes, net, cpu );

            set_resource_limits( receiver,
                                 ram_bytes,
                                 net_managed ? net : tot_itr->net_weight.amount,
                                 cpu_managed ? cpu : tot_itr->cpu_weight.amount );
         }
      }

      if ( tot_itr->is_empty() ) {
         totals_tbl.erase( tot_itr );
      }
   }

   /**
    * @brief Checks if account satisfies voting requirement (voting for a proxy or 21 producers)
    * for buying REX
    *
    * @param owner - account buying or already holding REX tokens
    * @err_msg - error message
    */
   void system_contract::check_voting_requirement( const name& owner, const char* error_msg )const
   {
      auto vitr = _voters.find( owner.value );
      check( vitr != _voters.end() && ( vitr->proxy || 21 <= vitr->producers.size() ), error_msg );
   }

   /**
    * @brief Checks if CPU and Network loans are available
    *
    * Loans are available if 1) REX pool lendable balance is nonempty, and 2) there are no
    * unfilled sellrex orders.
    */
   bool system_contract::rex_loans_available()const
   {
      if ( !rex_available() ) {
         return false;
      } else {
         if ( _rexorders.begin() == _rexorders.end() ) {
            return true; // no outstanding sellrex orders
         } else {
            auto idx = _rexorders.get_index<"bytime"_n>();
            return !idx.begin()->is_open; // no outstanding unfilled sellrex orders
         }
      }
   }

   /**
    * @brief Updates rex_pool balances upon creating a new loan or renewing an existing one
    *
    * @param payment - loan fee paid
    * @param rented_tokens - amount of tokens to be staked to loan receiver
    * @param new_loan - flag indicating whether the loan is new or being renewed
    */
   void system_contract::add_loan_to_rex_pool( const asset& payment, int64_t rented_tokens, bool new_loan )
   {
      _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& rt ) {
         // add payment to total_rent
         rt.total_rent.amount    += payment.amount;
         // move rented_tokens from total_unlent to total_lent
         rt.total_unlent.amount  -= rented_tokens;
         rt.total_lent.amount    += rented_tokens;
         // add payment to total_unlent
         rt.total_unlent.amount  += payment.amount;
         rt.total_lendable.amount = rt.total_unlent.amount + rt.total_lent.amount;
         // increment loan_num if a new loan is being created
         if ( new_loan ) {
            rt.loan_num++;
         }
      });
   }

   /**
    * @brief Updates rex_pool balances upon closing an expired loan
    *
    * @param loan - loan to be closed
    */
   void system_contract::remove_loan_from_rex_pool( const rex_loan& loan )
   {
      const auto& pool = _rexpool.begin();
      const int64_t delta_total_rent = exchange_state::get_bancor_output( pool->total_unlent.amount,
                                                                          pool->total_rent.amount,
                                                                          loan.total_staked.amount );
      _rexpool.modify( pool, same_payer, [&]( auto& rt ) {
         // deduct calculated delta_total_rent from total_rent
         rt.total_rent.amount    -= delta_total_rent;
         // move rented tokens from total_lent to total_unlent
         rt.total_unlent.amount  += loan.total_staked.amount;
         rt.total_lent.amount    -= loan.total_staked.amount;
         rt.total_lendable.amount = rt.total_unlent.amount + rt.total_lent.amount;
      });
   }

   /**
    * @brief Updates the fields of an existing loan that is being renewed
    */
   template <typename Index, typename Iterator>
   int64_t system_contract::update_renewed_loan( Index& idx, const Iterator& itr, int64_t rented_tokens )
   {
      int64_t delta_stake = rented_tokens - itr->total_staked.amount;
      idx.modify ( itr, same_payer, [&]( auto& loan ) {
         loan.total_staked.amount = rented_tokens;
         loan.expiration         += eosio::days(30);
         loan.balance.amount     -= loan.payment.amount;
      });
      return delta_stake;
   }

   /**
    * @brief Performs maintenance operations on expired NET and CPU loans and sellrex orders
    *
    * @param max - maximum number of each of the three categories to be processed
    */
   void system_contract::runrex( uint16_t max )
   {
      check( rex_system_initialized(), "rex system not initialized yet" );

      const auto& pool = _rexpool.begin();

      auto process_expired_loan = [&]( auto& idx, const auto& itr ) -> std::pair<bool, int64_t> {
         /// update rex_pool in order to delete existing loan
         remove_loan_from_rex_pool( *itr );
         bool    delete_loan   = false;
         int64_t delta_stake   = 0;
         /// calculate rented tokens at current price
         int64_t rented_tokens = exchange_state::get_bancor_output( pool->total_rent.amount,
                                                                    pool->total_unlent.amount,
                                                                    itr->payment.amount );
         /// conditions for loan renewal
         bool renew_loan = itr->payment <= itr->balance        /// loan has sufficient balance 
                        && itr->payment.amount < rented_tokens /// loan has favorable return 
                        && rex_loans_available();              /// no pending sell orders
         if ( renew_loan ) {
            /// update rex_pool in order to account for renewed loan 
            add_loan_to_rex_pool( itr->payment, rented_tokens, false );
            /// update renewed loan fields
            delta_stake = update_renewed_loan( idx, itr, rented_tokens );
         } else {
            delete_loan = true;
            delta_stake = -( itr->total_staked.amount );
            /// refund "from" account if the closed loan balance is positive
            if ( itr->balance.amount > 0 ) {
               transfer_to_fund( itr->from, itr->balance );
            }
         }

         return { delete_loan, delta_stake };
      };

      /// transfer from eosio.names to eosio.rex
      if ( pool->namebid_proceeds.amount > 0 ) {
         channel_to_rex( names_account, pool->namebid_proceeds );
         _rexpool.modify( pool, same_payer, [&]( auto& rt ) {
            rt.namebid_proceeds.amount = 0;
         });
      }

      /// process cpu loans
      {
         rex_cpu_loan_table cpu_loans( _self, _self.value );
         auto cpu_idx = cpu_loans.get_index<"byexpr"_n>();
         for ( uint16_t i = 0; i < max; ++i ) {
            auto itr = cpu_idx.begin();
            if ( itr == cpu_idx.end() || itr->expiration > current_time_point() ) break;

            auto result = process_expired_loan( cpu_idx, itr );
            if ( result.second != 0 )
               update_resource_limits( itr->from, itr->receiver, 0, result.second );

            if ( result.first )
               cpu_idx.erase( itr );
         }
      }

      /// process net loans
      {
         rex_net_loan_table net_loans( _self, _self.value );
         auto net_idx = net_loans.get_index<"byexpr"_n>();
         for ( uint16_t i = 0; i < max; ++i ) {
            auto itr = net_idx.begin();
            if ( itr == net_idx.end() || itr->expiration > current_time_point() ) break;

            auto result = process_expired_loan( net_idx, itr );
            if ( result.second != 0 )
               update_resource_limits( itr->from, itr->receiver, result.second, 0 );

            if ( result.first )
               net_idx.erase( itr );
         }
      }

      /// process sellrex orders
      if ( _rexorders.begin() != _rexorders.end() ) {
         auto idx  = _rexorders.get_index<"bytime"_n>();
         auto oitr = idx.begin();
         for ( uint16_t i = 0; i < max; ++i ) {
            if ( oitr == idx.end() || !oitr->is_open ) break;
            auto next = oitr;
            ++next;
            auto bitr = _rexbalance.find( oitr->owner.value );
            if ( bitr != _rexbalance.end() ) { // should always be true
               auto result = fill_rex_order( bitr, oitr->rex_requested );
               if ( result.success ) {
                  const name order_owner = oitr->owner;
                  idx.modify( oitr, same_payer, [&]( auto& order ) {
                     order.proceeds.amount     = result.proceeds.amount;
                     order.stake_change.amount = result.stake_change.amount;
                     order.close();
                  });
                  /// send dummy action to show owner and proceeds of filled sellrex order
                  rex_results::orderresult_action order_act( rex_account, std::vector<eosio::permission_level>{ } );
                  order_act.send( order_owner, result.proceeds );
               }
            }
            oitr = next;
         }
      }

   }

   template <typename T>
   int64_t system_contract::rent_rex( T& table, const name& from, const name& receiver, const asset& payment, const asset& fund )
   {
      runrex(2);

      check( rex_loans_available(), "rex loans are currently not available" );
      check( payment.symbol == core_symbol() && fund.symbol == core_symbol(), "must use core token" );
      check( 0 < payment.amount && 0 <= fund.amount, "must use positive asset amount" );

      transfer_from_fund( from, payment + fund );

      const auto& pool = _rexpool.begin(); /// already checked that _rexpool.begin() != _rexpool.end() in rex_loans_available()

      int64_t rented_tokens = exchange_state::get_bancor_output( pool->total_rent.amount,
                                                                 pool->total_unlent.amount,
                                                                 payment.amount );
      check( payment.amount < rented_tokens, "loan price does not favor renting" );
      add_loan_to_rex_pool( payment, rented_tokens, true );

      table.emplace( from, [&]( auto& c ) {
         c.from         = from;
         c.receiver     = receiver;
         c.payment      = payment;
         c.balance      = fund;
         c.total_staked = asset( rented_tokens, core_symbol() );
         c.expiration   = current_time_point() + eosio::days(30);
         c.loan_num     = pool->loan_num;
      });

      rex_results::rentresult_action rentresult_act{ rex_account, std::vector<eosio::permission_level>{ } };
      rentresult_act.send( asset{ rented_tokens, core_symbol() } );
      return rented_tokens;
   }

   /**
    * @brief Processes a sellrex order and returns object containing the results
    *
    * Processes an incoming or already scheduled sellrex order. If REX pool has enough core
    * tokens not frozen in loans, order is filled. In this case, REX pool totals, user rex_balance
    * and user vote_stake are updated. However, this function does not update user voting power. The
    * function returns success flag, order proceeds, and vote stake delta. These are used later in a
    * different function to complete order processing, i.e. transfer proceeds to user REX fund and
    * update user vote weight.
    *
    * @param bitr - iterator pointing to rex_balance database record
    * @param rex - amount of rex to be sold
    *
    * @return rex_order_outcome - a struct containing success flag, order proceeds, and resultant
    * vote stake change
    */
   rex_order_outcome system_contract::fill_rex_order( const rex_balance_table::const_iterator& bitr, const asset& rex )
   {
      auto rexitr = _rexpool.begin();
      const int64_t S0 = rexitr->total_lendable.amount;
      const int64_t R0 = rexitr->total_rex.amount;
      const int64_t p  = (uint128_t(rex.amount) * S0) / R0;
      const int64_t R1 = R0 - rex.amount;
      const int64_t S1 = S0 - p;
      asset proceeds( p, core_symbol() );
      asset stake_change( 0, core_symbol() );
      bool  success = false;

      const int64_t unlent_lower_bound = ( uint128_t(2) * rexitr->total_lent.amount ) / 10;
      const int64_t available_unlent   = rexitr->total_unlent.amount - unlent_lower_bound; // available_unlent <= 0 is possible
      if ( proceeds.amount <= available_unlent ) {
         const int64_t init_vote_stake_amount = bitr->vote_stake.amount;
         const int64_t current_stake_value    = ( uint128_t(bitr->rex_balance.amount) * S0 ) / R0;
         _rexpool.modify( rexitr, same_payer, [&]( auto& rt ) {
            rt.total_rex.amount      = R1;
            rt.total_lendable.amount = S1;
            rt.total_unlent.amount   = rt.total_lendable.amount - rt.total_lent.amount;
         });
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rb.vote_stake.amount   = current_stake_value - proceeds.amount;
            rb.rex_balance.amount -= rex.amount;
            rb.matured_rex        -= rex.amount;
         });
         stake_change.amount = bitr->vote_stake.amount - init_vote_stake_amount;
         success = true;
      } else {
         proceeds.amount = 0;
      }

      return { success, proceeds, stake_change };
   }

   template <typename T>
   void system_contract::fund_rex_loan( T& table, const name& from, uint64_t loan_num, const asset& payment  )
   {
      check( payment.symbol == core_symbol(), "must use core token" );
      transfer_from_fund( from, payment );
      auto itr = table.require_find( loan_num, "loan not found" );
      check( itr->from == from, "user must be loan creator" );
      check( itr->expiration > current_time_point(), "loan has already expired" );
      table.modify( itr, same_payer, [&]( auto& loan ) {
         loan.balance.amount += payment.amount;
      });
   }

   template <typename T>
   void system_contract::defund_rex_loan( T& table, const name& from, uint64_t loan_num, const asset& amount  )
   {
      check( amount.symbol == core_symbol(), "must use core token" );
      auto itr = table.require_find( loan_num, "loan not found" );
      check( itr->from == from, "user must be loan creator" );
      check( itr->expiration > current_time_point(), "loan has already expired" );
      check( itr->balance >= amount, "insufficent loan balance" );
      table.modify( itr, same_payer, [&]( auto& loan ) {
         loan.balance.amount -= amount.amount;
      });
      transfer_to_fund( from, amount );
   }

   /**
    * @brief Transfers tokens from owner REX fund
    *
    * @pre - owner REX fund has sufficient balance
    *
    * @param owner - owner account name
    * @param amount - tokens to be transfered out of REX fund
    */
   void system_contract::transfer_from_fund( const name& owner, const asset& amount )
   {
      check( 0 < amount.amount && amount.symbol == core_symbol(), "must transfer positive amount from REX fund" );
      auto itr = _rexfunds.require_find( owner.value, "must deposit to REX fund first" );
      check( amount <= itr->balance, "insufficient funds" );
      _rexfunds.modify( itr, same_payer, [&]( auto& fund ) {
         fund.balance.amount -= amount.amount;
      });
   }

   /**
    * @brief Transfers tokens to owner REX fund
    *
    * @param owner - owner account name
    * @param amount - tokens to be transfered to REX fund
    */
   void system_contract::transfer_to_fund( const name& owner, const asset& amount )
   {
      check( 0 < amount.amount && amount.symbol == core_symbol(), "must transfer positive amount to REX fund" );
      auto itr = _rexfunds.find( owner.value );
      if ( itr == _rexfunds.end() ) {
         _rexfunds.emplace( owner, [&]( auto& fund ) {
            fund.owner   = owner;
            fund.balance = amount;
         });
      } else {
         _rexfunds.modify( itr, same_payer, [&]( auto& fund ) {
            fund.balance.amount += amount.amount;
         });
      }
   }

   /**
    * @brief Processes owner filled sellrex order and updates vote weight
    *
    * Checks if user has a scheduled sellrex order that has been filled, completes its processing,
    * and deletes it. Processing entails transfering proceeds to user REX fund and updating user
    * vote weight. Additional proceeds and stake change can be passed as arguments. This function
    * is called only by actions pushed by owner.
    *
    * @param owner - owner account name
    * @param proceeds - additional proceeds to be transfered to owner REX fund
    * @param delta_stake - additional stake to be added to owner vote weight
    * @param force_vote_update - if true, vote weight is updated even if vote stake didn't change
    *
    * @return asset - REX amount of owner unfilled sell order if one exists
    */
   asset system_contract::update_rex_account( const name& owner, const asset& proceeds, const asset& delta_stake, bool force_vote_update )
   {
      asset to_fund( proceeds );
      asset to_stake( delta_stake );
      asset rex_in_sell_order( 0, rex_symbol );
      auto itr = _rexorders.find( owner.value );
      if ( itr != _rexorders.end() ) {
         if ( itr->is_open ) {
            rex_in_sell_order.amount = itr->rex_requested.amount;
         } else {
            to_fund.amount  += itr->proceeds.amount;
            to_stake.amount += itr->stake_change.amount;
            _rexorders.erase( itr );
         }
      }

      if ( to_fund.amount > 0 )
         transfer_to_fund( owner, to_fund );
      if ( force_vote_update || to_stake.amount != 0 )
         update_voting_power( owner, to_stake );

      return rex_in_sell_order;
   }

   /**
    * @brief Channels system fees to REX pool
    *
    * @param from - account from which asset is transfered to REX pool
    * @param amount - amount of tokens to be transfered
    */
   void system_contract::channel_to_rex( const name& from, const asset& amount )
   {
#if CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX
      if ( rex_available() ) {
         _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& rp ) {
            rp.total_unlent.amount   += amount.amount;
            rp.total_lendable.amount += amount.amount;
         });
         // inline transfer to rex_account
         token::transfer_action transfer_act{ token_account, { from, active_permission } };
         transfer_act.send( from, rex_account, amount,
                            std::string("transfer from ") + from.to_string() + " to eosio.rex" );
      }
#endif
   }

   /**
    * @brief Updates namebid proceeds to be transfered to REX pool
    *
    * @param highest_bid - highest bidding amount of closed namebid
    */
   void system_contract::channel_namebid_to_rex( const int64_t highest_bid )
   {
#if CHANNEL_RAM_AND_NAMEBID_FEES_TO_REX
      if ( rex_available() ) {
         _rexpool.modify( _rexpool.begin(), same_payer, [&]( auto& rp ) {
            rp.namebid_proceeds.amount += highest_bid;
         });
      }
#endif
   }

   /**
    * @brief Calculates maturity time of purchased REX tokens which is 4 days from end
    * of the day UTC
    *
    * @return time_point_sec
    */
   time_point_sec system_contract::get_rex_maturity()
   {
      const uint32_t num_of_maturity_buckets = 5;
      static const uint32_t now = current_time_point().sec_since_epoch();
      static const uint32_t r   = now % seconds_per_day;
      static const time_point_sec rms{ now - r + num_of_maturity_buckets * seconds_per_day };
      return rms;
   }

   /**
    * @brief Updates REX owner maturity buckets
    *
    * @param bitr - iterator pointing to rex_balance object
    */
   void system_contract::process_rex_maturities( const rex_balance_table::const_iterator& bitr )
   {
      const time_point_sec now = current_time_point();
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         while ( !rb.rex_maturities.empty() && rb.rex_maturities.front().first <= now ) {
            rb.matured_rex += rb.rex_maturities.front().second;
            rb.rex_maturities.pop_front();
         }
      });
   }

   /**
    * @brief Consolidates REX maturity buckets into one
    *
    * @param bitr - iterator pointing to rex_balance object
    * @param rex_in_sell_order - REX tokens in owner unfilled sell order, if one exists
    */
   void system_contract::consolidate_rex_balance( const rex_balance_table::const_iterator& bitr,
                                                  const asset& rex_in_sell_order )
   {
      const int64_t rex_in_savings = read_rex_savings( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         int64_t total  = rb.matured_rex - rex_in_sell_order.amount;
         rb.matured_rex = rex_in_sell_order.amount;
         while ( !rb.rex_maturities.empty() ) {
            total += rb.rex_maturities.front().second;
            rb.rex_maturities.pop_front();
         }
         if ( total > 0 ) {
            rb.rex_maturities.emplace_back( get_rex_maturity(), total );
         }
      });
      put_rex_savings( bitr, rex_in_savings );
   }

   /**
    * @brief Updates REX pool balances upon REX purchase
    *
    * @param payment - amount of core tokens paid
    *
    * @return asset - calculated amount of REX tokens purchased
    */
   asset system_contract::add_to_rex_pool( const asset& payment )
   {
      /**
       * If CORE_SYMBOL is (EOS,4), maximum supply is 10^10 tokens (10 billion tokens), i.e., maximum amount
       * of indivisible units is 10^14. rex_ratio = 10^4 sets the upper bound on (REX,4) indivisible units to
       * 10^18 and that is within the maximum allowable amount field of asset type which is set to 2^62
       * (approximately 4.6 * 10^18). For a different CORE_SYMBOL, and in order for maximum (REX,4) amount not
       * to exceed that limit, maximum amount of indivisible units cannot be set to a value larger than 4 * 10^14.
       * If precision of CORE_SYMBOL is 4, that corresponds to a maximum supply of 40 billion tokens.
       */
      const int64_t rex_ratio = 10000;
      const asset   init_total_rent( 20'000'0000, core_symbol() ); /// base balance prevents renting profitably until at least a minimum number of core_symbol() is made available
      asset rex_received( 0, rex_symbol );
      auto itr = _rexpool.begin();
      if ( !rex_system_initialized() ) {
         /// initialize REX pool
         _rexpool.emplace( _self, [&]( auto& rp ) {
            rex_received.amount = payment.amount * rex_ratio;
            rp.total_lendable   = payment;
            rp.total_lent       = asset( 0, core_symbol() );
            rp.total_unlent     = rp.total_lendable - rp.total_lent;
            rp.total_rent       = init_total_rent;
            rp.total_rex        = rex_received;
            rp.namebid_proceeds = asset( 0, core_symbol() );
         });
      } else if ( !rex_available() ) { /// should be a rare corner case, REX pool is initialized but empty
         _rexpool.modify( itr, same_payer, [&]( auto& rp ) {
            rex_received.amount      = payment.amount * rex_ratio;
            rp.total_lendable.amount = payment.amount;
            rp.total_lent.amount     = 0;
            rp.total_unlent.amount   = rp.total_lendable.amount - rp.total_lent.amount;
            rp.total_rent.amount     = init_total_rent.amount;
            rp.total_rex.amount      = rex_received.amount;
         });
      } else {
         /// total_lendable > 0 if total_rex > 0 except in a rare case and due to rounding errors
         check( itr->total_lendable.amount > 0, "lendable REX pool is empty" );
         const int64_t S0 = itr->total_lendable.amount;
         const int64_t S1 = S0 + payment.amount;
         const int64_t R0 = itr->total_rex.amount;
         const int64_t R1 = (uint128_t(S1) * R0) / S0;
         rex_received.amount = R1 - R0;
         _rexpool.modify( itr, same_payer, [&]( auto& rp ) {
            rp.total_lendable.amount = S1;
            rp.total_rex.amount      = R1;
            rp.total_unlent.amount   = rp.total_lendable.amount - rp.total_lent.amount;
            check( rp.total_unlent.amount >= 0, "programmer error, this should never go negative" );
         });
      }

      return rex_received;
   }

   /**
    * @brief Updates owner REX balance upon buying REX tokens
    *
    * @param owner - account name of REX owner
    * @param payment - amount core tokens paid to buy REX
    * @param rex_received - amount of purchased REX tokens
    *
    * @return asset - change in owner REX vote stake
    */
   asset system_contract::add_to_rex_balance( const name& owner, const asset& payment, const asset& rex_received )
   {
      asset init_rex_stake( 0, core_symbol() );
      asset current_rex_stake( 0, core_symbol() );
      auto bitr = _rexbalance.find( owner.value );
      if ( bitr == _rexbalance.end() ) {
         bitr = _rexbalance.emplace( owner, [&]( auto& rb ) {
            rb.owner       = owner;
            rb.vote_stake  = payment;
            rb.rex_balance = rex_received;
         });
         current_rex_stake.amount = payment.amount;
      } else {
         init_rex_stake.amount = bitr->vote_stake.amount;
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rb.rex_balance.amount += rex_received.amount;
            rb.vote_stake.amount   = ( uint128_t(rb.rex_balance.amount) * _rexpool.begin()->total_lendable.amount )
                                     / _rexpool.begin()->total_rex.amount;
         });
         current_rex_stake.amount = bitr->vote_stake.amount;
      }

      const int64_t rex_in_savings = read_rex_savings( bitr );
      process_rex_maturities( bitr );
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         const time_point_sec maturity = get_rex_maturity();
         if ( !rb.rex_maturities.empty() && rb.rex_maturities.back().first == maturity ) {
            rb.rex_maturities.back().second += rex_received.amount;
         } else {
            rb.rex_maturities.emplace_back( maturity, rex_received.amount );
         }
      });
      put_rex_savings( bitr, rex_in_savings );
      return current_rex_stake - init_rex_stake;
   }

   /**
    * @brief Reads amount of REX in savings bucket and removes the bucket from maturities
    *
    * Reads and (temporarily) removes REX savings bucket from REX maturities in order to
    * allow uniform processing of remaining buckets as savings is a special case. This 
    * function is used in conjunction with put_rex_savings.
    *
    * @param bitr - iterator pointing to rex_balance object
    *
    * @return int64_t - amount of REX in savings bucket
    */
   int64_t system_contract::read_rex_savings( const rex_balance_table::const_iterator& bitr )
   {
      int64_t rex_in_savings = 0;
      static const time_point_sec end_of_days = time_point_sec::maximum();
      if ( !bitr->rex_maturities.empty() && bitr->rex_maturities.back().first == end_of_days ) {
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rex_in_savings = rb.rex_maturities.back().second;
            rb.rex_maturities.pop_back();
         });
      }
      return rex_in_savings;
   }

   /**
    * @brief Adds a specified REX amount to savings bucket
    *
    * @param bitr - iterator pointing to rex_balance object
    * @param rex - amount of REX to be added
    */
   void system_contract::put_rex_savings( const rex_balance_table::const_iterator& bitr, int64_t rex )
   {
      if ( rex == 0 ) return;
      static const time_point_sec end_of_days = time_point_sec::maximum();
      _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
         if ( !rb.rex_maturities.empty() && rb.rex_maturities.back().first == end_of_days ) {
            rb.rex_maturities.back().second += rex;
         } else {
            rb.rex_maturities.emplace_back( end_of_days, rex );
         }
      });
   }

   /**
    * @brief Updates voter REX vote stake to the current value of REX tokens held
    *
    * @param voter - account name of voter
    */
   void system_contract::update_rex_stake( const name& voter )
   {
      int64_t delta_stake = 0;
      auto bitr = _rexbalance.find( voter.value );
      if ( bitr != _rexbalance.end() && rex_available() ) {
         asset init_vote_stake = bitr->vote_stake;
         asset current_vote_stake( 0, core_symbol() );
         current_vote_stake.amount = ( uint128_t(bitr->rex_balance.amount) * _rexpool.begin()->total_lendable.amount )
                                     / _rexpool.begin()->total_rex.amount;
         _rexbalance.modify( bitr, same_payer, [&]( auto& rb ) {
            rb.vote_stake.amount = current_vote_stake.amount; 
         });
         delta_stake = current_vote_stake.amount - init_vote_stake.amount;
      }

      if ( delta_stake != 0 ) {
         auto vitr = _voters.find( voter.value );
         if ( vitr != _voters.end() ) {
            _voters.modify( vitr, same_payer, [&]( auto& vinfo ) {
               vinfo.staked += delta_stake;
            });
         }
      }
   }

}; /// namespace eosiosystem
