#include <eosio.system/eosio.system.hpp>
#include <eosiolib/dispatcher.hpp>
#include <eosiolib/crypto.h>

#include "producer_pay.cpp"
#include "delegate_bandwidth.cpp"
#include "voting.cpp"
#include "exchange_state.cpp"
#include "upgrade.cpp"
#include <map>

namespace eosiosystem {

   system_contract::system_contract( name s, name code, datastream<const char*> ds )
   :native(s,code,ds),
    _voters(_self, _self.value),
    _producers(_self, _self.value),
    _producers2(_self, _self.value),
    _global(_self, _self.value),
    _global2(_self, _self.value),
    _global3(_self, _self.value),
    _guarantee(_self, _self.value),
    _rammarket(_self, _self.value),
    _upgrade(_self, _self.value)
   {

      //print( "construct system\n" );
      _gstate  = _global.exists() ? _global.get() : get_default_parameters();
      _gstate2 = _global2.exists() ? _global2.get() : eosio_global_state2{};
      _gstate3 = _global3.exists() ? _global3.get() : eosio_global_state3{};

      _ustate = _upgrade.exists() ? _upgrade.get() : upgrade_state{};
   }

   eosio_global_state system_contract::get_default_parameters() {
      eosio_global_state dp;
      get_blockchain_parameters(dp);
      return dp;
   }

   time_point system_contract::current_time_point() {
      const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
      return ct;
   }

   block_timestamp system_contract::current_block_time() {
      const static block_timestamp cbt{ current_time_point() };
      return cbt;
   }

   symbol system_contract::core_symbol()const {
      const static auto sym = get_core_symbol( _rammarket );
      return sym;
   }

   system_contract::~system_contract() {
      _global.set( _gstate, _self );
      _global2.set( _gstate2, _self );
      _global3.set( _gstate3, _self );

      _upgrade.set( _ustate, _self );
   }

