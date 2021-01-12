#pragma once

#include <eosio/chain/abi_serializer.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/variant_object.hpp>

#include <contracts.hpp>

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

template<typename Impl = TESTER>
class eosio_system_tester : public Impl {
public:
   using action_result = eosio::testing::base_tester::action_result;

   eosio_system_tester() {
      this->produce_blocks( 2 );

      this->create_accounts({ "eosio.token"_n, "eosio.ram"_n, "eosio.ramfee"_n, "eosio.stake"_n,
               "eosio.bpay"_n, "eosio.vpay"_n, "eosio.saving"_n, "eosio.names"_n });

      this->produce_blocks( 100 );

      this->set_code( "eosio.token"_n, contracts::eosio_token_wasm() );
      this->set_abi( "eosio.token"_n, contracts::eosio_token_abi().data() );

      {
         const auto& accnt = this->control->db().template get<account_object,by_name>( "eosio.token"_n );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         token_abi_ser.set_abi(abi, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ));
      }

      this->create_currency( "eosio.token"_n, config::system_account_name, core_from_string("10000000000.0000") );
      this->issue(config::system_account_name,      core_from_string("1000000000.0000"));
      BOOST_REQUIRE_EQUAL( core_from_string("1000000000.0000"), get_balance( name("eosio") ) );

      this->set_code( config::system_account_name, contracts::eosio_system_wasm() );
      this->set_abi( config::system_account_name, contracts::eosio_system_abi().data() );

      Impl::push_action(config::system_account_name, "init"_n,
                            config::system_account_name,  mutable_variant_object()
                            ("version", 0)
                            ("core", CORE_SYM_STR));

