#include <rem.system/rem.system.hpp>
#include <rem.token/rem.token.hpp>
#include <rem.attr/rem.attr.hpp>

#include <eosio/crypto.hpp>
#include <eosio/dispatcher.hpp>

#include <cmath>

namespace eosiosystem {

   using eosio::current_time_point;
   using eosio::token;

   double get_continuous_rate(int64_t annual_rate) {
      return std::log1p(double(annual_rate)/double(100*inflation_precision));
   }


   system_contract::system_contract( name s, name code, datastream<const char*> ds )
   :native(s,code,ds),
    _voters(get_self(), get_self().value),
    _producers(get_self(), get_self().value),
    _producers2(get_self(), get_self().value),
    _global(get_self(), get_self().value),
    _global2(get_self(), get_self().value),
    _global3(get_self(), get_self().value),
    _global4(get_self(), get_self().value),
    _globalrem(get_self(), get_self().value),
    _rotation(get_self(), get_self().value),
    _rexpool(get_self(), get_self().value),
    _rexfunds(get_self(), get_self().value),
    _rexbalance(get_self(), get_self().value),
    _rexorders(get_self(), get_self().value)
   {
      //print( "construct system\n" );
      _gstate  = _global.exists() ? _global.get() : get_default_parameters();
      _gstate2 = _global2.exists() ? _global2.get() : eosio_global_state2{};
      _gstate3 = _global3.exists() ? _global3.get() : eosio_global_state3{};
      _gstate4 = _global4.exists() ? _global4.get() : get_default_inflation_parameters();
      _gremstate = _globalrem.exists() ? _globalrem.get() : get_default_rem_parameters();

      _grotation = _rotation.get_or_create(_self, rotation_state{
         .last_rotation_time      = time_point{},
         .rotation_period         = eosio::hours(4),
         .standby_prods_to_rotate = 4
      });
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
      const auto rem_state = eosio_global_rem_state{
         .per_stake_share = 0.6,
         .per_vote_share = 0.3,

         .gifter_attr_contract = name{"rem.attr"},
         .gifter_attr_issuer   = name{"rem.attr"},
         .gifter_attr_name     = name{"accgifter"},

         .guardian_stake_threshold = 250'000'0000LL,

         .stake_lock_period = eosio::days(180),
         .stake_unlock_period = eosio::days(180),

         .reassertion_period = eosio::days( 7 )
      };

      return rem_state;
   }

   symbol system_contract::core_symbol()const {
      const static auto sym = get_core_symbol();
      return sym;
   }

   system_contract::~system_contract() {
      _global.set( _gstate, get_self() );
      _global2.set( _gstate2, get_self() );
      _global3.set( _gstate3, get_self() );
      _global4.set( _gstate4, get_self() );
      _globalrem.set( _gremstate, get_self() );
      _rotation.set( _grotation, get_self() );
   }

   void system_contract::setrwrdratio( double stake_share, double vote_share ) {
      require_auth(get_self());

      check(stake_share > 0, "share must be positive");
      check(vote_share > 0, "share must be positive");
      check(stake_share + vote_share < 1.0, "perstake and pervote shares together must be less than 1.0");
      _gremstate.per_stake_share = stake_share;
      _gremstate.per_vote_share = vote_share;
   }

   void system_contract::setlockperiod( uint64_t period_in_days ) {
      require_auth(get_self());

      check(period_in_days != 0, "lock period cannot be zero");
      _gremstate.stake_lock_period = eosio::days(period_in_days);
   }

   void system_contract::setunloperiod( uint64_t period_in_days ) {
      require_auth(get_self());

      check(period_in_days != 0, "unlock period cannot be zero");
      _gremstate.stake_unlock_period = eosio::days(period_in_days);
   }

   void system_contract::setgiftcontra( name value ) {
      require_auth(get_self());

      _gremstate.gifter_attr_contract = value;
   }

   void system_contract::setgiftiss( name value ) {
      require_auth(get_self());

      _gremstate.gifter_attr_issuer = value;
   }

   void system_contract::setgiftattr( name value ) {
      require_auth(get_self());

      _gremstate.gifter_attr_name = value;
   }

   void system_contract::setminstake( uint64_t min_account_stake ) {
      require_auth( get_self() );

      _gstate.min_account_stake = min_account_stake;
   }

   void system_contract::setram( uint64_t max_ram_size ) {
      require_auth( get_self() );

      check( _gstate.max_ram_size < max_ram_size, "ram may only be increased" ); /// decreasing ram might result market maker issues
      check( max_ram_size < 1024ll*1024*1024*1024*1024, "ram size is unrealistic" );
      check( max_ram_size > _gstate.total_ram_bytes_reserved, "attempt to set max below reserved" );

      _gstate.max_ram_size = max_ram_size;
   }