   void system_contract::setram( uint64_t max_ram_size ) {
      require_auth( _self );

      eosio_assert( _gstate.max_ram_size < max_ram_size, "ram may only be increased" ); /// decreasing ram might result market maker issues
      eosio_assert( max_ram_size < 1024ll*1024*1024*1024*1024, "ram size is unrealistic" );
      eosio_assert( max_ram_size > _gstate.total_ram_bytes_reserved, "attempt to set max below reserved" );

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

   /**
    *  Sets the rate of increase of RAM in bytes per block. It is capped by the uint16_t to
    *  a maximum rate of 3 TB per year.
    *
    *  If update_ram_supply hasn't been called for the most recent block, then new ram will
    *  be allocated at the old rate up to the present block before switching the rate.
    */
   void system_contract::setramrate( uint16_t bytes_per_block ) {
      require_auth( _self );

      update_ram_supply();
      _gstate2.new_ram_per_block = bytes_per_block;
   }

   void system_contract::setparams( const eosio::blockchain_parameters& params ) {
      require_auth( _self );
      (eosio::blockchain_parameters&)(_gstate) = params;
      eosio_assert( 3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3" );
      set_blockchain_parameters( params );
   }

   // *bos begin*
   void system_contract::namelist(std::string list, std::string action, const std::vector<name> &names)
   {
      const int MAX_LIST_LENGTH = 30;
      const int MAX_ACTION_LENGTH = 10;
      enum  class list_type:int64_t
      {
         actor_blacklist_type = 1,
         contract_blacklist_type,
         resource_greylist_type,
         list_type_count
      };
      enum class list_action_type:int64_t
      {
         insert_type = 1,
         remove_type,
         list_action_type_count
      };

      std::map<std::string, list_type> list_type_string_to_enum = {
              {"actor_blacklist", list_type::actor_blacklist_type},
              {"contract_blacklist", list_type::contract_blacklist_type},
              {"resource_greylist", list_type::resource_greylist_type}};

      std::map<std::string, list_action_type> list_action_type_string_to_enum = {
          {"insert", list_action_type::insert_type},
          {"remove", list_action_type::remove_type}};

      std::map<std::string, list_type>::iterator itlt = list_type_string_to_enum.find(list);
      std::map<std::string, list_action_type>::iterator itlat = list_action_type_string_to_enum.find(action);

      require_auth(_self);
      eosio_assert(3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3");
      eosio_assert(list.length() < MAX_LIST_LENGTH, "list string is greater than max length 30");
      eosio_assert(action.length() < MAX_ACTION_LENGTH, " action string is greater than max length 10");
      eosio_assert(itlt != list_type_string_to_enum.end(), " unknown list type string  support 'actor_blacklist' ,'contract_blacklist', 'resource_greylist'");
      eosio_assert(itlat != list_action_type_string_to_enum.end(), " unknown list type string support 'insert' or 'remove'");

      auto packed_names = pack(names);

      set_name_list_packed(static_cast<int64_t>(itlt->second), static_cast<int64_t>(itlat->second), packed_names.data(), packed_names.size());
   }

   void system_contract::setguaminres(uint32_t ram, uint32_t cpu, uint32_t net)
   {
      require_auth(_self);

      const static uint32_t MAX_BYTE = 100*1024;
      const static uint32_t MAX_MICROSEC = 100*1000;

      const static uint32_t STEP_BYTE = 10*1024;
      const static uint32_t STEP_MICROSEC = 10*1000;
      eosio_assert(3 <= _gstate.max_authority_depth, "max_authority_depth should be at least 3");
      eosio_assert(ram <= MAX_BYTE  && net <= MAX_BYTE, "the value of ram, cpu and net should not more then 100 kb");
      eosio_assert(cpu <= MAX_MICROSEC , "the value of  cpu  should not more then 100 ms");

      eosio_guaranteed_min_res _gmr = _guarantee.exists() ? _guarantee.get() : eosio_guaranteed_min_res{};
      eosio_assert(ram >= _gmr.ram, "can not reduce ram guarantee ");
      eosio_assert(ram <= _gmr.ram + STEP_BYTE, "minimum ram guarantee can not increace more then 10kb every time");
      eosio_assert(cpu <= _gmr.cpu + STEP_MICROSEC, "minimum cpu guarantee can not increace more then 10ms token weight every time");
      eosio_assert(net <= _gmr.net + STEP_BYTE, "minimum net guarantee can not increace more then 10kb token weight every time");

      _gmr.ram = ram;
      _gmr.cpu = cpu;
      _gmr.net = net;

      _guarantee.set(_gmr, _self);
      set_guaranteed_minimum_resources(ram, cpu, net);
   }
   // *bos end*

   void system_contract::setpriv( name account, uint8_t ispriv ) {
      require_auth( _self );
      set_privileged( account.value, ispriv );
   }

   void system_contract::setalimits( name account, int64_t ram, int64_t net, int64_t cpu ) {
      require_auth( _self );

      user_resources_table userres( _self, account.value );
      auto ritr = userres.find( account.value );
      eosio_assert( ritr == userres.end(), "only supports unlimited accounts" );

      auto vitr = _voters.find( account.value );
      if( vitr != _voters.end() ) {
         bool ram_managed = has_field( vitr->flags1, voter_info::flags1_fields::ram_managed );
         bool net_managed = has_field( vitr->flags1, voter_info::flags1_fields::net_managed );
         bool cpu_managed = has_field( vitr->flags1, voter_info::flags1_fields::cpu_managed );
         eosio_assert( !(ram_managed || net_managed || cpu_managed), "cannot use setalimits on an account with managed resources" );
      }

      set_resource_limits( account.value, ram, net, cpu );
   }

   void system_contract::setacctram( name account, std::optional<int64_t> ram_bytes ) {
      require_auth( _self );

      int64_t current_ram, current_net, current_cpu;
      get_resource_limits( account.value, &current_ram, &current_net, &current_cpu );

      int64_t ram = 0;

      if( !ram_bytes ) {
         auto vitr = _voters.find( account.value );
         eosio_assert( vitr != _voters.end() && has_field( vitr->flags1, voter_info::flags1_fields::ram_managed ),
                       "RAM of account is already unmanaged" );

         user_resources_table userres( _self, account.value );
         auto ritr = userres.find( account.value );

         ram = ram_gift_bytes;
         if( ritr != userres.end() ) {
            ram += ritr->ram_bytes;
         }

         _voters.modify( vitr, same_payer, [&]( auto& v ) {
            v.flags1 = set_field( v.flags1, voter_info::flags1_fields::ram_managed, false );
         });
      } else {
         eosio_assert( *ram_bytes >= 0, "not allowed to set RAM limit to unlimited" );

         auto vitr = _voters.find( account.value );
         if ( vitr != _voters.end() ) {
            _voters.modify( vitr, same_payer, [&]( auto& v ) {
               v.flags1 = set_field( v.flags1, voter_info::flags1_fields::ram_managed, true );
            });
         } else {
            _voters.emplace( account, [&]( auto& v ) {
               v.owner  = account;
               v.flags1 = set_field( v.flags1, voter_info::flags1_fields::ram_managed, true );
            });
         }

         ram = *ram_bytes;
      }

      set_resource_limits( account.value, ram, current_net, current_cpu );
   }

   void system_contract::setacctnet( name account, std::optional<int64_t> net_weight ) {
      require_auth( _self );

      int64_t current_ram, current_net, current_cpu;
      get_resource_limits( account.value, &current_ram, &current_net, &current_cpu );

      int64_t net = 0;

      if( !net_weight ) {
         auto vitr = _voters.find( account.value );
         eosio_assert( vitr != _voters.end() && has_field( vitr->flags1, voter_info::flags1_fields::net_managed ),
                       "Network bandwidth of account is already unmanaged" );

         user_resources_table userres( _self, account.value );
         auto ritr = userres.find( account.value );

         if( ritr != userres.end() ) {
            net = ritr->net_weight.amount;
         }

         _voters.modify( vitr, same_payer, [&]( auto& v ) {
            v.flags1 = set_field( v.flags1, voter_info::flags1_fields::net_managed, false );
         });
      } else {
         eosio_assert( *net_weight >= -1, "invalid value for net_weight" );

         auto vitr = _voters.find( account.value );
         if ( vitr != _voters.end() ) {
            _voters.modify( vitr, same_payer, [&]( auto& v ) {
               v.flags1 = set_field( v.flags1, voter_info::flags1_fields::net_managed, true );
            });
         } else {
            _voters.emplace( account, [&]( auto& v ) {
               v.owner  = account;
               v.flags1 = set_field( v.flags1, voter_info::flags1_fields::net_managed, true );
            });
         }

         net = *net_weight;
      }

      set_resource_limits( account.value, current_ram, net, current_cpu );
   }

   void system_contract::setacctcpu( name account, std::optional<int64_t> cpu_weight ) {
      require_auth( _self );

      int64_t current_ram, current_net, current_cpu;
      get_resource_limits( account.value, &current_ram, &current_net, &current_cpu );

      int64_t cpu = 0;

      if( !cpu_weight ) {
         auto vitr = _voters.find( account.value );
         eosio_assert( vitr != _voters.end() && has_field( vitr->flags1, voter_info::flags1_fields::cpu_managed ),
                       "CPU bandwidth of account is already unmanaged" );

         user_resources_table userres( _self, account.value );
         auto ritr = userres.find( account.value );

         if( ritr != userres.end() ) {
            cpu = ritr->cpu_weight.amount;
         }

         _voters.modify( vitr, same_payer, [&]( auto& v ) {
            v.flags1 = set_field( v.flags1, voter_info::flags1_fields::cpu_managed, false );
         });
      } else {
         eosio_assert( *cpu_weight >= -1, "invalid value for cpu_weight" );

         auto vitr = _voters.find( account.value );
         if ( vitr != _voters.end() ) {
            _voters.modify( vitr, same_payer, [&]( auto& v ) {
               v.flags1 = set_field( v.flags1, voter_info::flags1_fields::cpu_managed, true );
            });
         } else {
            _voters.emplace( account, [&]( auto& v ) {
               v.owner  = account;
               v.flags1 = set_field( v.flags1, voter_info::flags1_fields::cpu_managed, true );
            });
         }

         cpu = *cpu_weight;
      }

      set_resource_limits( account.value, current_ram, current_net, cpu );
   }

   void system_contract::rmvproducer( name producer ) {
      require_auth( _self );
      auto prod = _producers.find( producer.value );
      eosio_assert( prod != _producers.end(), "producer not found" );
      _producers.modify( prod, same_payer, [&](auto& p) {
            p.deactivate();
         });
   }

   void system_contract::updtrevision( uint8_t revision ) {
      require_auth( _self );
      eosio_assert( _gstate2.revision < 255, "can not increment revision" ); // prevent wrap around
      eosio_assert( revision == _gstate2.revision + 1, "can only increment revision by one" );
      eosio_assert( revision <= 1, // set upper bound to greatest revision supported in the code
                    "specified revision is not yet supported by the code" );
      _gstate2.revision = revision;
   }

   void system_contract::bidname( name bidder, name newname, asset bid ) {
      require_auth( bidder );
      eosio_assert( newname.suffix() == newname, "you can only bid on top-level suffix" );

      eosio_assert( (bool)newname, "the empty name is not a valid account name to bid on" );
      eosio_assert( (newname.value & 0xFull) == 0, "13 character names are not valid account names to bid on" );
      eosio_assert( (newname.value & 0x1F0ull) == 0, "accounts with 12 character names and no dots can be created without bidding required" );
      eosio_assert( !is_account( newname ), "account already exists" );
      eosio_assert( bid.symbol == core_symbol(), "asset must be system token" );
      eosio_assert( bid.amount > 0, "insufficient bid" );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {bidder, active_permission} },
         { bidder, names_account, bid, std::string("bid name ")+ newname.to_string() }
      );

      name_bid_table bids(_self, _self.value);

      if (newname.length() < BASE_LENGTH)
      {
         
         auto idx = bids.get_index<"highbid"_n>();
         auto highest = idx.lower_bound(std::numeric_limits<uint64_t>::max() / 2);
        
         if (highest != idx.end() &&
             highest->high_bid > 0)
         {
           std::string msg= "newname which length is less than 3  must increase bid by 10% than the highest bid in all bid :current value:"+std::to_string(highest->high_bid );
           eosio_assert(bid.amount - highest->high_bid > (highest->high_bid / 10),msg.c_str());
         }
      }

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
         eosio_assert( current->high_bid > 0, "this auction has already closed" );
         eosio_assert( bid.amount - current->high_bid > (current->high_bid / 10), "must increase bid by 10%" );
         eosio_assert( current->high_bidder != bidder, "account is already highest bidder" );

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

   void system_contract::bidrefund( name bidder, name newname ) {
      bid_refund_table refunds_table(_self, newname.value);
      auto it = refunds_table.find( bidder.value );
      eosio_assert( it != refunds_table.end(), "refund not found" );
      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {names_account, active_permission}, {bidder, active_permission} },
         { names_account, bidder, asset(it->amount), std::string("refund bid on name ")+(name{newname}).to_string() }
      );
      refunds_table.erase( it );
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
   void native::newaccount( name              creator,
                            name              newact,
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
               if("bos"_n==suffix)
               {
                     require_auth(_self);
               }
               name_bid_table bids(_self, _self.value);
               auto current = bids.find( newact.value );
               eosio_assert( current != bids.end(), "no active bid for name" );
               eosio_assert( current->high_bidder == creator, "only highest bidder can claim" );
               eosio_assert( current->high_bid < 0, "auction for name is not closed yet" );
               bids.erase( current );
            } else {
               eosio_assert( creator == suffix, "only suffix may create this account" );
            }

            const static auto BOS_PREFIX = "bos.";
            std::string::size_type p = newact.to_string().find(BOS_PREFIX);
            if(p != std::string::npos && 0 == p)
            {
            require_auth(_self);
            }
           
         }
      }

      user_resources_table  userres( _self, newact.value);

      userres.emplace( newact, [&]( auto& res ) {
        res.owner = newact;
        res.net_weight = asset( 0, system_contract::get_core_symbol() );
        res.cpu_weight = asset( 0, system_contract::get_core_symbol() );
      });

      set_resource_limits( newact.value, 0, 0, 0 );
   }

   void native::setabi( name acnt, const std::vector<char>& abi ) {
      eosio::multi_index< "abihash"_n, abi_hash >  table(_self, _self.value);
      auto itr = table.find( acnt.value );
      if( itr == table.end() ) {
         table.emplace( acnt, [&]( auto& row ) {
            row.owner= acnt;
            sha256( const_cast<char*>(abi.data()), abi.size(), &row.hash );
         });
      } else {
         table.modify( itr, same_payer, [&]( auto& row ) {
            sha256( const_cast<char*>(abi.data()), abi.size(), &row.hash );
         });
      }
   }

   void system_contract::init( unsigned_int version, symbol core ) {
      require_auth( _self );
      eosio_assert( version.value == 0, "unsupported version for init action" );

      auto itr = _rammarket.find(ramcore_symbol.raw());
      eosio_assert( itr == _rammarket.end(), "system contract has already been initialized" );

      auto system_token_supply   = eosio::token::get_supply(token_account, core.code() );
      eosio_assert( system_token_supply.symbol == core, "specified core symbol does not exist (precision mismatch)" );

      eosio_assert( system_token_supply.amount > 0, "system token supply must be greater than 0" );
      _rammarket.emplace( _self, [&]( auto& m ) {
         m.supply.amount = 100000000000000ll;
         m.supply.symbol = ramcore_symbol;
         m.base.balance.amount = int64_t(_gstate.free_ram());
         m.base.balance.symbol = ram_symbol;
         m.quote.balance.amount = system_token_supply.amount / 1000;
         m.quote.balance.symbol = core;
      });
   }
} /// eosio.system


EOSIO_DISPATCH( eosiosystem::system_contract,
     // native.hpp (newaccount definition is actually in eosio.system.cpp)
     (newaccount)(updateauth)(deleteauth)(linkauth)(unlinkauth)(canceldelay)(onerror)(setabi)
     // eosio.system.cpp
     (init)(setram)(setramrate)(setparams)(namelist)(setguaminres)(setpriv)(setalimits)(setacctram)(setacctnet)(setacctcpu)
     (rmvproducer)(updtrevision)(bidname)(bidrefund)
     // delegate_bandwidth.cpp
     (buyrambytes)(buyram)(sellram)(delegatebw)(undelegatebw)(refund)
     // voting.cpp
     (regproducer)(unregprod)(voteproducer)(regproxy)
     // producer_pay.cpp
     (onblock)(claimrewards)
     //upgrade.cpp
     (setupgrade)
)
