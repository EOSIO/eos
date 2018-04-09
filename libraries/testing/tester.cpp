#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/contracts/eos_contract.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <eosio.bios/eosio.bios.wast.hpp>
#include <eosio.bios/eosio.bios.abi.hpp>

#include <fc/utility.hpp>
#include <fc/io/json.hpp>

#include "WAST/WAST.h"
#include "WASM/WASM.h"
#include "IR/Module.h"
#include "IR/Validate.h"

using namespace eosio::chain::contracts;

namespace eosio { namespace testing {

   fc::variant_object filter_fields(const fc::variant_object& filter, const fc::variant_object& value) {
      fc::mutable_variant_object res;
      for( auto& entry : filter ) {
         auto it = value.find(entry.key());
         res( it->key(), it->value() );
      }
      return res;
   }

   void base_tester::init(bool push_genesis, chain_controller::runtime_limits limits) {
      cfg.block_log_dir      = tempdir.path() / "blocklog";
      cfg.shared_memory_dir  = tempdir.path() / "shared";
      cfg.shared_memory_size = 1024*1024*8;

      cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
      cfg.genesis.initial_key = get_public_key( config::system_account_name, "active" );
      cfg.limits = limits;

      for(int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
         if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--binaryen"))
            cfg.wasm_runtime = chain::wasm_interface::vm_type::binaryen;
         else if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wavm"))
            cfg.wasm_runtime = chain::wasm_interface::vm_type::wavm;
      }

      open();

      if (push_genesis)
         push_genesis_block();
   }


   void base_tester::init(chain_controller::controller_config config) {
      cfg = config;
      open();
   }


   public_key_type  base_tester::get_public_key( name keyname, string role ) const {
      return get_private_key( keyname, role ).get_public_key();
   }


   private_key_type base_tester::get_private_key( name keyname, string role ) const {
      return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(string(keyname)+role));
   }


   void base_tester::close() {
      control.reset();
      chain_transactions.clear();
   }


   void base_tester::open() {
      control.reset( new chain_controller(cfg) );
      chain_transactions.clear();
      control->applied_block.connect([this]( const block_trace& trace ){
         for( const auto& region : trace.block.regions) {
            for( const auto& cycle : region.cycles_summary ) {
               for ( const auto& shard : cycle ) {
                  for( const auto& receipt: shard.transactions ) {
                     chain_transactions.emplace(receipt.id, receipt);
                  }
               }
            }
         }
      });
   }

   signed_block base_tester::push_block(signed_block b) {
      control->push_block(b, 2);
      return b;
   }

   signed_block base_tester::_produce_block( fc::microseconds skip_time, uint32_t skip_flag) {
      auto head_time = control->head_block_time();
      auto next_time = head_time + skip_time;
      uint32_t slot  = control->get_slot_at_time( next_time );
      auto sch_pro   = control->get_scheduled_producer(slot);
      auto priv_key  = get_private_key( sch_pro, "active" );

      return control->generate_block( next_time, sch_pro, priv_key, skip_flag );
   }

   void base_tester::produce_blocks( uint32_t n ) {
      for( uint32_t i = 0; i < n; ++i )
         produce_block();
   }


   void base_tester::produce_blocks_until_end_of_round() {
      uint64_t blocks_per_round;
      while(true) {
         blocks_per_round = control->get_global_properties().active_producers.producers.size() * config::producer_repetitions;
         produce_block();
         if (control->head_block_num() % blocks_per_round == (blocks_per_round - 1) ) break;
      }
   }


  void base_tester::set_transaction_headers( signed_transaction& trx, uint32_t expiration, uint32_t delay_sec ) const {
     trx.expiration = control->head_block_time() + fc::seconds(expiration);
     trx.set_reference_block( control->head_block_id() );

     trx.max_net_usage_words = 0; // No limit
     trx.max_kcpu_usage = 0; // No limit
     trx.delay_sec = delay_sec;
  }


