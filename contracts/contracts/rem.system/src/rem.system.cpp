/**
 *  @copyright defined in eos/LICENSE.txt
 */


#include <eosio/crypto.hpp>
#include <eosio/dispatcher.hpp>

#include <rem.system/rem.system.hpp>
#include <rem.attr/rem.attr.hpp>

#include "producer_pay.cpp"
#include "delegate_bandwidth.cpp"
#include "voting.cpp"
#include "exchange_state.cpp"
#include "rex.cpp"

namespace eosiosystem {

   const int64_t  inflation_precision           = 100;     // 2 decimals
   const int64_t  default_annual_rate           = 0;       // 0% annual rate
   const int64_t  default_inflation_pay_factor  = 5;       // 20% of the inflation
   const int64_t  default_votepay_factor        = 4;       // 25% of the producer pay

   double get_continuous_rate(int64_t annual_rate) {
      return std::log(double(1)+double(annual_rate)/double(100*inflation_precision));
   }


   system_contract::system_contract( name s, name code, datastream<const char*> ds )
   :native(s,code,ds),
    _voters(_self, _self.value),
    _producers(_self, _self.value),
    _producers2(_self, _self.value),
    _global(_self, _self.value),
    _global2(_self, _self.value),
    _global3(_self, _self.value),
    _global4(_self, _self.value),
    _globalrem(_self, _self.value),
    _rammarket(_self, _self.value),
    _rexpool(_self, _self.value),
    _rexfunds(_self, _self.value),
    _rexbalance(_self, _self.value),
    _rexorders(_self, _self.value)
   {
      //print( "construct system\n" );
      _gstate  = _global.exists() ? _global.get() : get_default_parameters();
      _gstate2 = _global2.exists() ? _global2.get() : eosio_global_state2{};
      _gstate3 = _global3.exists() ? _global3.get() : eosio_global_state3{};
      _gstate4 = _global4.exists() ? _global4.get() : get_default_inflation_parameters();
      _gremstate = _globalrem.exists() ? _globalrem.get() : get_default_rem_parameters();
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }

   eosio_global_state4 system_contract::get_default_inflation_parameters() {
      eosio_global_state4 gs4;
      gs4.continuous_rate      = get_continuous_rate(default_annual_rate);
      gs4.inflation_pay_factor = default_inflation_pay_factor;
      gs4.votepay_factor       = default_votepay_factor;
      return gs4;
   }

   eosio_global_rem_state system_contract::get_default_rem_parameters() {
      eosio_global_rem_state gs;
      gs.per_stake_share = 0.6;
      gs.per_vote_share = 0.3;

      return gs;
   }

   symbol system_contract::core_symbol()const {
      const static auto sym = get_core_symbol( _rammarket );
      return sym;
   }

   system_contract::~system_contract() {
      _global.set( _gstate, _self );
      _global2.set( _gstate2, _self );
      _global3.set( _gstate3, _self );
      _global4.set( _gstate4, _self );
      _globalrem.set( _gremstate, _self );
   }

   void system_contract::setrwrdratio( double stake_share, double vote_share ) {
      require_auth(_self);

      check(stake_share > 0, "share must be positive");
      check(vote_share > 0, "share must be positive");
      check(stake_share + vote_share < 1.0, "perstake and pervote shares together must be less than 1.0");
      _gremstate.per_stake_share = stake_share;
      _gremstate.per_vote_share = vote_share;
   }

   void system_contract::setlockperiod( uint64_t period_in_days ) {
      require_auth(_self);

      check(period_in_days != 0, "lock period cannot be zero");
      _gremstate.stake_lock_period = eosio::days(period_in_days);
   }

   void system_contract::setunloperiod( uint64_t period_in_days ) {
      require_auth(_self);

      check(period_in_days != 0, "unlock period cannot be zero");
      _gremstate.stake_unlock_period = eosio::days(period_in_days);
   }

