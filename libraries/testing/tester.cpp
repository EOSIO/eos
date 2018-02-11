#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/contracts/eos_contract.hpp>
#include <eosio/chain/contracts/contract_table_objects.hpp>

#include <test.system/test.system.wast.hpp>
#include <test.system/test.system.abi.hpp>

#include <fc/utility.hpp>
#include <fc/io/json.hpp>

#include "WAST/WAST.h"
#include "WASM/WASM.h"
#include "IR/Module.h"
#include "IR/Validate.h"

namespace eosio { namespace testing {

   tester::tester(chain_controller::runtime_limits limits, bool process_genesis) {
      cfg.block_log_dir      = tempdir.path() / "blocklog";
      cfg.shared_memory_dir  = tempdir.path() / "shared";
      cfg.shared_memory_size = 1024*1024*8;

      cfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
      cfg.genesis.initial_key = get_public_key( config::system_account_name, "active" );
      cfg.limits = limits;

      open();
      if (process_genesis)
         push_genesis_block();
   }

   void tester::push_genesis_block() {
      set_code(config::system_account_name, test_system_wast);
      set_abi(config::system_account_name, test_system_abi);
   }

   public_key_type  tester::get_public_key( name keyname, string role ) const {
      return get_private_key( keyname, role ).get_public_key(); 
   }

   private_key_type tester::get_private_key( name keyname, string role ) const {
      return private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(string(keyname)+role));
   }

