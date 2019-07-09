#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain/eosio_contract.hpp>
#include <eosio/chain/generated_transaction_object.hpp>

#include <fstream>

#include <contracts.hpp>

eosio::chain::asset core_from_string(const std::string& s) {
  return eosio::chain::asset::from_string(s + " " CORE_SYMBOL_NAME);
}

namespace eosio { namespace testing {
   std::string read_wast( const char* fn ) {
      std::ifstream wast_file(fn);
      FC_ASSERT( wast_file.is_open(), "wast file cannot be found" );
      wast_file.seekg(0, std::ios::end);
      std::vector<char> wast;
      int len = wast_file.tellg();
      FC_ASSERT( len >= 0, "wast file length is -1" );
      wast.resize(len+1);
      wast_file.seekg(0, std::ios::beg);
      wast_file.read(wast.data(), wast.size());
      wast[wast.size()-1] = '\0';
      wast_file.close();
      return {wast.data()};
   }

   std::vector<uint8_t> read_wasm( const char* fn ) {
      std::ifstream wasm_file(fn, std::ios::binary);
      FC_ASSERT( wasm_file.is_open(), "wasm file cannot be found" );
      wasm_file.seekg(0, std::ios::end);
      std::vector<uint8_t> wasm;
      int len = wasm_file.tellg();
      FC_ASSERT( len >= 0, "wasm file length is -1" );
      wasm.resize(len);
      wasm_file.seekg(0, std::ios::beg);
      wasm_file.read((char*)wasm.data(), wasm.size());
      wasm_file.close();
      return wasm;
   }

   std::vector<char> read_abi( const char* fn ) {
      std::ifstream abi_file(fn);
      FC_ASSERT( abi_file.is_open(), "abi file cannot be found" );
      abi_file.seekg(0, std::ios::end);
      std::vector<char> abi;
      int len = abi_file.tellg();
      FC_ASSERT( len >= 0, "abi file length is -1" );
      abi.resize(len+1);
      abi_file.seekg(0, std::ios::beg);
      abi_file.read(abi.data(), abi.size());
      abi[abi.size()-1] = '\0';
      abi_file.close();
      return abi;
   }

   const fc::microseconds base_tester::abi_serializer_max_time{1000*1000}; // 1s for slow test machines

   bool expect_assert_message(const fc::exception& ex, string expected) {
      BOOST_TEST_MESSAGE("LOG : " << "expected: " << expected << ", actual: " << ex.get_log().at(0).get_message());
      return (ex.get_log().at(0).get_message().find(expected) != std::string::npos);
   }

   fc::variant_object filter_fields(const fc::variant_object& filter, const fc::variant_object& value) {
      fc::mutable_variant_object res;
      for( auto& entry : filter ) {
         auto it = value.find(entry.key());
         res( it->key(), it->value() );
      }
      return res;
   }

   void copy_row(const chain::key_value_object& obj, vector<char>& data) {
      data.resize( obj.value.size() );
      memcpy( data.data(), obj.value.data(), obj.value.size() );
   }

   bool base_tester::is_same_chain( base_tester& other ) {
     return control->head_block_id() == other.control->head_block_id();
   }

   void base_tester::init(bool push_genesis, db_read_mode read_mode) {
      cfg.blocks_dir      = tempdir.path() / config::default_blocks_dir_name;
      cfg.state_dir  = tempdir.path() / config::default_state_dir_name;
      cfg.state_size = 1024*1024*8;
      cfg.state_guard_size = 0;
      cfg.reversible_cache_size = 1024*1024*8;
      cfg.reversible_guard_size = 0;
      cfg.contracts_console = true;
      cfg.read_mode = read_mode;

      cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
      cfg.genesis.initial_key = get_public_key( config::system_account_name, "active" );

      for(int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
         if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wavm"))
            cfg.wasm_runtime = chain::wasm_interface::vm_type::wavm;
         else if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wabt"))
            cfg.wasm_runtime = chain::wasm_interface::vm_type::wabt;
      }

      open(nullptr);

      if (push_genesis)
         push_genesis_block();
   }