   void system_contract::setgiftcontra( name value ) {
      require_auth(_self);

      _gremstate.gifter_attr_contract = value;
   }

   void system_contract::setgiftiss( name value ) {
      require_auth(_self);

      _gremstate.gifter_attr_issuer = value;
   }

   void system_contract::setgiftattr( name value ) {
      require_auth(_self);

      _gremstate.gifter_attr_name = value;
   }

   void system_contract::setminstake( uint64_t min_account_stake ) {
      require_auth( _self );

      _gstate.min_account_stake = min_account_stake;
   }

   void system_contract::setram( uint64_t max_ram_size ) {
      require_auth( _self );

      check( _gstate.max_ram_size < max_ram_size, "ram may only be increased" ); /// decreasing ram might result market maker issues
      check( max_ram_size < 1024ll*1024*1024*1024*1024, "ram size is unrealistic" );
      check( max_ram_size > _gstate.total_ram_bytes_reserved, "attempt to set max below reserved" );

      auto delta = int64_t(max_ram_size) - int64_t(_gstate.max_ram_size);
      auto itr = _rammarket.find(ramcore_symbol.raw());

      /**
       *  Increase the amount of ram for sale based upon the change in max ram size.
       */
      _rammarket.modify( itr, same_payer, [&]( auto& m ) {
         m.base.balance.amount += delta;
      });

      _gstate.max_ram_size = max_ram_size;
   }

   void system_contract::update_ram_supply() {
      auto cbt = current_block_time();

      if( cbt <= _gstate2.last_ram_increase ) return;

      auto itr = _rammarket.find(ramcore_symbol.raw());
      auto new_ram = (cbt.slot - _gstate2.last_ram_increase.slot)*_gstate2.new_ram_per_block;
      _gstate.max_ram_size += new_ram;

      /**
       *  Increase the amount of ram for sale based upon the change in max ram size.
       */
      _rammarket.modify( itr, same_payer, [&]( auto& m ) {
         m.base.balance.amount += new_ram;
      });
      _gstate2.last_ram_increase = cbt;
   }

   void system_contract::setramrate( uint16_t bytes_per_block ) {
      require_auth( _self );

      update_ram_supply();
      _gstate2.new_ram_per_block = bytes_per_block;
   }

   void system_contract::setparams( const eosio::blockchain_parameters& params ) {
      require_auth( _self );
      (eosio::blockchain_parameters&)(_gstate) = params;
      check( 3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3" );
      set_blockchain_parameters( params );
   }

   void system_contract::setpriv( const name& account, uint8_t ispriv ) {
      require_auth( _self );
      set_privileged( account, ispriv );
   }

   void system_contract::setalimits( const name& account, int64_t ram, int64_t net, int64_t cpu ) {
      require_auth( _self );

      user_resources_table userres( _self, account.value );
      auto ritr = userres.find( account.value );
      check( ritr == userres.end(), "only supports unlimited accounts" );

      auto vitr = _voters.find( account.value );
      if( vitr != _voters.end() ) {
         bool ram_managed = has_field( vitr->flags1, voter_info::flags1_fields::ram_managed );
         bool net_managed = has_field( vitr->flags1, voter_info::flags1_fields::net_managed );
         bool cpu_managed = has_field( vitr->flags1, voter_info::flags1_fields::cpu_managed );
         check( !(ram_managed || net_managed || cpu_managed), "cannot use setalimits on an account with managed resources" );
      }

      set_resource_limits( account, ram, net, cpu );
   }

   void system_contract::activate( const eosio::checksum256& feature_digest ) {
      require_auth( get_self() );
      preactivate_feature( feature_digest );
   }

   void system_contract::rmvproducer( const name& producer ) {
      require_auth( _self );
      auto prod = _producers.find( producer.value );
      check( prod != _producers.end(), "producer not found" );
      if (prod->active()) {
         user_resources_table totals_tbl( _self, producer.value );
         const auto& tot = totals_tbl.get(producer.value, "producer must have resources");
         _gstate.total_producer_stake -= tot.own_stake_amount;
      }
      _producers.modify( prod, same_payer, [&](auto& p) {
            p.deactivate();
         });
   }

   void system_contract::updtrevision( uint8_t revision ) {
      require_auth( _self );
      check( _gstate2.revision < 255, "can not increment revision" ); // prevent wrap around
      check( revision == _gstate2.revision + 1, "can only increment revision by one" );
      check( revision <= 1, // set upper bound to greatest revision supported in the code
                    "specified revision is not yet supported by the code" );
      _gstate2.revision = revision;
   }

   void system_contract::bidname( const name& bidder, const name& newname, const asset& bid ) {
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
      name_bid_table bids(_self, _self.value);
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

         bid_refund_table refunds_table(_self, newname.value);

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

         transaction t;
         t.actions.emplace_back( permission_level{_self, active_permission},
                                 _self, "bidrefund"_n,
                                 std::make_tuple( current->high_bidder, newname )
         );
         t.delay_sec = 0;
         uint128_t deferred_id = (uint128_t(newname.value) << 64) | current->high_bidder.value;
         cancel_deferred( deferred_id );
         t.send( deferred_id, bidder );

         bids.modify( current, bidder, [&]( auto& b ) {
            b.high_bidder = bidder;
            b.high_bid = bid.amount;
            b.last_bid_time = current_time_point();
         });
      }
   }