      {
         const auto& accnt = this->control->db().template get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ));
      }

      this->produce_blocks();

      this->create_account_with_resources( "alice1111111"_n, config::system_account_name, core_from_string("1.0000"), false );
      this->create_account_with_resources( "bob111111111"_n, config::system_account_name, core_from_string("0.4500"), false );
      this->create_account_with_resources( "carol1111111"_n, config::system_account_name, core_from_string("1.0000"), false );

      BOOST_REQUIRE_EQUAL( core_from_string("1000000000.0000"),
            this->get_balance(name("eosio")) + this->get_balance(name("eosio.ramfee")) + this->get_balance(name("eosio.stake")) + this->get_balance(name("eosio.ram")) );
   }

   action_result open( account_name  owner,
                       const string& symbolname,
                       account_name  ram_payer    ) {
      return this->push_action( ram_payer, "open"_n, mvo()
                                ( "owner", owner )
                                ( "symbol", symbolname )
                                ( "ram_payer", ram_payer )
         );
   }

   void create_accounts_with_resources( vector<account_name> accounts, account_name creator = config::system_account_name ) {
      for( auto a : accounts ) {
         Impl::create_account_with_resources( a, creator );
      }
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, uint32_t ram_bytes = 8000 ) {
      signed_transaction trx;
      this->set_transaction_headers(trx);

      authority owner_auth;
      owner_auth =  authority( this->get_public_key( a, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( this->get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( this->get_action( config::system_account_name, "buyrambytes"_n, vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("bytes", ram_bytes) )
                              );
      trx.actions.emplace_back( this->get_action( config::system_account_name, "delegatebw"_n, vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", core_from_string("10.0000") )
                                            ("stake_cpu_quantity", core_from_string("10.0000") )
                                            ("transfer", 0 )
                                          )
                                );

      this->set_transaction_headers(trx);
      trx.sign( this->get_private_key( creator, "active" ), this->control->get_chain_id()  );
      return this->push_transaction( trx );
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, asset ramfunds, bool multisig,
                                                        asset net = core_from_string("10.0000"), asset cpu = core_from_string("10.0000") ) {
      signed_transaction trx;
      this->set_transaction_headers(trx);

      authority owner_auth;
      if (multisig) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{this->get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( this->get_public_key( a, "owner" ) );
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( this->get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( this->get_action( config::system_account_name, "buyram"_n, vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("quant", ramfunds) )
                              );

      trx.actions.emplace_back( this->get_action( config::system_account_name, "delegatebw"_n, vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", net )
                                            ("stake_cpu_quantity", cpu )
                                            ("transfer", 0 )
                                          )
                                );

      this->set_transaction_headers(trx);
      trx.sign( this->get_private_key( creator, "active" ), this->control->get_chain_id()  );
      return this->push_transaction( trx );
   }

   transaction_trace_ptr setup_producer_accounts( const std::vector<account_name>& accounts ) {
      account_name creator(config::system_account_name);
      signed_transaction trx;
      this->set_transaction_headers(trx);
      asset cpu = core_from_string("80.0000");
      asset net = core_from_string("80.0000");
      asset ram = core_from_string("1.0000");

      for (const auto& a: accounts) {
         authority owner_auth( this->get_public_key( a, "owner" ) );
         trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                   newaccount{
                                         .creator  = creator,
                                         .name     = a,
                                         .owner    = owner_auth,
                                         .active   = authority( this->get_public_key( a, "active" ) )
                                         });

         trx.actions.emplace_back( this->get_action( config::system_account_name, "buyram"_n, vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("payer", creator)
                                               ("receiver", a)
                                               ("quant", ram) )
                                   );

         trx.actions.emplace_back( this->get_action( config::system_account_name, "delegatebw"_n, vector<permission_level>{ {creator, config::active_name} },
                                               mvo()
                                               ("from", creator)
                                               ("receiver", a)
                                               ("stake_net_quantity", net)
                                               ("stake_cpu_quantity", cpu )
                                               ("transfer", 0 )
                                               )
                                   );
      }

      this->set_transaction_headers(trx);
      trx.sign( this->get_private_key( creator, "active" ), this->control->get_chain_id()  );
      return this->push_transaction( trx );
   }

   action_result buyram( const account_name& payer, account_name receiver, const asset& eosin ) {
      return push_action( payer, "buyram"_n, mvo()( "payer",payer)("receiver",receiver)("quant",eosin) );
   }
   action_result buyrambytes( const account_name& payer, account_name receiver, uint32_t numbytes ) {
      return push_action( payer, "buyrambytes"_n, mvo()( "payer",payer)("receiver",receiver)("bytes",numbytes) );
   }

   action_result sellram( const account_name& account, uint64_t numbytes ) {
      return push_action( account, "sellram"_n, mvo()( "account", account)("bytes",numbytes) );
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );

         return Impl::push_action( std::move(act), auth ? signer.to_uint64_t() :
                                                         signer == "bob111111111"_n ? "alice1111111"_n.to_uint64_t() : "bob111111111"_n.to_uint64_t() );
   }

   action_result stake( const account_name& from, const account_name& to, const asset& net, const asset& cpu ) {
      return push_action( name(from), "delegatebw"_n, mvo()
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
      return push_action( name(from), "delegatebw"_n, mvo()
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
      return push_action( name(from), "undelegatebw"_n, mvo()
                          ("from",     from)
                          ("receiver", to)
                          ("unstake_net_quantity", net)
                          ("unstake_cpu_quantity", cpu)
      );
   }

   action_result unstake( const account_name& acnt, const asset& net, const asset& cpu ) {
      return unstake( acnt, acnt, net, cpu );
   }

   action_result bidname( const account_name& bidder, const account_name& newname, const asset& bid ) {
      return push_action( name(bidder), "bidname"_n, mvo()
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
         ("max_inline_action_size", 512*1024 + n)
         ("max_inline_action_depth", 4 + n)
         ("max_authority_depth", 6 + n)
         ("max_ram_size", (n % 10 + 1) * 1024 * 1024)
         ("ram_reserve_ratio", 100 + n);
   }

   action_result regproducer( const account_name& acnt, int params_fixture = 1 ) {
      action_result r = push_action( acnt, "regproducer"_n, mvo()
                          ("producer",  acnt )
                          ("producer_key", this->get_public_key( acnt, "active" ) )
                          ("url", "" )
                          ("location", 0 )
      );
      BOOST_REQUIRE_EQUAL( this->success(), r);
      return r;
   }

   action_result vote( const account_name& voter, const std::vector<account_name>& producers, const account_name& proxy = name(0) ) {
      return push_action(voter, "voteproducer"_n, mvo()
                         ("voter",     voter)
                         ("proxy",     proxy)
                         ("producers", producers));
   }

   uint32_t last_block_time() const {
      return time_point_sec( this->control->head_block_time() ).sec_since_epoch();
   }

   asset get_balance( const account_name& act ) {
      vector<char> data = this->get_row_by_account( "eosio.token"_n, act, "accounts"_n, name(symbol(CORE_SYMBOL).to_symbol_code().value) );
      const auto ret = data.empty() ? asset(0, symbol(CORE_SYMBOL)) : token_abi_ser.binary_to_variant("account", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ))["balance"].template as<asset>();
      return ret;
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = this->get_row_by_account( config::system_account_name, act, "userres"_n, act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "user_resources", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );
   }

   fc::variant get_voter_info( const account_name& act ) {
      vector<char> data = this->get_row_by_account( config::system_account_name, config::system_account_name, "voters"_n, act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "voter_info", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );
   }

   fc::variant get_producer_info( const account_name& act ) {
      vector<char> data = this->get_row_by_account( config::system_account_name, config::system_account_name, "producers"_n, act );
      return abi_ser.binary_to_variant( "producer_info", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );
   }

   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, "create"_n, contract, act );
   }

   void issue( name to, const asset& amount, name manager = config::system_account_name ) {
      base_tester::push_action( "eosio.token"_n, "issue"_n, manager, mutable_variant_object()
                                ("to",      to )
                                ("quantity", amount )
                                ("memo", "")
                                );
   }
   void transfer( name from, name to, const asset& amount, name manager = config::system_account_name ) {
      base_tester::push_action( "eosio.token"_n, "transfer"_n, manager, mutable_variant_object()
                                ("from",    from)
                                ("to",      to )
                                ("quantity", amount)
                                ("memo", "")
                                );
   }

   double stake2votes( asset stake ) {
      auto now = this->control->pending_block_time().time_since_epoch().count() / 1000000;
      return stake.get_amount() * pow(2, int64_t((now - (config::block_timestamp_epoch / 1000)) / (86400 * 7))/ double(52) ); // 52 week periods (i.e. ~years)
   }

   double stake2votes( const string& s ) {
      return stake2votes( core_from_string(s) );
   }

   fc::variant get_stats( const string& symbolname ) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = this->get_row_by_account( "eosio.token"_n, name(symbol_code), "stat"_n, name(symbol_code) );
      return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant( "currency_stats", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );
   }

   asset get_token_supply() {
      return get_stats("4," CORE_SYMBOL_NAME)["supply"].template as<asset>();
   }

   fc::variant get_global_state() {
      vector<char> data = this->get_row_by_account( config::system_account_name, config::system_account_name, "global"_n, "global"_n );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );

   }

   fc::variant get_refund_request( name account ) {
      vector<char> data = this->get_row_by_account( config::system_account_name, account, "refunds"_n, account );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "refund_request", data, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ) );
   }

   abi_serializer initialize_multisig() {
      abi_serializer msig_abi_ser;
      {
         this->create_account_with_resources( "eosio.msig"_n, config::system_account_name );
         BOOST_REQUIRE_EQUAL( this->success(), buyram( name("eosio"), name("eosio.msig"), core_from_string("5000.0000") ) );
         this->produce_block();

         auto trace = base_tester::push_action(config::system_account_name, "setpriv"_n,
                                               config::system_account_name,  mutable_variant_object()
                                               ("account", "eosio.msig")
                                               ("is_priv", 1)
         );

         this->set_code( "eosio.msig"_n, contracts::eosio_msig_wasm() );
         this->set_abi( "eosio.msig"_n, contracts::eosio_msig_abi().data() );

         this->produce_blocks();
         const auto& accnt = this->control->db().template get<account_object,by_name>( "eosio.msig"_n );
         abi_def msig_abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, msig_abi), true);
         msig_abi_ser.set_abi(msig_abi, abi_serializer::create_yield_function( eosio::testing::base_tester::abi_serializer_max_time ));
      }
      return msig_abi_ser;
   }

   vector<name> active_and_vote_producers() {
      //stake more than 15% of total EOS supply to activate chain
      transfer( name("eosio"), name("alice1111111"), core_from_string("650000000.0000"), name("eosio") );
      BOOST_REQUIRE_EQUAL( this->success(), stake( name("alice1111111"), name("alice1111111"), core_from_string("300000000.0000"), core_from_string("300000000.0000") ) );

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

            BOOST_REQUIRE_EQUAL( this->success(), regproducer(p) );
         }
      }
      this->produce_blocks( 250);

      auto trace_auth = Impl::push_action(config::system_account_name, updateauth::get_name(), config::system_account_name, mvo()
                                            ("account", name(config::system_account_name).to_string())
                                            ("permission", name(config::active_name).to_string())
                                            ("parent", name(config::owner_name).to_string())
                                            ("auth",  authority(1, {key_weight{this->get_public_key( config::system_account_name, "active" ), 1}}, {
                                                  permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1},
                                                     permission_level_weight{{config::producers_account_name,  config::active_name}, 1}
                                               }
                                            ))
      );
      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);

      //vote for producers
      {
         transfer( config::system_account_name, name("alice1111111"), core_from_string("100000000.0000"), config::system_account_name );
         BOOST_REQUIRE_EQUAL(this->success(), stake( name("alice1111111"), core_from_string("30000000.0000"), core_from_string("30000000.0000") ) );
         BOOST_REQUIRE_EQUAL(this->success(), buyram( name("alice1111111"), name("alice1111111"), core_from_string("30000000.0000") ) );
         BOOST_REQUIRE_EQUAL(this->success(), push_action("alice1111111"_n, "voteproducer"_n, mvo()
                                                    ("voter",  "alice1111111")
                                                    ("proxy", name(0).to_string())
                                                    ("producers", vector<account_name>(producer_names.begin(), producer_names.begin()+21))
                             )
         );
      }
      this->produce_blocks( 250 );

      auto producer_keys = this->control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( 21, producer_keys.size() );
      BOOST_REQUIRE_EQUAL( name("defproducera"), producer_keys[0].producer_name );

      return producer_names;
   }

   void cross_15_percent_threshold() {
      setup_producer_accounts({"producer1111"_n});
      regproducer("producer1111"_n);
      {
         signed_transaction trx;
         this->set_transaction_headers(trx);

         trx.actions.emplace_back( this->get_action( config::system_account_name, "delegatebw"_n,
                                               vector<permission_level>{{config::system_account_name, config::active_name}},
                                               mvo()
                                               ("from", name{config::system_account_name})
                                               ("receiver", "producer1111")
                                               ("stake_net_quantity", core_from_string("150000000.0000") )
                                               ("stake_cpu_quantity", core_from_string("0.0000") )
                                               ("transfer", 1 )
                                             )
                                 );
         trx.actions.emplace_back( this->get_action( config::system_account_name, "voteproducer"_n,
                                               vector<permission_level>{{"producer1111"_n, config::active_name}},
                                               mvo()
                                               ("voter", "producer1111")
                                               ("proxy", name(0).to_string())
                                               ("producers", vector<account_name>(1, "producer1111"_n))
                                             )
                                 );
         trx.actions.emplace_back( this->get_action( config::system_account_name, "undelegatebw"_n,
                                               vector<permission_level>{{"producer1111"_n, config::active_name}},
                                               mvo()
                                               ("from", "producer1111")
                                               ("receiver", "producer1111")
                                               ("unstake_net_quantity", core_from_string("150000000.0000") )
                                               ("unstake_cpu_quantity", core_from_string("0.0000") )
                                             )
                                 );

         this->set_transaction_headers(trx);
         trx.sign( this->get_private_key( config::system_account_name, "active" ), this->control->get_chain_id()  );
         trx.sign( this->get_private_key( "producer1111"_n, "active" ), this->control->get_chain_id()  );
         this->push_transaction( trx );
      }
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
   return core_from_string( eos_str ).get_amount();
}

}