   void base_tester::init(controller::config config, const snapshot_reader_ptr& snapshot) {
      cfg = config;
      open(snapshot);
   }


   void base_tester::close() {
      control.reset();
      chain_transactions.clear();
   }


   void base_tester::open( const snapshot_reader_ptr& snapshot) {
      control.reset( new controller(cfg) );
      control->add_indices();
      control->startup( []() { return false; }, snapshot);
      chain_transactions.clear();
      control->accepted_block.connect([this]( const block_state_ptr& block_state ){
        FC_ASSERT( block_state->block );
          for( const auto& receipt : block_state->block->transactions ) {
              if( receipt.trx.contains<packed_transaction>() ) {
                  auto &pt = receipt.trx.get<packed_transaction>();
                  chain_transactions[pt.get_transaction().id()] = receipt;
              } else {
                  auto& id = receipt.trx.get<transaction_id_type>();
                  chain_transactions[id] = receipt;
              }
          }
      });
   }

   signed_block_ptr base_tester::push_block(signed_block_ptr b) {
      auto bs = control->create_block_state_future(b);
      control->abort_block();
      control->push_block(bs);

      auto itr = last_produced_block.find(b->producer);
      if (itr == last_produced_block.end() || block_header::num_from_id(b->id()) > block_header::num_from_id(itr->second)) {
         last_produced_block[b->producer] = b->id();
      }

      return b;
   }

   signed_block_ptr base_tester::_produce_block( fc::microseconds skip_time, bool skip_pending_trxs, uint32_t skip_flag) {
      auto head = control->head_block_state();
      auto head_time = control->head_block_time();
      auto next_time = head_time + skip_time;

      if( !control->pending_block_state() || control->pending_block_state()->header.timestamp != next_time ) {
         _start_block( next_time );
      }

      if( !skip_pending_trxs ) {
         unapplied_transactions_type unapplied_trxs = control->get_unapplied_transactions(); // make copy of map
         for (const auto& entry : unapplied_trxs ) {
            auto trace = control->push_transaction(entry.second, fc::time_point::maximum(), DEFAULT_BILLED_CPU_TIME_US );
            if(trace->except) {
               trace->except->dynamic_rethrow_exception();
            }
         }

         vector<transaction_id_type> scheduled_trxs;
         while( (scheduled_trxs = get_scheduled_transactions() ).size() > 0 ) {
            for (const auto& trx : scheduled_trxs ) {
               auto trace = control->push_scheduled_transaction(trx, fc::time_point::maximum(), DEFAULT_BILLED_CPU_TIME_US);
               if(trace->except) {
                  trace->except->dynamic_rethrow_exception();
               }
            }
         }
      }

      auto head_block = _finish_block();

      _start_block( next_time + fc::microseconds(config::block_interval_us));
      return head_block;
   }

   void base_tester::_start_block(fc::time_point block_time) {
      auto head_block_number = control->head_block_num();
      auto producer = control->head_block_state()->get_scheduled_producer(block_time);

      auto last_produced_block_num = control->last_irreversible_block_num();
      auto itr = last_produced_block.find(producer.producer_name);
      if (itr != last_produced_block.end()) {
         last_produced_block_num = std::max(control->last_irreversible_block_num(), block_header::num_from_id(itr->second));
      }

      control->abort_block();
      control->start_block( block_time, head_block_number - last_produced_block_num );
   }