   transaction_trace base_tester::create_account( account_name a, account_name creator, bool multisig ) {
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
                                contracts::newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) ),
                                   .recovery = authority( get_public_key( a, "recovery" ) ),
                                });

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  );
      return push_transaction( trx );
   }

   transaction_trace base_tester::push_transaction( packed_transaction& trx, uint32_t skip_flag ) { try {
      return control->push_transaction( trx, skip_flag );
   } FC_CAPTURE_AND_RETHROW( (transaction_header(trx.get_transaction())) ) }

   transaction_trace base_tester::push_transaction( signed_transaction& trx, uint32_t skip_flag ) { try {
      auto ptrx = packed_transaction(trx);
      return push_transaction( ptrx, skip_flag );
   } FC_CAPTURE_AND_RETHROW( (transaction_header(trx)) ) }


   typename base_tester::action_result base_tester::push_action(action&& act, uint64_t authorizer) {
      signed_transaction trx;
      if (authorizer) {
         act.authorization = vector<permission_level>{{authorizer, config::active_name}};
      }
      trx.actions.emplace_back(std::move(act));
      set_transaction_headers(trx);
      if (authorizer) {
         trx.sign(get_private_key(authorizer, "active"), chain_id_type());
      }
      try {
         push_transaction(trx);
      } catch (const fc::exception& ex) {
         return error(ex.top_message());
      }
      produce_block();
      BOOST_REQUIRE_EQUAL(true, chain_has_transaction(trx.id()));
      return success();
   }


   transaction_trace base_tester::push_action( const account_name& code,
                             const action_name& acttype,
                             const account_name& actor,
                             const variant_object& data,
                             uint32_t expiration,
                             uint32_t delay_sec)

   { try {
      return push_action(code, acttype, vector<account_name>{ actor }, data, expiration, delay_sec);
   } FC_CAPTURE_AND_RETHROW( (code)(acttype)(actor)(data)(expiration) ) }


   transaction_trace base_tester::push_action( const account_name& code,
                             const action_name& acttype,
                             const vector<account_name>& actors,
                             const variant_object& data,
                             uint32_t expiration,
                             uint32_t delay_sec)

   { try {
      const auto& acnt = control->get_database().get<account_object,by_name>(code);
      auto abi = acnt.get_abi();
      chain::contracts::abi_serializer abis(abi);
      auto a = control->get_database().get<account_object,by_name>(code).get_abi();

      string action_type_name = abis.get_action_type(acttype);
      FC_ASSERT( action_type_name != string(), "unknown action type ${a}", ("a",acttype) );


      action act;
      act.account = code;
      act.name = acttype;
      for (const auto& actor : actors) {
         act.authorization.push_back(permission_level{actor, config::active_name});
      }
      act.data = abis.variant_to_binary(action_type_name, data);

      signed_transaction trx;
      trx.actions.emplace_back(std::move(act));
      set_transaction_headers(trx, expiration, delay_sec);
      for (const auto& actor : actors) {
         trx.sign(get_private_key(actor, "active"), chain_id_type());
      }

      return push_transaction(trx);
   } FC_CAPTURE_AND_RETHROW( (code)(acttype)(actors)(data)(expiration) ) }


   transaction_trace base_tester::push_reqauth( account_name from, const vector<permission_level>& auths, const vector<private_key_type>& keys ) {
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
      contracts::abi_serializer::from_variant(pretty_trx, trx, get_resolver());
      set_transaction_headers(trx);
      wdump((trx));
      for(auto iter = keys.begin(); iter != keys.end(); iter++)
         trx.sign( *iter, chain_id_type() );
      return push_transaction( trx );
   }


    transaction_trace base_tester::push_reqauth(account_name from, string role, bool multi_sig) {
        if (!multi_sig) {
            return push_reqauth(from, vector<permission_level>{{from, config::owner_name}},
                                        {get_private_key(from, role)});
        } else {
            return push_reqauth(from, vector<permission_level>{{from, config::owner_name}},
                                        {get_private_key(from, role), get_private_key( config::system_account_name, "active" )} );
        }
    }


   transaction_trace base_tester::push_dummy(account_name from, const string& v) {
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
               ("account", name(config::system_account_name))
               ("name", "nonce")
               ("data", fc::mutable_variant_object()
                  ("value", v)
               )
            })
         );

      signed_transaction trx;
      contracts::abi_serializer::from_variant(pretty_trx, trx, get_resolver());
      set_transaction_headers(trx);

      trx.sign( get_private_key( from, "active" ), chain_id_type() );
      return push_transaction( trx );
   }


   transaction_trace base_tester::transfer( account_name from, account_name to, string amount, string memo, account_name currency ) {
      return transfer( from, to, asset::from_string(amount), memo, currency );
   }


   transaction_trace base_tester::transfer( account_name from, account_name to, asset amount, string memo, account_name currency ) {
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
      contracts::abi_serializer::from_variant(pretty_trx, trx, get_resolver());
      set_transaction_headers(trx);

      trx.sign( get_private_key( from, name(config::active_name).to_string() ), chain_id_type()  );
      return push_transaction( trx );
   }


   transaction_trace base_tester::issue( account_name to, string amount, account_name currency ) {
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
               )
            })
         );

      signed_transaction trx;
      contracts::abi_serializer::from_variant(pretty_trx, trx, get_resolver());
      set_transaction_headers(trx);

      trx.sign( get_private_key( currency, name(config::active_name).to_string() ), chain_id_type()  );
      return push_transaction( trx );
   }


   void base_tester::link_authority( account_name account, account_name code, permission_name req, action_name type ) {
      signed_transaction trx;

      trx.actions.emplace_back( vector<permission_level>{{account, config::active_name}},
                                contracts::linkauth(account, code, type, req));
      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );

      push_transaction( trx );
   }


   void base_tester::unlink_authority( account_name account, account_name code, action_name type ) {
      signed_transaction trx;

      trx.actions.emplace_back( vector<permission_level>{{account, config::active_name}},
                                contracts::unlinkauth(account, code, type ));
      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );

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
                                contracts::updateauth{
                                   .account    = account,
                                   .permission = perm,
                                   .parent     = parent,
                                   .data       = move(auth),
                                });

         set_transaction_headers(trx);
      for (const auto& key: keys) {
         trx.sign( key, chain_id_type()  );
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
                                   contracts::deleteauth(account, perm) );

         set_transaction_headers(trx);
         for (const auto& key: keys) {
            trx.sign( key, chain_id_type()  );
         }

         push_transaction( trx );
      } FC_CAPTURE_AND_RETHROW( (account)(perm) ) }


   void base_tester::delete_authority( account_name account,
                                       permission_name perm ) {
      delete_authority(account, perm, { permission_level{ account, config::owner_name } }, { get_private_key( account, "owner" ) });
   }


   void base_tester::set_code( account_name account, const char* wast ) try {
      set_code(account, wast_to_wasm(wast));
   } FC_CAPTURE_AND_RETHROW( (account) )


   void base_tester::set_code( account_name account, const vector<uint8_t> wasm ) try {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                contracts::setcode{
                                   .account    = account,
                                   .vmtype     = 0,
                                   .vmversion  = 0,
                                   .code       = bytes(wasm.begin(), wasm.end())
                                });

      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account) )


   void base_tester::set_abi( account_name account, const char* abi_json) {
      auto abi = fc::json::from_string(abi_json).template as<contracts::abi_def>();
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                contracts::setabi{
                                   .account    = account,
                                   .abi        = abi
                                });

      set_transaction_headers(trx);
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
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
      const auto& db  = control->get_database();
      const auto* tbl = db.template find<contracts::table_id_object, contracts::by_code_scope_table>(boost::make_tuple(code, account, N(accounts)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto *obj = db.template find<contracts::key_value_object, contracts::by_scope_primary>(boost::make_tuple(tbl->id, asset_symbol.value()));
         if (obj) {
            //balance is the second field after symbol, so skip the symbol
            fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset(result, asset_symbol);
   }


   vector<char> base_tester::get_row_by_account( uint64_t code, uint64_t scope, uint64_t table, const account_name& act ) {
      vector<char> data;
      const auto& db = control->get_database();
      const auto* t_id = db.find<chain::contracts::table_id_object, chain::contracts::by_code_scope_table>( boost::make_tuple( code, scope, table ) );
      if ( !t_id ) {
         return data;
      }
      //FC_ASSERT( t_id != 0, "object not found" );

      const auto& idx = db.get_index<chain::contracts::key_value_index, chain::contracts::by_scope_primary>();

      auto itr = idx.lower_bound( boost::make_tuple( t_id->id, act ) );
      if ( itr == idx.end() || itr->t_id != t_id->id || act.value != itr->primary_key ) {
         return data;
      }

      chain_apis::read_only::copy_inline_row( *itr, data );
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
         for (int i = 1; i <= a.control->head_block_num(); ++i) {
            auto block = a.control->fetch_block_by_number(i);
            if (block && !b.control->is_known_block(block->id())) {
               b.control->push_block(*block, eosio::chain::validation_steps::created_block);
            }
         }
      };

      sync_dbs(*this, other);
      sync_dbs(other, *this);
   }

   void base_tester::push_genesis_block() {
      set_code(config::system_account_name, eosio_bios_wast);
      set_abi(config::system_account_name, eosio_bios_abi);
      //produce_block();
   }

   producer_schedule_type base_tester::set_producers(const vector<account_name>& producer_names, const uint32_t version) {
      // Create producer schedule
      producer_schedule_type schedule;
      schedule.version = version;
      for (auto& producer_name: producer_names) {
         producer_key pk = { producer_name, get_public_key( producer_name, "active" )};
         schedule.producers.emplace_back(pk);
      }

      push_action(N(eosio), N(setprods), N(eosio),
                  fc::mutable_variant_object()("version", schedule.version)("producers", schedule.producers));

      return schedule;
   }

   const contracts::table_id_object* base_tester::find_table( name code, name scope, name table ) {
      auto tid = control->get_database().find<table_id_object, by_code_scope_table>(boost::make_tuple(code, scope, table));
      return tid;
   }

} }  /// eosio::test

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