   void system_contract::bidrefund( const name& bidder, const name& newname ) {
      bid_refund_table refunds_table(_self, newname.value);
      auto it = refunds_table.find( bidder.value );
      check( it != refunds_table.end(), "refund not found" );

      token::transfer_action transfer_act{ token_account, { {names_account, active_permission}, {bidder, active_permission} } };
      transfer_act.send( names_account, bidder, asset(it->amount), std::string("refund bid on name ")+(name{newname}).to_string() );
      refunds_table.erase( it );
   }

   void system_contract::setinflation( int64_t annual_rate, int64_t inflation_pay_factor, int64_t votepay_factor ) {
      require_auth(_self);
      check(annual_rate >= 0, "annual_rate can't be negative");
      check(inflation_pay_factor > 0, "inflation_pay_factor must be positive");
      check(votepay_factor > 0, "votepay_factor must be positive");

      _gstate4.continuous_rate      = get_continuous_rate(annual_rate);
      _gstate4.inflation_pay_factor = inflation_pay_factor;
      _gstate4.votepay_factor       = votepay_factor;
      _global4.set( _gstate4, _self );
   }

   /**
    *  Called after a new account is created. This code enforces resource-limits rules
    *  for new accounts as well as new account naming conventions.
    *
    *  Account names containing '.' symbols must have a suffix equal to the name of the creator.
    *  This allows users who buy a premium name (shorter than 12 characters with no dots) to be the only ones
    *  who can create accounts with the creator's name as a suffix.
    *
    */
   void system_contract::newaccount( const name&       creator,
                            const name&       newact,
                            ignore<authority> owner,
                            ignore<authority> active ) {

      if( creator != _self ) {
         uint64_t tmp = newact.value >> 4;
         bool has_dot = false;

         for( uint32_t i = 0; i < 12; ++i ) {
           has_dot |= !(tmp & 0x1f);
           tmp >>= 5;
         }
         if( has_dot ) { // or is less than 12 characters
            auto suffix = newact.suffix();
            if( suffix == newact ) {
               name_bid_table bids(_self, _self.value);
               auto current = bids.find( newact.value );
               check( current != bids.end(), "no active bid for name" );
               check( current->high_bidder == creator, "only highest bidder can claim" );
               check( current->high_bid < 0, "auction for name is not closed yet" );
               bids.erase( current );
            } else {
               check( creator == suffix, "only suffix may create this account" );
            }
         }
      }

      user_resources_table  userres( _self, newact.value);

      int64_t free_stake_amount = 0;
      int64_t free_gift_bytes   = 0;

      if ( eosio::attribute::has_attribute( _gremstate.gifter_attr_contract, _gremstate.gifter_attr_issuer, creator, _gremstate.gifter_attr_name ) ) {
         const auto system_token_max_supply = eosio::token::get_max_supply(token_account, system_contract::get_core_symbol().code() );
         const double bytes_per_token       = (double)_gstate.max_ram_size / (double)system_token_max_supply.amount;
         free_stake_amount                  = _gstate.min_account_stake;
         free_gift_bytes                    = bytes_per_token * free_stake_amount;
      }

      userres.emplace( newact, [&]( auto& res ) {
        res.owner = newact;
        res.net_weight = asset( 0, system_contract::get_core_symbol() );
        res.cpu_weight = asset( 0, system_contract::get_core_symbol() );
        res.free_stake_amount = free_stake_amount;
      });

      set_resource_limits( newact, free_gift_bytes, 0, 0 );
   }