   signed_block_ptr base_tester::_finish_block() {
      FC_ASSERT( control->pending_block_state(), "must first start a block before it can be finished" );

      auto producer = control->head_block_state()->get_scheduled_producer( control->pending_block_time() );
      private_key_type priv_key;
      // Check if signing private key exist in the list
      auto private_key_itr = block_signing_private_keys.find( producer.block_signing_key );
      if( private_key_itr == block_signing_private_keys.end() ) {
         // If it's not found, default to active k1 key
         priv_key = get_private_key( producer.producer_name, "active" );
      } else {
         priv_key = private_key_itr->second;
      }

      control->finalize_block();
      control->sign_block( [&]( digest_type d ) {
                    return priv_key.sign(d);
                    });

      control->commit_block();
      last_produced_block[control->head_block_state()->header.producer] = control->head_block_state()->id;

      return control->head_block_state()->block;
   }

   void base_tester::produce_blocks( uint32_t n, bool empty ) {
      if( empty ) {
         for( uint32_t i = 0; i < n; ++i )
            produce_empty_block();
      } else {
         for( uint32_t i = 0; i < n; ++i )
            produce_block();
      }
   }

   vector<transaction_id_type> base_tester::get_scheduled_transactions() const {
      const auto& idx = control->db().get_index<generated_transaction_multi_index,by_delay>();

      vector<transaction_id_type> result;

      auto itr = idx.begin();
      while( itr != idx.end() && itr->delay_until <= control->pending_block_time() ) {
         result.emplace_back(itr->trx_id);
         ++itr;
      }
      return result;
   }

   void base_tester::produce_blocks_until_end_of_round() {
      uint64_t blocks_per_round;
      while(true) {
         blocks_per_round = control->active_producers().producers.size() * config::producer_repetitions;
         produce_block();
         if (control->head_block_num() % blocks_per_round == (blocks_per_round - 1)) break;
      }
   }

   void base_tester::produce_blocks_for_n_rounds(const uint32_t num_of_rounds) {
      for(uint32_t i = 0; i < num_of_rounds; i++) {
         produce_blocks_until_end_of_round();
      }
   }

   void base_tester::produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(const fc::microseconds target_elapsed_time) {
      fc::microseconds elapsed_time;
      while (elapsed_time < target_elapsed_time) {
         for(uint32_t i = 0; i < control->head_block_state()->active_schedule.producers.size(); i++) {
            const auto time_to_skip = fc::milliseconds(config::producer_repetitions * config::block_interval_ms);
            produce_block(time_to_skip);
            elapsed_time += time_to_skip;
         }
         // if it is more than 24 hours, producer will be marked as inactive
         const auto time_to_skip = fc::seconds(23 * 60 * 60);
         produce_block(time_to_skip);
         elapsed_time += time_to_skip;
      }

   }


  void base_tester::set_transaction_headers( transaction& trx, uint32_t expiration, uint32_t delay_sec ) const {
     trx.expiration = control->head_block_time() + fc::seconds(expiration);
     trx.set_reference_block( control->head_block_id() );

     trx.max_net_usage_words = 0; // No limit
     trx.max_cpu_usage_ms = 0; // No limit
     trx.delay_sec = delay_sec;
  }