   void tester::close() {
      control.reset();
      chain_transactions.clear();
   }
   void tester::open() {
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

   signed_block tester::produce_block( fc::microseconds skip_time ) {
      auto head_time = control->head_block_time();
      auto next_time = head_time + skip_time;
      uint32_t slot  = control->get_slot_at_time( next_time );
      auto sch_pro   = control->get_scheduled_producer(slot);
      auto priv_key  = get_private_key( sch_pro, "active" );

      return control->generate_block( next_time, sch_pro, priv_key, skip_missed_block_penalty );
   }


   void tester::produce_blocks( uint32_t n ) {
      for( uint32_t i = 0; i < n; ++i )
         produce_block();
   }

  void tester::set_tapos( signed_transaction& trx ) const {
     trx.set_reference_block( control->head_block_id() );
  }


   void tester::create_account( account_name a, account_name creator, bool multisig ) {
      signed_transaction trx;
      set_tapos( trx );

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

      set_tapos(trx);
      trx.sign( get_private_key( creator, "active" ), chain_id_type()  );
      push_transaction( trx );
   }

   transaction_trace tester::push_transaction( packed_transaction& trx ) {
      return control->push_transaction( trx );
   }

   transaction_trace tester::push_transaction( signed_transaction& trx ) {
      auto ptrx = packed_transaction(trx);
      return push_transaction( ptrx );
   }

   tester::action_result tester::push_action(action&& cert_act, uint64_t authorizer) {
      signed_transaction trx;
      if (authorizer) {
         cert_act.authorization = vector<permission_level>{{authorizer, config::active_name}};
      }
      trx.actions.emplace_back(std::move(cert_act));
      set_tapos(trx);
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

   transaction_trace tester::push_reqauth( account_name from, const vector<permission_level>& auths, const vector<private_key_type>& keys ) {
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
      set_tapos( trx );
      for(auto iter = keys.begin(); iter != keys.end(); iter++)
         trx.sign( *iter, chain_id_type() );
      return push_transaction( trx );
   }


   transaction_trace tester::push_nonce(account_name from, const string& v) {
      variant pretty_trx = fc::mutable_variant_object()
         ("actions", fc::variants({
            fc::mutable_variant_object()
               ("account", name(config::system_account_name))
               ("name", "nonce")
               ("authorization", fc::variants({
                  fc::mutable_variant_object()
                     ("actor", from)
                     ("permission", name(config::active_name))
               }))
               ("data", fc::mutable_variant_object()
                  ("value", v)
               )
            })
         );

      signed_transaction trx;
      contracts::abi_serializer::from_variant(pretty_trx, trx, get_resolver());
      set_tapos( trx );

      trx.sign( get_private_key( from, "active" ), chain_id_type() );
      return push_transaction( trx );
   }

   transaction_trace tester::transfer( account_name from, account_name to, string amount, string memo, account_name currency ) {
      return transfer( from, to, asset::from_string(amount), memo, currency );
   }

   transaction_trace tester::transfer( account_name from, account_name to, asset amount, string memo, account_name currency ) {
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
      set_tapos( trx );

      trx.sign( get_private_key( from, name(config::active_name).to_string() ), chain_id_type()  );
      return push_transaction( trx );
   }

   void tester::set_authority( account_name account,
                               permission_name perm,
                               authority auth,
                               permission_name parent ) { try {
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,perm}},
                                contracts::updateauth{
                                   .account    = account,
                                   .permission = perm,
                                   .parent     = parent,
                                   .data       = move(auth),
                                });

      set_tapos( trx );
      trx.sign( get_private_key( account, "active" ), chain_id_type()  ); 
      push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account)(perm)(auth)(parent) ) }

   void tester::set_code( account_name account, const char* wast ) try {
      const auto assemble = [](const char* wast) -> vector<unsigned char> {
         using namespace IR;
         using namespace WAST;
         using namespace WASM;
         using namespace Serialization;

         Module module;
         vector<Error> parse_errors;
         parseModule(wast, fc::const_strlen(wast), module, parse_errors);
         if (!parse_errors.empty()) {
            fc::exception parse_exception(
               FC_LOG_MESSAGE(warn, "Failed to parse WAST"),
               fc::std_exception_code,
               "wast_parse_error",
               "Failed to parse WAST"
            );

            for (const auto& err: parse_errors) {
               parse_exception.append_log( FC_LOG_MESSAGE(error, ":${desc}: ${message}", ("desc", err.locus.describe())("message", err.message.c_str()) ) );
               parse_exception.append_log( FC_LOG_MESSAGE(error, string(err.locus.column(8), ' ') + "^" ));
            }

            throw parse_exception;
         }

         try {
            // Serialize the WebAssembly module.
            ArrayOutputStream stream;
            serialize(stream,module);
            return stream.getBytes();
         } catch(const FatalSerializationException& ex) {
            fc::exception serialize_exception (
               FC_LOG_MESSAGE(warn, "Failed to serialize wasm: ${message}", ("message", ex.message)),
               fc::std_exception_code,
               "wasm_serialization_error",
               "Failed to serialize WASM"
            );
            throw serialize_exception;
         }
      };

      auto wasm = assemble(wast);

      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                contracts::setcode{
                                   .account    = account,
                                   .vmtype     = 0,
                                   .vmversion  = 0,
                                   .code       = bytes(wasm.begin(), wasm.end())
                                });

      set_tapos( trx );
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      push_transaction( trx );
   } FC_CAPTURE_AND_RETHROW( (account)(wast) )

   void tester::set_abi( account_name account, const char* abi_json) {
      auto abi = fc::json::from_string(abi_json).template as<contracts::abi_def>();
      signed_transaction trx;
      trx.actions.emplace_back( vector<permission_level>{{account,config::active_name}},
                                contracts::setabi{
                                   .account    = account,
                                   .abi        = abi
                                });

      set_tapos( trx );
      trx.sign( get_private_key( account, "active" ), chain_id_type()  );
      push_transaction( trx );
   }

   bool tester::chain_has_transaction( const transaction_id_type& txid ) const {
      return chain_transactions.count(txid) != 0;
   }

   const transaction_receipt& tester::get_transaction_receipt( const transaction_id_type& txid ) const {
      return chain_transactions.at(txid);
   }

   /**
    *  Reads balance as stored by generic_currency contract
    */
   asset tester::get_currency_balance( const account_name& code,
                                       const symbol&       asset_symbol,
                                       const account_name& account ) const {
      const auto& db  = control->get_database();
      const auto* tbl = db.find<contracts::table_id_object, contracts::by_code_scope_table>(boost::make_tuple(code, account, N(account)));
      share_type result = 0;

      // the balance is implied to be 0 if either the table or row does not exist
      if (tbl) {
         const auto *obj = db.find<contracts::key_value_object, contracts::by_scope_primary>(boost::make_tuple(tbl->id, asset_symbol.value()));
         if (obj) {
            fc::datastream<const char *> ds(obj->value.data(), obj->value.size());
            fc::raw::unpack(ds, result);
         }
      }
      return asset(result, asset_symbol);
   }

   vector<uint8_t> tester::to_uint8_vector(const string& s) {
      vector<uint8_t> v(s.size());
      copy(s.begin(), s.end(), v.begin());
      return v;
   };

   vector<uint8_t> tester::to_uint8_vector(uint64_t x) {
      vector<uint8_t> v(sizeof(x));
      *reinterpret_cast<uint64_t*>(v.data()) = x;
      return v;
   };

   uint64_t tester::to_uint64(fc::variant x) {
      vector<uint8_t> blob;
      fc::from_variant<uint8_t>(x, blob);
      FC_ASSERT(8 == blob.size());
      return *reinterpret_cast<uint64_t*>(blob.data());
   }

   string tester::to_string(fc::variant x) {
      vector<uint8_t> v;
      fc::from_variant<uint8_t>(x, v);
      string s(v.size(), 0);
      copy(v.begin(), v.end(), s.begin());
      return s;
   }

} }  /// eosio::test