   void system_contract::update_ram_supply() {
      auto cbt = eosio::current_block_time();

      if( cbt <= _gstate2.last_ram_increase ) return;

      auto new_ram = (cbt.slot - _gstate2.last_ram_increase.slot)*_gstate2.new_ram_per_block;
      _gstate.max_ram_size += new_ram;
      _gstate2.last_ram_increase = cbt;
   }

   void system_contract::setramrate( uint16_t bytes_per_block ) {
      require_auth( get_self() );

      update_ram_supply();
      _gstate2.new_ram_per_block = bytes_per_block;
   }

   void system_contract::setparams( const eosio::blockchain_parameters& params ) {
      require_auth( get_self() );
      (eosio::blockchain_parameters&)(_gstate) = params;
      check( 3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3" );
      set_blockchain_parameters( params );
   }

   void system_contract::setpriv( const name& account, uint8_t ispriv ) {
      require_auth( get_self() );
      set_privileged( account, ispriv );
   }

   void system_contract::setalimits( const name& account, int64_t ram, int64_t net, int64_t cpu ) {
      require_auth( get_self() );

      user_resources_table userres( get_self(), account.value );
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
      require_auth( get_self() );
      auto prod = _producers.get( producer.value, "producer not found" );

      _producers.modify( prod, same_payer, [&](auto& p) {
            p.deactivate();
      });
   }

   void system_contract::updtrevision( uint8_t revision ) {
      require_auth( get_self() );
      check( _gstate2.revision < 255, "can not increment revision" ); // prevent wrap around
      check( revision == _gstate2.revision + 1, "can only increment revision by one" );
      check( revision <= 1, // set upper bound to greatest revision supported in the code
             "specified revision is not yet supported by the code" );
      _gstate2.revision = revision;
   }

   void system_contract::setinflation( int64_t annual_rate, int64_t inflation_pay_factor, int64_t votepay_factor ) {
      require_auth(get_self());
      check(annual_rate >= 0, "annual_rate can't be negative");
      if ( inflation_pay_factor < pay_factor_precision ) {
         check( false, "inflation_pay_factor must not be less than " + std::to_string(pay_factor_precision) );
      }
      if ( votepay_factor < pay_factor_precision ) {
         check( false, "votepay_factor must not be less than " + std::to_string(pay_factor_precision) );
      }
      _gstate4.continuous_rate      = get_continuous_rate(annual_rate);
      _gstate4.inflation_pay_factor = inflation_pay_factor;
      _gstate4.votepay_factor       = votepay_factor;
      _global4.set( _gstate4, get_self() );
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

      if( creator != get_self() ) {
         uint64_t tmp = newact.value >> 4;
         bool has_dot = false;

         for( uint32_t i = 0; i < 12; ++i ) {
           has_dot |= !(tmp & 0x1f);
           tmp >>= 5;
         }
         if( has_dot ) { // or is less than 12 characters
            auto suffix = newact.suffix();
            if( suffix == newact ) {
               name_bid_table bids(get_self(), get_self().value);
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

      user_resources_table  userres( get_self(), newact.value );

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
      eosio::multi_index< "abihash"_n, abi_hash >  table(get_self(), get_self().value);
      auto itr = table.find( acnt.value );
      if( itr == table.end() ) {
         table.emplace( acnt, [&]( auto& row ) {
            row.owner = acnt;
            row.hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
         });
      } else {
         table.modify( itr, same_payer, [&]( auto& row ) {
            row.hash = eosio::sha256(const_cast<char*>(abi.data()), abi.size());
         });
      }
   }

   void system_contract::init( unsigned_int version, const symbol& core ) {
      require_auth( get_self() );
      check( version.value == 0, "unsupported version for init action" );
      check( _gstate.core_symbol == symbol(), "system contract has already been initialized" );
      _gstate.core_symbol = core;

      auto system_token_supply   = eosio::token::get_supply(token_account, core.code() );
      check( system_token_supply.symbol == core, "specified core symbol does not exist (precision mismatch)" );
      check( system_token_supply.amount > 0, "system token supply must be greater than 0" );

      token::open_action open_act{ token_account, { {get_self(), active_permission} } };
      open_act.send( rex_account, core, get_self() );
   }

   bool system_contract::vote_is_reasserted( eosio::time_point last_reassertion_time ) const {
         return (current_time_point() - last_reassertion_time) < _gremstate.reassertion_period;
   }
} /// rem.system