   transaction_trace_ptr base_tester::create_account( account_name a, account_name creator, bool multisig, bool include_code ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      if( multisig ) {
         // multisig between account's owner key and creators active permission
         owner_auth = authority(2, {key_weight{get_public_key( a, "owner" ), 1}}, {permission_level_weight{{creator, config::active_name}, 1}});
      } else {
         owner_auth =  authority( get_public_key( a, "owner" ) );
      }

      authority active_auth( get_public_key( a, "active" ) );

      auto sort_permissions = []( authority& auth ) {
         std::sort( auth.accounts.begin(), auth.accounts.end(),
                    []( const permission_level_weight& lhs, const permission_level_weight& rhs ) {
                        return lhs.permission < rhs.permission;
                    }
                  );
      };

      if( include_code ) {
         FC_ASSERT( owner_auth.threshold <= std::numeric_limits<weight_type>::max(), "threshold is too high" );
         FC_ASSERT( active_auth.threshold <= std::numeric_limits<weight_type>::max(), "threshold is too high" );
         owner_auth.accounts.push_back( permission_level_weight{ {a, config::eosio_code_name},
                                                                 static_cast<weight_type>(owner_auth.threshold) } );
         sort_permissions(owner_auth);
         active_auth.accounts.push_back( permission_level_weight{ {a, config::eosio_code_name},
                                                                  static_cast<weight_type>(active_auth.threshold) } );
         sort_permissions(active_auth);
      }

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = active_auth,
                                });

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr base_tester::push_transaction( packed_transaction& trx,
                                                        fc::time_point deadline,
                                                        uint32_t billed_cpu_time_us
                                                      )
   { try {
      if( !control->pending_block_state() )
         _start_block(control->head_block_time() + fc::microseconds(config::block_interval_us));

      auto mtrx = std::make_shared<transaction_metadata>( std::make_shared<packed_transaction>(trx) );
      auto time_limit = deadline == fc::time_point::maximum() ?
            fc::microseconds::maximum() :
            fc::microseconds( deadline - fc::time_point::now() );
      transaction_metadata::start_recover_keys( mtrx, control->get_thread_pool(), control->get_chain_id(), time_limit );
      auto r = control->push_transaction( mtrx, deadline, billed_cpu_time_us );
      if( r->except_ptr ) std::rethrow_exception( r->except_ptr );
      if( r->except ) throw *r->except;
      return r;
   } FC_RETHROW_EXCEPTIONS( warn, "transaction_header: ${header}", ("header", transaction_header(trx.get_transaction()) )) }

   transaction_trace_ptr base_tester::push_transaction( signed_transaction& trx,
                                                        fc::time_point deadline,
                                                        uint32_t billed_cpu_time_us
                                                      )
   { try {
      if( !control->pending_block_state() )
         _start_block(control->head_block_time() + fc::microseconds(config::block_interval_us));
      auto c = packed_transaction::none;

      if( fc::raw::pack_size(trx) > 1000 ) {
         c = packed_transaction::zlib;
      }

      auto time_limit = deadline == fc::time_point::maximum() ?
            fc::microseconds::maximum() :
            fc::microseconds( deadline - fc::time_point::now() );
      auto mtrx = std::make_shared<transaction_metadata>(trx, c);
      transaction_metadata::start_recover_keys( mtrx, control->get_thread_pool(), control->get_chain_id(), time_limit );
      auto r = control->push_transaction( mtrx, deadline, billed_cpu_time_us );
      if( r->except_ptr ) std::rethrow_exception( r->except_ptr );
      if( r->except)  throw *r->except;
      return r;
   } FC_RETHROW_EXCEPTIONS( warn, "transaction_header: ${header}, billed_cpu_time_us: ${billed}",
                            ("header", transaction_header(trx) ) ("billed", billed_cpu_time_us))
   }

   typename base_tester::action_result base_tester::push_action(action&& act, uint64_t authorizer) {
      signed_transaction trx;
      if (authorizer) {
         act.authorization = vector<permission_level>{{authorizer, config::active_name}};
      }
      trx.actions.emplace_back(std::move(act));
      set_transaction_headers(trx);
      if (authorizer) {
         trx.sign(get_private_key(authorizer, "active"), control->get_chain_id());
      }
      try {
         push_transaction(trx);
      } catch (const fc::exception& ex) {
         edump((ex.to_detail_string()));
         return error(ex.top_message()); // top_message() is assumed by many tests; otherwise they fail
         //return error(ex.to_detail_string());
      }
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      return success();
   }

   transaction_trace_ptr base_tester::push_action( const account_name& code,
                                                   const action_name& acttype,
                                                   const account_name& actor,
                                                   const variant_object& data,
                                                   uint32_t expiration,
                                                   uint32_t delay_sec
                                                 )

   {
      vector<permission_level> auths;
      auths.push_back( permission_level{actor, config::active_name} );
      return push_action( code, acttype, auths, data, expiration, delay_sec );
   }