   void native::setabi( const name& acnt, const std::vector<char>& abi ) {
      eosio::multi_index< "abihash"_n, abi_hash >  table(_self, _self.value);
      auto itr = table.find( acnt.value );
      if( itr == table.end() ) {
         table.emplace( acnt, [&]( auto& row ) {
            row.owner = acnt;
            row.hash  = sha256(const_cast<char*>(abi.data()), abi.size());
         });
      } else {
         table.modify( itr, same_payer, [&]( auto& row ) {
            row.hash = sha256(const_cast<char*>(abi.data()), abi.size());
         });
      }
   }

   void system_contract::init( unsigned_int version, const symbol& core ) {
      require_auth( _self );
      check( version.value == 0, "unsupported version for init action" );

      auto itr = _rammarket.find(ramcore_symbol.raw());
      check( itr == _rammarket.end(), "system contract has already been initialized" );

      auto system_token_supply   = eosio::token::get_supply(token_account, core.code() );
      check( system_token_supply.symbol == core, "specified core symbol does not exist (precision mismatch)" );

      check( system_token_supply.amount > 0, "system token supply must be greater than 0" );
      _rammarket.emplace( _self, [&]( auto& m ) {
         m.supply.amount = 100000000000000ll;
         m.supply.symbol = ramcore_symbol;
         m.base.balance.amount = int64_t(_gstate.free_ram());
         m.base.balance.symbol = ram_symbol;
         m.quote.balance.amount = system_token_supply.amount / 1000;
         m.quote.balance.symbol = core;
      });

      token::open_action open_act{ token_account, { {_self, active_permission} } };
      open_act.send( rex_account, core, _self );
   }
         
   bool system_contract::vote_is_reasserted( eosio::time_point last_reassertion_time ) const {
         return (current_time_point() - last_reassertion_time) < _gremstate.reassertion_period;
   }
} /// rem.system


EOSIO_DISPATCH( eosiosystem::system_contract,
     // native.hpp (newaccount definition is actually in rem.system.cpp)
     (newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(canceldelay)(onerror)(setabi)
     // rem.system.cpp
     (init)(setram)(setminstake)(setramrate)(setparams)(setpriv)(setalimits)(setrwrdratio)
     (setlockperiod)(setunloperiod)(activate)(rmvproducer)(updtrevision)(bidname)(bidrefund)(setinflation)
     // rex.cpp
     (deposit)(withdraw)(buyrex)(unstaketorex)(sellrex)(cnclrexorder)(rentcpu)(rentnet)(fundcpuloan)(fundnetloan)
     (defcpuloan)(defnetloan)(updaterex)(consolidate)(mvtosavings)(mvfrsavings)(setrex)(rexexec)(closerex)
     // delegate_bandwidth.cpp
     (delegatebw)(undelegatebw)(refund)
     // voting.cpp
     (regproducer)(unregprod)(voteproducer)(regproxy)
     // producer_pay.cpp
     (onblock)(claimrewards)(torewards)
)
