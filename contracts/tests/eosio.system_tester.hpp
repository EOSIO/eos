/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/resource_limits.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"

#include <fc/variant_object.hpp>
#include <fstream>

using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;

using mvo = fc::mutable_variant_object;

#ifndef TESTER
#ifdef NON_VALIDATING_TEST
#define TESTER tester
#else
#define TESTER validating_tester
#endif
#endif


namespace eosio_system {

class eosio_system_tester : public TESTER {
public:

   void basic_setup() {
      produce_blocks( 2 );

      create_accounts({ N(eosio.token), N(eosio.ram), N(eosio.ramfee), N(eosio.stake),
               N(eosio.bpay), N(eosio.vpay), N(eosio.saving), N(eosio.names), N(eosio.rex) });


      produce_blocks( 100 );
      set_code( N(eosio.token), contracts::token_wasm());
      set_abi( N(eosio.token), contracts::token_abi().data() );
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.token) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         token_abi_ser.set_abi(abi, abi_serializer_max_time);
      }
   }

   void create_core_token( symbol core_symbol = symbol{CORE_SYM} ) {
      FC_ASSERT( core_symbol.precision() != 4, "create_core_token assumes precision of core token is 4" );
      create_currency( N(eosio.token), config::system_account_name, asset(100000000000000, core_symbol) );
      issue( asset(10000000000000, core_symbol) );
      BOOST_REQUIRE_EQUAL( asset(10000000000000, core_symbol), get_balance( "eosio", core_symbol ) );
   }

   void deploy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::system_wasm() );
      set_abi( config::system_account_name, contracts::system_abi().data() );
      if( call_init ) {
         base_tester::push_action(config::system_account_name, N(init),
                                               config::system_account_name,  mutable_variant_object()
                                               ("version", 0)
                                               ("core", CORE_SYM_STR)
         );
      }

      {
         const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi, abi_serializer_max_time);
      }
   }

   void remaining_setup() {
      produce_blocks();

      // Assumes previous setup steps were done with core token symbol set to CORE_SYM
      create_account_with_resources( N(alice1111111), config::system_account_name, core_sym::from_string("1.0000"), false );
      create_account_with_resources( N(bob111111111), config::system_account_name, core_sym::from_string("0.4500"), false );
      create_account_with_resources( N(carol1111111), config::system_account_name, core_sym::from_string("1.0000"), false );

      BOOST_REQUIRE_EQUAL( core_sym::from_string("1000000000.0000"), get_balance("eosio")  + get_balance("eosio.ramfee") + get_balance("eosio.stake") + get_balance("eosio.ram") );
   }

   enum class setup_level {
      none,
      minimal,
      core_token,
      deploy_contract,
      full
   };

   eosio_system_tester( setup_level l = setup_level::full ) {
      if( l == setup_level::none ) return;

      basic_setup();
      if( l == setup_level::minimal ) return;

      create_core_token();
      if( l == setup_level::core_token ) return;

      deploy_contract();
      if( l == setup_level::deploy_contract ) return;

      remaining_setup();
   }

   template<typename Lambda>
   eosio_system_tester(Lambda setup) {
      setup(*this);

      basic_setup();
      create_core_token();
      deploy_contract();
      remaining_setup();
   }


   void create_accounts_with_resources( vector<account_name> accounts, account_name creator = config::system_account_name ) {
      for( auto a : accounts ) {
         create_account_with_resources( a, creator );
      }
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, uint32_t ram_bytes = 8000 ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      owner_auth =  authority( get_public_key( a, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( config::system_account_name, N(buyrambytes), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("bytes", ram_bytes) )
                              );
      trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", core_sym::from_string("10.0000") )
                                            ("stake_cpu_quantity", core_sym::from_string("10.0000") )
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = core_sym::from_string("10.0000"), asset cpu = core_sym::from_string("10.0000") ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( config::system_account_name, N(buyram), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", net )
                                            ("stake_cpu_quantity", cpu )
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr setup_producer_accounts( const std::vector<account_name>& accounts,
                                                  asset ram = core_sym::from_string("1.0000"),
                                                  asset cpu = core_sym::from_string("80.0000"),
                                                  asset net = core_sym::from_string("80.0000")
                                                )
   {
      account_name creator(config::system_account_name);
      signed_transaction trx;
      set_transaction_headers(trx);

      for (const auto& a: accounts) {
         authority owner_auth( get_public_key( a, "owner" ) );
         trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                   newaccount{
                                         .creator  = creator,
                                         .name     = a,
                                         .owner    = owner_auth,
                                         .active   = authority( get_public_key( a, "active" ) )
                                         });

         trx.actions.emplace_back( get_action( config::system_account_name, N(buyram), vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("payer", creator)
                                               ("receiver", a)
                                               ("quant", ram) )
                                   );

         trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("from", creator)
                                               ("receiver", a)
                                               ("stake_net_quantity", net)
                                               ("stake_cpu_quantity", cpu )
                                               ("transfer", 0 )
                                               )
                                   );
      }

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   action_result buyram( const account_name& payer, account_name receiver, const asset& eosin ) {
      return push_action( payer, N(buyram), mvo()( "payer",payer)("receiver",receiver)("quant",eosin) );
   }
   action_result buyrambytes( const account_name& payer, account_name receiver, uint32_t numbytes ) {
      return push_action( payer, N(buyrambytes), mvo()( "payer",payer)("receiver",receiver)("bytes",numbytes) );
   }

   action_result sellram( const account_name& account, uint64_t numbytes ) {
      return push_action( account, N(sellram), mvo()( "account", account)("bytes",numbytes) );
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), auth ? uint64_t(signer) : signer == N(bob111111111) ? N(alice1111111) : N(bob111111111) );
   }

   action_result stake( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action( name(from), N(delegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", 0 )
      );
   }

   action_result stake( const account_name& acnt, const asset& net, const asset& cpu ) {
      return stake( acnt, acnt, net, cpu );
   }

   action_result stake_with_transfer( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action( name(from), N(delegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("stake_net_quantity", net)
                          ("stake_cpu_quantity", cpu)
                          ("transfer", true )
      );
   }

   action_result stake_with_transfer( const account_name& acnt, const asset& net, const asset& cpu ) {
      return stake_with_transfer( acnt, acnt, net, cpu );
   }

   action_result unstake( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action( name(from), N(undelegatebw), mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("unstake_net_quantity", net)
                          ("unstake_cpu_quantity", cpu)
      );
   }

   action_result unstake( const account_name& acnt, const asset& net, const asset& cpu ) {
      return unstake( acnt, acnt, net, cpu );
   }

   int64_t bancor_convert( int64_t S, int64_t R, int64_t T ) { return double(R) * T  / ( double(S) + T ); };

   int64_t get_net_limit( account_name a ) {
      int64_t ram_bytes = 0, net = 0, cpu = 0;
      control->get_resource_limits_manager().get_account_limits( a, ram_bytes, net, cpu );
      return net;
   };

   int64_t get_cpu_limit( account_name a ) {
      int64_t ram_bytes = 0, net = 0, cpu = 0;
      control->get_resource_limits_manager().get_account_limits( a, ram_bytes, net, cpu );
      return cpu;
   };

   action_result deposit( const account_name& owner, const asset& amount ) {
      return push_action( name(owner), N(deposit), mvo()
                          ("owner",  owner)
                          ("amount", amount)
      );
   }

   action_result withdraw( const account_name& owner, const asset& amount ) {
      return push_action( name(owner), N(withdraw), mvo()
                          ("owner",  owner)
                          ("amount", amount)
      );
   }

   action_result buyrex( const account_name& from, const asset& amount ) {
      return push_action( name(from), N(buyrex), mvo()
                          ("from",   from)
                          ("amount", amount)
      );
   }

   asset get_buyrex_result( const account_name& from, const asset& amount ) {
      auto trace = base_tester::push_action( config::system_account_name, N(buyrex), from, mvo()("from", from)("amount", amount) );
      asset rex_received;
      for ( size_t i = 0; i < trace->action_traces.size(); ++i ) {
         if ( trace->action_traces[i].act.name == N(buyresult) ) {
            fc::raw::unpack( trace->action_traces[i].act.data.data(),
                             trace->action_traces[i].act.data.size(),
                             rex_received );
            return rex_received;
         }
      }
      return rex_received;
   }

   action_result unstaketorex( const account_name& owner, const account_name& receiver, const asset& from_net, const asset& from_cpu ) {
      return push_action( name(owner), N(unstaketorex), mvo()
                          ("owner",    owner)
                          ("receiver", receiver)
                          ("from_net", from_net)
                          ("from_cpu", from_cpu)
      );
   }

   asset get_unstaketorex_result( const account_name& owner, const account_name& receiver, const asset& from_net, const asset& from_cpu ) {
      auto trace = base_tester::push_action( config::system_account_name, N(unstaketorex), owner, mvo()
                                             ("owner", owner)
                                             ("receiver", receiver)
                                             ("from_net", from_net)
                                             ("from_cpu", from_cpu)
      );
      asset rex_received;
      for ( size_t i = 0; i < trace->action_traces.size(); ++i ) {
         if ( trace->action_traces[i].act.name == N(buyresult) ) {
            fc::raw::unpack( trace->action_traces[i].act.data.data(),
                             trace->action_traces[i].act.data.size(),
                             rex_received );
            return rex_received;
         }
      }
      return rex_received;
   }

   action_result sellrex( const account_name& from, const asset& rex ) {
      return push_action( name(from), N(sellrex), mvo()
                          ("from", from)
                          ("rex",  rex)
      );
   }

   asset get_sellrex_result( const account_name& from, const asset& rex ) {
      auto trace = base_tester::push_action( config::system_account_name, N(sellrex), from, mvo()("from", from)("rex", rex) );
      asset proceeds;
      for ( size_t i = 0; i < trace->action_traces.size(); ++i ) {
         if ( trace->action_traces[i].act.name == N(sellresult) ) {
            fc::raw::unpack( trace->action_traces[i].act.data.data(),
                             trace->action_traces[i].act.data.size(),
                             proceeds );
            return proceeds;
         }
      }
      return proceeds;
   }

   auto get_rexorder_result( const transaction_trace_ptr& trace ) {
      std::vector<std::pair<account_name, asset>> output;
      for ( size_t i = 0; i < trace->action_traces.size(); ++i ) {
         if ( trace->action_traces[i].act.name == N(orderresult) ) {
            fc::datastream<const char*> ds( trace->action_traces[i].act.data.data(),
                                            trace->action_traces[i].act.data.size() );
            account_name owner; fc::raw::unpack( ds, owner );
            asset proceeds; fc::raw::unpack( ds, proceeds );
            output.emplace_back( owner, proceeds );
         }
      }
      return output;
   }

   action_result cancelrexorder( const account_name& owner ) {
      return push_action( name(owner), N(cnclrexorder), mvo()("owner", owner) );
   }

   action_result rentcpu( const account_name& from, const account_name& receiver, const asset& payment, const asset& fund = core_sym::from_string("0.0000") ) {
      return push_action( name(from), N(rentcpu), mvo()
                          ("from",         from)
                          ("receiver",     receiver)
                          ("loan_payment", payment)
                          ("loan_fund",    fund)
      );
   }

   action_result rentnet( const account_name& from, const account_name& receiver, const asset& payment, const asset& fund = core_sym::from_string("0.0000") ) {
      return push_action( name(from), N(rentnet), mvo()
                          ("from",         from)
                          ("receiver",     receiver)
                          ("loan_payment", payment)
                          ("loan_fund",    fund)
      );
   }

   asset _get_rentrex_result( const account_name& from, const account_name& receiver, const asset& payment, bool cpu ) {
      const name act = cpu ? N(rentcpu) : N(rentnet);
      auto trace = base_tester::push_action( config::system_account_name, act, from, mvo()
                                             ("from",         from)
                                             ("receiver",     receiver)
                                             ("loan_payment", payment)
                                             ("loan_fund",    core_sym::from_string("0.0000") )
      );

      asset rented_tokens = core_sym::from_string("0.0000");
      for ( size_t i = 0; i < trace->action_traces.size(); ++i ) {
         if ( trace->action_traces[i].act.name == N(rentresult) ) {
            fc::raw::unpack( trace->action_traces[i].act.data.data(),
                             trace->action_traces[i].act.data.size(),
                             rented_tokens );
            return rented_tokens;
         }
      }
      return rented_tokens;
   }

   asset get_rentcpu_result( const account_name& from, const account_name& receiver, const asset& payment ) {
      return _get_rentrex_result( from, receiver, payment, true );
   }

   asset get_rentnet_result( const account_name& from, const account_name& receiver, const asset& payment ) {
      return _get_rentrex_result( from, receiver, payment, false );
   }

   action_result fundcpuloan( const account_name& from, const uint64_t loan_num, const asset& payment ) {
      return push_action( name(from), N(fundcpuloan), mvo()
                          ("from",       from)
                          ("loan_num",   loan_num)
                          ("payment",    payment)
      );
   }

   action_result fundnetloan( const account_name& from, const uint64_t loan_num, const asset& payment ) {
      return push_action( name(from), N(fundnetloan), mvo()
                          ("from",       from)
                          ("loan_num",   loan_num)
                          ("payment",    payment)
      );
   }


   action_result defundcpuloan( const account_name& from, const uint64_t loan_num, const asset& amount ) {
      return push_action( name(from), N(defcpuloan), mvo()
                          ("from",     from)
                          ("loan_num", loan_num)
                          ("amount",   amount)
      );
   }

   action_result defundnetloan( const account_name& from, const uint64_t loan_num, const asset& amount ) {
      return push_action( name(from), N(defnetloan), mvo()
                          ("from",     from)
                          ("loan_num", loan_num)
                          ("amount",   amount)
      );
   }

   action_result updaterex( const account_name& owner ) {
      return push_action( name(owner), N(updaterex), mvo()("owner", owner) );
   }

   action_result rexexec( const account_name& user, uint16_t max ) {
      return push_action( name(user), N(rexexec), mvo()("user", user)("max", max) );
   }

   action_result consolidate( const account_name& owner ) {
      return push_action( name(owner), N(consolidate), mvo()("owner", owner) );
   }

   action_result mvtosavings( const account_name& owner, const asset& rex ) {
      return push_action( name(owner), N(mvtosavings), mvo()("owner", owner)("rex", rex) );
   }

   action_result mvfrsavings( const account_name& owner, const asset& rex ) {
      return push_action( name(owner), N(mvfrsavings), mvo()("owner", owner)("rex", rex) );
   }

   action_result closerex( const account_name& owner ) {
      return push_action( name(owner), N(closerex), mvo()("owner", owner) );
   }

   fc::variant get_last_loan(bool cpu) {
      vector<char> data;
      const auto& db = control->db();
      namespace chain = eosio::chain;
      auto table = cpu ? N(cpuloan) : N(netloan);
      const auto* t_id = db.find<eosio::chain::table_id_object, chain::by_code_scope_table>( boost::make_tuple( config::system_account_name, config::system_account_name, table ) );
      if ( !t_id ) {
         return fc::variant();
      }

      const auto& idx = db.get_index<chain::key_value_index, chain::by_scope_primary>();

      auto itr = idx.upper_bound( boost::make_tuple( t_id->id, std::numeric_limits<uint64_t>::max() ));
      if ( itr == idx.begin() ) {
         return fc::variant();
      }
      --itr;
      if ( itr->t_id != t_id->id ) {
         return fc::variant();
      }

      data.resize( itr->value.size() );
      memcpy( data.data(), itr->value.data(), data.size() );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "rex_loan", data, abi_serializer_max_time );
   }

   fc::variant get_last_cpu_loan() {
      return get_last_loan( true );
   }

   fc::variant get_last_net_loan() {
      return get_last_loan( false );
   }

   fc::variant get_loan_info( const uint64_t& loan_num, bool cpu ) const {
      name table_name = cpu ? N(cpuloan) : N(netloan);
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, table_name, loan_num );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "rex_loan", data, abi_serializer_max_time );
   }

   fc::variant get_cpu_loan( const uint64_t loan_num ) const {
      return get_loan_info( loan_num, true );
   }

   fc::variant get_net_loan( const uint64_t loan_num ) const {
      return get_loan_info( loan_num, false );
   }

   fc::variant get_dbw_obj( const account_name& from, const account_name& receiver ) const {
      vector<char> data = get_row_by_account( config::system_account_name, from, N(delband), receiver );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("delegated_bandwidth", data, abi_serializer_max_time);
   }

   asset get_rex_balance( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexbal), act );
      return data.empty() ? asset(0, symbol(SY(4, REX))) : abi_ser.binary_to_variant("rex_balance", data, abi_serializer_max_time)["rex_balance"].as<asset>();
   }

   fc::variant get_rex_balance_obj( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexbal), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("rex_balance", data, abi_serializer_max_time);
   }

   asset get_rex_fund( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexfund), act );
      return data.empty() ? asset(0, symbol{CORE_SYM}) : abi_ser.binary_to_variant("rex_fund", data, abi_serializer_max_time)["balance"].as<asset>();
   }

   fc::variant get_rex_fund_obj( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexfund), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "rex_fund", data, abi_serializer_max_time );
   }

   asset get_rex_vote_stake( const account_name& act ) const {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexbal), act );
      return data.empty() ? core_sym::from_string("0.0000") : abi_ser.binary_to_variant("rex_balance", data, abi_serializer_max_time)["vote_stake"].as<asset>();
   }

   fc::variant get_rex_order( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexqueue), act );
      return abi_ser.binary_to_variant( "rex_order", data, abi_serializer_max_time );
   }

   fc::variant get_rex_order_obj( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rexqueue), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "rex_order", data, abi_serializer_max_time );
   }

   fc::variant get_rex_pool() const {
      vector<char> data;
      const auto& db = control->db();
      namespace chain = eosio::chain;
      const auto* t_id = db.find<eosio::chain::table_id_object, chain::by_code_scope_table>( boost::make_tuple( config::system_account_name, config::system_account_name, N(rexpool) ) );
      if ( !t_id ) {
         return fc::variant();
      }

      const auto& idx = db.get_index<chain::key_value_index, chain::by_scope_primary>();

      auto itr = idx.lower_bound( boost::make_tuple( t_id->id, 0 ) );
      if ( itr == idx.end() || itr->t_id != t_id->id || 0 != itr->primary_key ) {
         return fc::variant();
      }

      data.resize( itr->value.size() );
      memcpy( data.data(), itr->value.data(), data.size() );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "rex_pool", data, abi_serializer_max_time );
   }

   void setup_rex_accounts( const std::vector<account_name>& accounts,
                            const asset& init_balance,
                            const asset& net = core_sym::from_string("80.0000"),
                            const asset& cpu = core_sym::from_string("80.0000"),
                            bool deposit_into_rex_fund = true ) {
      const asset nstake = core_sym::from_string("10.0000");
      const asset cstake = core_sym::from_string("10.0000");
      create_account_with_resources( N(proxyaccount), config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
      BOOST_REQUIRE_EQUAL( success(), push_action( N(proxyaccount), N(regproxy), mvo()("proxy", "proxyaccount")("isproxy", true) ) );
      for (const auto& a: accounts) {
         create_account_with_resources( a, config::system_account_name, core_sym::from_string("1.0000"), false, net, cpu );
         transfer( config::system_account_name, a, init_balance + nstake + cstake, config::system_account_name );
         BOOST_REQUIRE_EQUAL( success(),                        stake( a, a, nstake, cstake) );
         BOOST_REQUIRE_EQUAL( success(),                        vote( a, { }, N(proxyaccount) ) );
         BOOST_REQUIRE_EQUAL( init_balance,                     get_balance(a) );
         BOOST_REQUIRE_EQUAL( asset::from_string("0.0000 REX"), get_rex_balance(a) );
         if (deposit_into_rex_fund) {
            BOOST_REQUIRE_EQUAL( success(),    deposit( a, init_balance ) );
            BOOST_REQUIRE_EQUAL( init_balance, get_rex_fund( a ) );
            BOOST_REQUIRE_EQUAL( 0,            get_balance( a ).get_amount() );
         }
      }
   }

   action_result bidname( const account_name& bidder, const account_name& newname, const asset& bid ) {
      return push_action( name(bidder), N(bidname), mvo()
                          ("bidder",  bidder)
                          ("newname", newname)
                          ("bid", bid)
                          );
   }

   static fc::variant_object producer_parameters_example( int n ) {
      return mutable_variant_object()
         ("max_block_net_usage", 10000000 + n )
         ("target_block_net_usage_pct", 10 + n )
         ("max_transaction_net_usage", 1000000 + n )
         ("base_per_transaction_net_usage", 100 + n)
         ("net_usage_leeway", 500 + n )
         ("context_free_discount_net_usage_num", 1 + n )
         ("context_free_discount_net_usage_den", 100 + n )
         ("max_block_cpu_usage", 10000000 + n )
         ("target_block_cpu_usage_pct", 10 + n )
         ("max_transaction_cpu_usage", 1000000 + n )
         ("min_transaction_cpu_usage", 100 + n )
         ("max_transaction_lifetime", 3600 + n)
         ("deferred_trx_expiration_window", 600 + n)
         ("max_transaction_delay", 10*86400+n)
         ("max_inline_action_size", 4096 + n)
         ("max_inline_action_depth", 4 + n)
         ("max_authority_depth", 6 + n)
         ("max_ram_size", (n % 10 + 1) * 1024 * 1024)
         ("ram_reserve_ratio", 100 + n);
   }

   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      action_result r = push_action( acnt, N(regproducer), mvo()
                          ("producer",  acnt )
                          ("producer_key", get_public_key( acnt, "active" ) )
                          ("url", "" )
                          ("location", 0 )
      );
      BOOST_REQUIRE_EQUAL( success(), r);
      return r;
   }

   action_result vote( const account_name& voter, const std::vector<account_name>& producers, const account_name& proxy = name(0) ) {
      return push_action(voter, N(voteproducer), mvo()
                         ("voter",     voter)
                         ("proxy",     proxy)
                         ("producers", producers));
   }

   uint32_t last_block_time() const {
      return time_point_sec( control->head_block_time() ).sec_since_epoch();
   }

   asset get_balance( const account_name& act, symbol balance_symbol = symbol{CORE_SYM} ) {
      vector<char> data = get_row_by_account( N(eosio.token), act, N(accounts), balance_symbol.to_symbol_code().value );
      return data.empty() ? asset(0, balance_symbol) : token_abi_ser.binary_to_variant("account", data, abi_serializer_max_time)["balance"].as<asset>();
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, act, N(userres), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "user_resources", data, abi_serializer_max_time );
   }

   fc::variant get_voter_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(voters), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data, abi_serializer_max_time );
   }

   fc::variant get_producer_info( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producers), act );
      return abi_ser.binary_to_variant( "producer_info", data, abi_serializer_max_time );
   }

   fc::variant get_producer_info2( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(producers2), act );
      return abi_ser.binary_to_variant( "producer_info2", data, abi_serializer_max_time );
   }

   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, N(create), contract, act );
   }

   void issue( const asset& amount, const name& manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(issue), manager, mutable_variant_object()
                                ("to",       manager )
                                ("quantity", amount )
                                ("memo",     "")
                                );
   }

   void transfer( const name& from, const name& to, const asset& amount, const name& manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(transfer), manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", amount)
                                ("memo", "")
                                );
   }

   void issue_and_transfer( const name& to, const asset& amount, const name& manager = config::system_account_name ) {
      signed_transaction trx;
      trx.actions.emplace_back( get_action( N(eosio.token), N(issue),
                                            vector<permission_level>{{manager, config::active_name}},
                                            mutable_variant_object()
                                            ("to",       manager )
                                            ("quantity", amount )
                                            ("memo",     "")
                                            )
                                );
      if ( to != manager ) {
         trx.actions.emplace_back( get_action( N(eosio.token), N(transfer),
                                               vector<permission_level>{{manager, config::active_name}},
                                               mutable_variant_object()
                                               ("from",     manager)
                                               ("to",       to )
                                               ("quantity", amount )
                                               ("memo",     "")
                                               )
                                   );
      }
      set_transaction_headers( trx );
      trx.sign( get_private_key( manager, "active" ), control->get_chain_id()  );
      push_transaction( trx );
   }

   double stake2votes( asset stake ) {
      auto now = control->pending_block_time().time_since_epoch().count() / 1000000;
      return stake.get_amount() * pow(2, int64_t((now - (config::block_timestamp_epoch / 1000)) / (86400 * 7))/ double(52) ); // 52 week periods (i.e. ~years)
   }

   double stake2votes( const string& s ) {
      return stake2votes( core_sym::from_string(s) );
   }

   fc::variant get_stats( const string& symbolname ) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   asset get_token_supply() {
      return get_stats("4," CORE_SYM_NAME)["supply"].as<asset>();
   }

   uint64_t microseconds_since_epoch_of_iso_string( const fc::variant& v ) {
      return static_cast<uint64_t>( time_point::from_iso_string( v.as_string() ).time_since_epoch().count() );
   }

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time );
   }

   fc::variant get_global_state2() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global2), N(global2) );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state2", data, abi_serializer_max_time );
   }

   fc::variant get_global_state3() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global3), N(global3) );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state3", data, abi_serializer_max_time );
   }

   fc::variant get_refund_request( name account ) {
      vector<char> data = get_row_by_account( config::system_account_name, account, N(refunds), account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "refund_request", data, abi_serializer_max_time );
   }

   abi_serializer initialize_multisig() {
      abi_serializer msig_abi_ser;
      {
         create_account_with_resources( N(eosio.msig), config::system_account_name );
         BOOST_REQUIRE_EQUAL( success(), buyram( "eosio", "eosio.msig", core_sym::from_string("5000.0000") ) );
         produce_block();

         auto trace = base_tester::push_action(config::system_account_name, N(setpriv),
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eosio.msig")
                                               ("is_priv", 1)
         );

         set_code( N(eosio.msig), contracts::msig_wasm() );
         set_abi( N(eosio.msig), contracts::msig_abi().data() );

         produce_blocks();
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.msig) );
         abi_def msig_abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, msig_abi), true);
         msig_abi_ser.set_abi(msig_abi, abi_serializer_max_time);
      }
      return msig_abi_ser;
   }

   vector<name> active_and_vote_producers() {
      //stake more than 15% of total EOS supply to activate chain
      transfer( "eosio", "alice1111111", core_sym::from_string("650000000.0000"), "eosio" );
      BOOST_REQUIRE_EQUAL( success(), stake( "alice1111111", "alice1111111", core_sym::from_string("300000000.0000"), core_sym::from_string("300000000.0000") ) );

      // create accounts {defproducera, defproducerb, ..., defproducerz} and register as producers
      std::vector<account_name> producer_names;
      {
         producer_names.reserve('z' - 'a' + 1);
         const std::string root("defproducer");
         for ( char c = 'a'; c < 'a'+21; ++c ) {
            producer_names.emplace_back(root + std::string(1, c));
         }
         setup_producer_accounts(producer_names);
         for (const auto& p: producer_names) {

            BOOST_REQUIRE_EQUAL( success(), regproducer(p) );
         }
      }
      produce_blocks( 250);

      auto trace_auth = TESTER::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()
                                            ("account", name(config::system_account_name).to_string())
                                            ("permission", name(config::active_name).to_string())
                                            ("parent", name(config::owner_name).to_string())
                                            ("auth",  authority(1, {key_weight{get_public_key( config::system_account_name, "active" ), 1}}, {
                                                  permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1},
                                                     permission_level_weight{{config::producers_account_name,  config::active_name}, 1}
                                               }
                                            ))
      );
      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

      //vote for producers
      {
         transfer( config::system_account_name, "alice1111111", core_sym::from_string("100000000.0000"), config::system_account_name );
         BOOST_REQUIRE_EQUAL(success(), stake( "alice1111111", core_sym::from_string("30000000.0000"), core_sym::from_string("30000000.0000") ) );
         BOOST_REQUIRE_EQUAL(success(), buyram( "alice1111111", "alice1111111", core_sym::from_string("30000000.0000") ) );
         BOOST_REQUIRE_EQUAL(success(), push_action(N(alice1111111), N(voteproducer), mvo()
                                                    ("voter",  "alice1111111")
                                                    ("proxy", name(0).to_string())
                                                    ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                             )
         );
      }
      produce_blocks( 250 );

      auto producer_keys = control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( 21, producer_keys.size() );
      BOOST_REQUIRE_EQUAL( name("defproducera"), producer_keys[0].producer_name );

      return producer_names;
   }

   void cross_15_percent_threshold() {
      setup_producer_accounts({N(producer1111)});
      regproducer(N(producer1111));
      {
         signed_transaction trx;
         set_transaction_headers(trx);

         trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw),
                                               vector<permission_level>{{config::system_account_name, config::active_name}},
                                               mvo()
                                               ("from", name{config::system_account_name})
                                               ("receiver", "producer1111")
                                               ("stake_net_quantity", core_sym::from_string("150000000.0000") )
                                               ("stake_cpu_quantity", core_sym::from_string("0.0000") )
                                               ("transfer", 1 )
                                             )
                                 );
         trx.actions.emplace_back( get_action( config::system_account_name, N(voteproducer),
                                               vector<permission_level>{{N(producer1111), config::active_name}},
                                               mvo()
                                               ("voter", "producer1111")
                                               ("proxy", name(0).to_string())
                                               ("producers", vector<account_name>(1, N(producer1111)))
                                             )
                                 );
         trx.actions.emplace_back( get_action( config::system_account_name, N(undelegatebw),
                                               vector<permission_level>{{N(producer1111), config::active_name}},
                                               mvo()
                                               ("from", "producer1111")
                                               ("receiver", "producer1111")
                                               ("unstake_net_quantity", core_sym::from_string("150000000.0000") )
                                               ("unstake_cpu_quantity", core_sym::from_string("0.0000") )
                                             )
                                 );

         set_transaction_headers(trx);
         trx.sign( get_private_key( config::system_account_name, "active" ), control->get_chain_id()  );
         trx.sign( get_private_key( N(producer1111), "active" ), control->get_chain_id()  );
         push_transaction( trx );
         produce_block();
      }
   }

   action_result setinflation( int64_t annual_rate, int64_t inflation_pay_factor, int64_t votepay_factor ) {
      return push_action( N(eosio), N(setinflation), mvo()
               ("annual_rate",     annual_rate)
               ("inflation_pay_factor", inflation_pay_factor)
               ("votepay_factor", votepay_factor)
      );
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
};

inline fc::mutable_variant_object voter( account_name acct ) {
   return mutable_variant_object()
      ("owner", acct)
      ("proxy", name(0).to_string())
      ("producers", variants() )
      ("staked", int64_t(0))
      //("last_vote_weight", double(0))
      ("proxied_vote_weight", double(0))
      ("is_proxy", 0)
      ;
}

inline fc::mutable_variant_object voter( account_name acct, const asset& vote_stake ) {
   return voter( acct )( "staked", vote_stake.get_amount() );
}

inline fc::mutable_variant_object voter( account_name acct, int64_t vote_stake ) {
   return voter( acct )( "staked", vote_stake );
}

inline fc::mutable_variant_object proxy( account_name acct ) {
   return voter( acct )( "is_proxy", 1 );
}

inline uint64_t M( const string& eos_str ) {
   return core_sym::from_string( eos_str ).get_amount();
}

}