   transaction_trace_ptr base_tester::push_action( const account_name& code,
                                                   const action_name& acttype,
                                                   const vector<account_name>& actors,
                                                   const variant_object& data,
                                                   uint32_t expiration,
                                                   uint32_t delay_sec
                                                 )

   {
      vector<permission_level> auths;
      for (const auto& actor : actors) {
         auths.push_back( permission_level{actor, config::active_name} );
      }
      return push_action( code, acttype, auths, data, expiration, delay_sec );
   }

   transaction_trace_ptr base_tester::push_action( const account_name& code,
                                                   const action_name& acttype,
                                                   const vector<permission_level>& auths,
                                                   const variant_object& data,
                                                   uint32_t expiration,
                                                   uint32_t delay_sec
                                                 )

   { try {
      signed_transaction trx;
      trx.actions.emplace_back( get_action( code, acttype, auths, data ) );
      set_transaction_headers( trx, expiration, delay_sec );
      for (const auto& auth : auths) {
         trx.sign( get_private_key( auth.actor, auth.permission.to_string() ), control->get_chain_id() );
      }

      return push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (code)(acttype)(auths)(data)(expiration)(delay_sec) ) }

   action base_tester::get_action( account_name code, action_name acttype, vector<permission_level> auths,
                                   const variant_object& data )const { try {
      const auto& acnt = control->get_account(code);
      auto abi = acnt.get_abi();
      chain::abi_serializer abis(abi, abi_serializer_max_time);

      string action_type_name = abis.get_action_type(acttype);
      FC_ASSERT( action_type_name != string(), "unknown action type ${a}", ("a",acttype) );


      action act;
      act.account = code;
      act.name = acttype;
      act.authorization = auths;
      act.data = abis.variant_to_binary(action_type_name, data, abi_serializer_max_time);
      return act;
   } FC_CAPTURE_AND_RETHROW() }

   transaction_trace_ptr base_tester::push_reqauth( account_name from, const vector<permission_level>& auths, const vector<private_key_type>& keys ) {
      variant pretty_trx = fc::mutable_variant_object()
         ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name(config::system_account_name))
               ("name", "reqauth")
               ("authorization", auths)
               ("data", fc::mutable_variant_object()
                  ("from", from)
               )
            })
        );

      signed_transaction trx;
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
      set_transaction_headers(trx);
      for(auto iter = keys.begin(); iter != keys.end(); iter++)
         trx.sign( *iter, control->get_chain_id() );
      return push_transaction( trx );
   }


    transaction_trace_ptr base_tester::push_reqauth(account_name from, string role, bool multi_sig) {
        if (!multi_sig) {
            return push_reqauth(from, vector<permission_level>{{from, config::owner_name}},
                                        {get_private_key(from, role)});
        } else {
            return push_reqauth(from, vector<permission_level>{{from, config::owner_name}},
                                        {get_private_key(from, role), get_private_key( config::system_account_name, "active" )} );
        }
    }


   transaction_trace_ptr base_tester::push_dummy(account_name from, const string& v, uint32_t billed_cpu_time_us) {
      // use reqauth for a normal action, this could be anything
      variant pretty_trx = fc::mutable_variant_object()
         ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name(config::system_account_name))
               ("name", "reqauth")
               ("authorization", fc::variants({
                  fc::mutable_variant_object()
                     ("actor", from)
                     ("permission", name(config::active_name))
               }))
               ("data", fc::mutable_variant_object()
                  ("from", from)
               )
            })
        )
        // lets also push a context free action, the multi chain test will then also include a context free action
        ("context_free_actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name(config::null_account_name))
               ("name", "nonce")
               ("data", fc::raw::pack(v))
            })
         );

      signed_transaction trx;
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
      set_transaction_headers(trx);

      trx.sign( get_private_key( from, "active" ), control->get_chain_id() );
      return push_transaction( trx, fc::time_point::maximum(), billed_cpu_time_us );
   }


   transaction_trace_ptr base_tester::transfer( account_name from, account_name to, string amount, string memo, account_name currency ) {
      return transfer( from, to, asset::from_string(amount), memo, currency );
   }


   transaction_trace_ptr base_tester::transfer( account_name from, account_name to, asset amount, string memo, account_name currency ) {
      variant pretty_trx = fc::mutable_variant_object()
         ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", currency)
               ("name", "transfer")
               ("authorization", fc::variants({
                  fc::mutable_variant_object()
                     ("actor", from)
                     ("permission", name(config::active_name))
               }))
               ("data", fc::mutable_variant_object()
                  ("from", from)
                  ("to", to)
                  ("quantity", amount)
                  ("memo", memo)
               )
            })
         );

      signed_transaction trx;
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
      set_transaction_headers(trx);

      trx.sign( get_private_key( from, name(config::active_name).to_string() ), control->get_chain_id()  );
      return push_transaction( trx );
   }


   transaction_trace_ptr base_tester::issue( account_name to, string amount, account_name currency, string memo ) {
      variant pretty_trx = fc::mutable_variant_object()
         ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", currency)
               ("name", "issue")
               ("authorization", fc::variants({
                  fc::mutable_variant_object()
                     ("actor", currency )
                     ("permission", name(config::active_name))
               }))
               ("data", fc::mutable_variant_object()
                  ("to", to)
                  ("quantity", amount)
                  ("memo", memo)
               )
            })
         );

      signed_transaction trx;
      abi_serializer::from_variant(pretty_trx, trx, get_resolver(), abi_serializer_max_time);
      set_transaction_headers(trx);

      trx.sign( get_private_key( currency, name(config::active_name).to_string() ), control->get_chain_id()  );
      return push_transaction( trx );
   }


   void base_tester::link_authority( account_name account, account_name code, permission_name req, action_name type ) {
      signed_transaction trx;

      trx.actions.emplace_back( vector<permission_level>{{account, config::active_name}},
                                linkauth(account, code, type, req));
      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), control->get_chain_id()  );

      push_transaction( trx );
   }


   void base_tester::unlink_authority( account_name account, account_name code, action_name type ) {
      signed_transaction trx;

      trx.actions.emplace_back( vector<permission_level>{{account, config::active_name}},
                                unlinkauth(account, code, type ));
      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), control->get_chain_id()  );

      push_transaction( trx );
   }


   void base_tester::set_authority( account_name account,
                               permission_name perm,
                               authority auth,
                               permission_name parent,
                               const vector<permission_level>& auths,
                               const vector<private_key_type>& keys) { try {
      signed_transaction trx;

      trx.actions.emplace_back( auths,
                                updateauth{
                                   .account    = account,
                                   .permission = perm,
                                   .parent     = parent,
                                   .auth       = move(auth),
                                });

         set_transaction_headers(trx);
      for (const auto& key: keys) {
         trx.sign( key, control->get_chain_id()  );
      }

      push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account)(perm)(auth)(parent) ) }


   void base_tester::set_authority( account_name account,
                                    permission_name perm,
                                    authority auth,
                                    permission_name parent) {
      set_authority(account, perm, auth, parent, { { account, config::owner_name } }, { get_private_key( account, "owner" ) });
   }



   void base_tester::delete_authority( account_name account,
                                    permission_name perm,
                                    const vector<permission_level>& auths,
                                    const vector<private_key_type>& keys ) { try {
         signed_transaction trx;
         trx.actions.emplace_back( auths,
                                   deleteauth(account, perm) );

         set_transaction_headers(trx);
         for (const auto& key: keys) {
            trx.sign( key, control->get_chain_id()  );
         }

         push_transaction( trx );
      } FC_CAPTURE_AND_RETHROW( (account)(perm) ) }


   void base_tester::delete_authority( account_name account,
                                       permission_name perm ) {
      delete_authority(account, perm, { permission_level{ account, config::owner_name } }, { get_private_key( account, "owner" ) });
   }


   void base_tester::set_code( account_name account, const char* wast, const private_key_type* signer  ) try {
      set_code(account, wast_to_wasm(wast), signer);
   } FC_CAPTURE_AND_RETHROW( (account) )


   void base_tester::set_code( account_name account, const vector<uint8_t> wasm, const private_key_type* signer ) try {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                setcode{
                                   .account    = account,
                                   .vmtype     = 0,
                                   .vmversion  = 0,
                                   .code       = bytes(wasm.begin(), wasm.end())
                                });

      set_transaction_headers(trx);
      if( signer ) {
         trx.sign( *signer, control->get_chain_id()  );
      } else {
         trx.sign( get_private_key( account, "active" ), control->get_chain_id()  );
      }
      push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account) )


   void base_tester::set_abi( account_name account, const char* abi_json, const private_key_type* signer ) {
      auto abi = fc::json::from_string(abi_json).template as<abi_def>();
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                setabi{
                                   .account    = account,
                                   .abi        = fc::raw::pack(abi)
                                });

      set_transaction_headers(trx);
      if( signer ) {
         trx.sign( *signer, control->get_chain_id()  );
      } else {
         trx.sign( get_private_key( account, "active" ), control->get_chain_id()  );
      }
      push_transaction( trx );
   }


   bool base_tester::chain_has_transaction( const transaction_id_type& txid ) const {
      return chain_transactions.count(txid) != 0;
   }


   const transaction_receipt& base_tester::get_transaction_receipt( const transaction_id_type& txid ) const {
      return chain_transactions.at(txid);
   }

   /**
    *  Reads balance as stored by generic_currency contract
    */

   asset base_tester::get_currency_balance( const account_name& code,
                                       const symbol&       asset_symbol,
                                       const account_name& account ) const {
      const auto& db  = control->db();
      const auto* tbl = db.template find<table_id_object, by_code_scope_table>(boost::make_tuple(code, account, N(accounts)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto *obj = db.template find<key_value_object, by_scope_primary>(boost::make_tuple(tbl->id, asset_symbol.to_symbol_code().value));
         if (obj) {
            //balance is the first field in the serialization
            fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset(result, asset_symbol);
   }


   vector<char> base_tester::get_row_by_account( uint64_t code, uint64_t scope, uint64_t table, const account_name& act ) const {
      vector<char> data;
      const auto& db = control->db();
      const auto* t_id = db.find<chain::table_id_object, chain::by_code_scope_table>( boost::make_tuple( code, scope, table ) );
      if ( !t_id ) {
         return data;
      }
      //FC_ASSERT( t_id != 0, "object not found" );

      const auto& idx = db.get_index<chain::key_value_index, chain::by_scope_primary>();

      auto itr = idx.lower_bound( boost::make_tuple( t_id->id, act ) );
      if ( itr == idx.end() || itr->t_id != t_id->id || act.value != itr->primary_key ) {
         return data;
      }

      data.resize( itr->value.size() );
      memcpy( data.data(), itr->value.data(), data.size() );
      return data;
   }


   vector<uint8_t> base_tester::to_uint8_vector(const string& s) {
      vector<uint8_t> v(s.size());
      copy(s.begin(), s.end(), v.begin());
      return v;
   };


   vector<uint8_t> base_tester::to_uint8_vector(uint64_t x) {
      vector<uint8_t> v(sizeof(x));
      *reinterpret_cast<uint64_t*>(v.data()) = x;
      return v;
   };


   uint64_t base_tester::to_uint64(fc::variant x) {
      vector<uint8_t> blob;
      fc::from_variant<uint8_t>(x, blob);
      FC_ASSERT(8 == blob.size());
      return *reinterpret_cast<uint64_t*>(blob.data());
   }


   string base_tester::to_string(fc::variant x) {
      vector<uint8_t> v;
      fc::from_variant<uint8_t>(x, v);
      string s(v.size(), 0);
      copy(v.begin(), v.end(), s.begin());
      return s;
   }


   void base_tester::sync_with(base_tester& other) {
      // Already in sync?
      if (control->head_block_id() == other.control->head_block_id())
         return;
      // If other has a longer chain than we do, sync it to us first
      if (control->head_block_num() < other.control->head_block_num())
         return other.sync_with(*this);

      auto sync_dbs = [](base_tester& a, base_tester& b) {
         for( uint32_t i = 1; i <= a.control->head_block_num(); ++i ) {

            auto block = a.control->fetch_block_by_number(i);
            if( block ) { //&& !b.control->is_known_block(block->id()) ) {
               auto bs = b.control->create_block_state_future( block );
               b.control->abort_block();
               b.control->push_block(bs); //, eosio::chain::validation_steps::created_block);
            }
         }
      };

      sync_dbs(*this, other);
      sync_dbs(other, *this);
   }

   void base_tester::push_genesis_block() {
      set_code(config::system_account_name, contracts::eosio_bios_wasm());
      set_abi(config::system_account_name, contracts::eosio_bios_abi().data());
   }

   vector<producer_key> base_tester::get_producer_keys( const vector<account_name>& producer_names )const {
       // Create producer schedule
       vector<producer_key> schedule;
       for (auto& producer_name: producer_names) {
           producer_key pk = { producer_name, get_public_key( producer_name, "active" )};
           schedule.emplace_back(pk);
       }
       return schedule;
   }

   transaction_trace_ptr base_tester::set_producers(const vector<account_name>& producer_names) {
      auto schedule = get_producer_keys( producer_names );

      return push_action( config::system_account_name, N(setprods), config::system_account_name,
                          fc::mutable_variant_object()("schedule", schedule));
   }

   const table_id_object* base_tester::find_table( name code, name scope, name table ) {
      auto tid = control->db().find<table_id_object, by_code_scope_table>(boost::make_tuple(code, scope, table));
      return tid;
   }

   bool fc_exception_message_is::operator()( const fc::exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = (message == expected);
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

   bool fc_exception_message_starts_with::operator()( const fc::exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = boost::algorithm::starts_with( message, expected );
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

   bool fc_assert_exception_message_is::operator()( const fc::assert_exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = false;
      auto pos = message.find( ": " );
      if( pos != std::string::npos ) {
         message = message.substr( pos + 2 );
         match = (message == expected);
      }
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

   bool fc_assert_exception_message_starts_with::operator()( const fc::assert_exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = false;
      auto pos = message.find( ": " );
      if( pos != std::string::npos ) {
         message = message.substr( pos + 2 );
         match = boost::algorithm::starts_with( message, expected );
      }
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

   bool eosio_assert_message_is::operator()( const eosio_assert_message_exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = (message == expected);
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

   bool eosio_assert_message_starts_with::operator()( const eosio_assert_message_exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = boost::algorithm::starts_with( message, expected );
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

   bool eosio_assert_code_is::operator()( const eosio_assert_code_exception& ex ) {
      auto message = ex.get_log().at( 0 ).get_message();
      bool match = (message == expected);
      if( !match ) {
         BOOST_TEST_MESSAGE( "LOG: expected: " << expected << ", actual: " << message );
      }
      return match;
   }

} }  /// eosio::testing

std::ostream& operator<<( std::ostream& osm, const fc::variant& v ) {
   //fc::json::to_stream( osm, v );
   osm << fc::json::to_pretty_string( v );
   return osm;
}

std::ostream& operator<<( std::ostream& osm, const fc::variant_object& v ) {
   osm << fc::variant(v);
   return osm;
}

std::ostream& operator<<( std::ostream& osm, const fc::variant_object::entry& e ) {
   osm << "{ " << e.key() << ": " << e.value() << " }";
   return osm;
}
