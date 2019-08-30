#pragma once
#include <eosio/chain/controller.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/account_object.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/unapplied_transaction_queue.hpp>
#include <fc/io/json.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/tuple/tuple_io.hpp>

#include <iosfwd>

#define REQUIRE_EQUAL_OBJECTS(left, right) { auto a = fc::variant( left ); auto b = fc::variant( right ); BOOST_REQUIRE_EQUAL( true, a.is_object() ); \
   BOOST_REQUIRE_EQUAL( true, b.is_object() ); \
   BOOST_REQUIRE_EQUAL_COLLECTIONS( a.get_object().begin(), a.get_object().end(), b.get_object().begin(), b.get_object().end() ); }

#define REQUIRE_MATCHING_OBJECT(left, right) { auto a = fc::variant( left ); auto b = fc::variant( right ); BOOST_REQUIRE_EQUAL( true, a.is_object() ); \
   BOOST_REQUIRE_EQUAL( true, b.is_object() ); \
   auto filtered = ::eosio::testing::filter_fields( a.get_object(), b.get_object() ); \
   BOOST_REQUIRE_EQUAL_COLLECTIONS( a.get_object().begin(), a.get_object().end(), filtered.begin(), filtered.end() ); }

std::ostream& operator<<( std::ostream& osm, const fc::variant& v );

std::ostream& operator<<( std::ostream& osm, const fc::variant_object& v );

std::ostream& operator<<( std::ostream& osm, const fc::variant_object::entry& e );

eosio::chain::asset core_from_string(const std::string& s);

namespace boost { namespace test_tools { namespace tt_detail {

   template<>
   struct print_log_value<fc::variant> {
      void operator()( std::ostream& osm, const fc::variant& v )
      {
         ::operator<<( osm, v );
      }
   };

   template<>
   struct print_log_value<fc::variant_object> {
      void operator()( std::ostream& osm, const fc::variant_object& v )
      {
         ::operator<<( osm, v );
      }
   };

   template<>
   struct print_log_value<fc::variant_object::entry> {
      void operator()( std::ostream& osm, const fc::variant_object::entry& e )
      {
         ::operator<<( osm, e );
      }
   };

} } }

namespace eosio { namespace testing {
   enum class setup_policy {
      none,
      old_bios_only,
      preactivate_feature_only,
      preactivate_feature_and_new_bios,
      full
   };

   std::vector<uint8_t> read_wasm( const char* fn );
   std::vector<char>    read_abi( const char* fn );
   std::string          read_wast( const char* fn );

   std::string          read_binary_snapshot( const char* fn );
   fc::variant          read_json_snapshot( const char* fn );

   using namespace eosio::chain;

   fc::variant_object filter_fields(const fc::variant_object& filter, const fc::variant_object& value);

   void copy_row(const chain::key_value_object& obj, vector<char>& data);

   bool expect_assert_message(const fc::exception& ex, string expected);

   using subjective_restriction_map = std::map<builtin_protocol_feature_t, protocol_feature_subjective_restrictions>;

   protocol_feature_set make_protocol_feature_set(const subjective_restriction_map& custom_subjective_restrictions = {});

   /**
    *  @class tester
    *  @brief provides utility function to simplify the creation of unit tests
    */
   class base_tester {
      public:
         typedef string action_result;

         static const uint32_t DEFAULT_EXPIRATION_DELTA = 6;

         static const uint32_t DEFAULT_BILLED_CPU_TIME_US = 2000;
         static const fc::microseconds abi_serializer_max_time;

         virtual ~base_tester() {};

         void              init(const setup_policy policy = setup_policy::full, db_read_mode read_mode = db_read_mode::SPECULATIVE);
         void              init(controller::config config, const snapshot_reader_ptr& snapshot = nullptr);
         void              init(controller::config config, protocol_feature_set&& pfs, const snapshot_reader_ptr& snapshot = nullptr);
         void              execute_setup_policy(const setup_policy policy);

         void              close();
         void              open( protocol_feature_set&& pfs, const snapshot_reader_ptr& snapshot);
         void              open( const snapshot_reader_ptr& snapshot);
         bool              is_same_chain( base_tester& other );

         virtual signed_block_ptr produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) ) = 0;
         virtual signed_block_ptr produce_empty_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) ) = 0;
         virtual signed_block_ptr finish_block() = 0;
         void                 produce_blocks( uint32_t n = 1, bool empty = false );
         void                 produce_blocks_until_end_of_round();
         void                 produce_blocks_for_n_rounds(const uint32_t num_of_rounds = 1);
         // Produce minimal number of blocks as possible to spend the given time without having any producer become inactive
         void                 produce_min_num_of_blocks_to_spend_time_wo_inactive_prod(const fc::microseconds target_elapsed_time = fc::microseconds());
         void                 push_block(signed_block_ptr b);

         /**
          * These transaction IDs represent transactions available in the head chain state as scheduled
          * or otherwise generated transactions.
          *
          * calling push_scheduled_transaction with these IDs will remove the associated transaction from
          * the chain state IFF it succeeds or objectively fails
          *
          * @return
          */
         vector<transaction_id_type> get_scheduled_transactions() const;
         unapplied_transaction_queue& get_unapplied_transaction_queue() { return unapplied_transactions; }

         transaction_trace_ptr    push_transaction( packed_transaction& trx, fc::time_point deadline = fc::time_point::maximum(), uint32_t billed_cpu_time_us = DEFAULT_BILLED_CPU_TIME_US );
         transaction_trace_ptr    push_transaction( signed_transaction& trx, fc::time_point deadline = fc::time_point::maximum(), uint32_t billed_cpu_time_us = DEFAULT_BILLED_CPU_TIME_US, bool no_throw = false );

         [[nodiscard]]
         action_result            push_action(action&& cert_act, uint64_t authorizer); // TODO/QUESTION: Is this needed?

         transaction_trace_ptr    push_action( const account_name& code,
                                               const action_name& acttype,
                                               const account_name& actor,
                                               const variant_object& data,
                                               uint32_t expiration = DEFAULT_EXPIRATION_DELTA,
                                               uint32_t delay_sec = 0 );
         transaction_trace_ptr    push_action( const account_name& code,
                                               const action_name& acttype,
                                               const vector<account_name>& actors,
                                               const variant_object& data,
                                               uint32_t expiration = DEFAULT_EXPIRATION_DELTA,
                                               uint32_t delay_sec = 0 );
         transaction_trace_ptr    push_action( const account_name& code,
                                               const action_name& acttype,
                                               const vector<permission_level>& auths,
                                               const variant_object& data,
                                               uint32_t expiration = DEFAULT_EXPIRATION_DELTA,
                                               uint32_t delay_sec = 0 );


         action get_action( account_name code, action_name acttype, vector<permission_level> auths,
                                         const variant_object& data )const;

         void  set_transaction_headers( transaction& trx,
                                        uint32_t expiration = DEFAULT_EXPIRATION_DELTA,
                                        uint32_t delay_sec = 0 )const;

         vector<transaction_trace_ptr>  create_accounts( vector<account_name> names,
                                                         bool multisig = false,
                                                         bool include_code = true
                                                       )
         {
            vector<transaction_trace_ptr> traces;
            traces.reserve(names.size());
            for( auto n : names ) traces.emplace_back( create_account( n, config::system_account_name, multisig, include_code ) );
            return traces;
         }

         void                  set_before_preactivate_bios_contract();
         void                  set_before_producer_authority_bios_contract();
         void                  set_bios_contract();

         vector<producer_authority>  get_producer_authorities( const vector<account_name>& producer_names )const;
         transaction_trace_ptr       set_producers(const vector<account_name>& producer_names);
         transaction_trace_ptr       set_producer_schedule(const vector<producer_authority>& schedule);
         transaction_trace_ptr       set_producers_legacy(const vector<account_name>& producer_names);

         void link_authority( account_name account, account_name code,  permission_name req, action_name type = {} );
         void unlink_authority( account_name account, account_name code, action_name type = {} );
         void set_authority( account_name account, permission_name perm, authority auth,
                                     permission_name parent, const vector<permission_level>& auths, const vector<private_key_type>& keys );
         void set_authority( account_name account, permission_name perm, authority auth,
                                     permission_name parent = config::owner_name );
         void delete_authority( account_name account, permission_name perm,  const vector<permission_level>& auths, const vector<private_key_type>& keys );
         void delete_authority( account_name account, permission_name perm );

         transaction_trace_ptr create_account( account_name name,
                                               account_name creator = config::system_account_name,
                                               bool multisig = false,
                                               bool include_code = true
                                             );

         transaction_trace_ptr push_reqauth( account_name from, const vector<permission_level>& auths, const vector<private_key_type>& keys );
         transaction_trace_ptr push_reqauth(account_name from, string role, bool multi_sig = false);
         // use when just want any old non-context free action
         transaction_trace_ptr push_dummy(account_name from, const string& v = "blah", uint32_t billed_cpu_time_us = DEFAULT_BILLED_CPU_TIME_US );
         transaction_trace_ptr transfer( account_name from, account_name to, asset amount, string memo, account_name currency );
         transaction_trace_ptr transfer( account_name from, account_name to, string amount, string memo, account_name currency );
         transaction_trace_ptr issue( account_name to, string amount, account_name currency , string memo);

         template<typename ObjectType>
         const auto& get(const chainbase::oid< ObjectType >& key) {
            return control->db().get<ObjectType>(key);
         }

         template<typename ObjectType, typename IndexBy, typename... Args>
         const auto& get( Args&&... args ) {
            return control->db().get<ObjectType,IndexBy>( forward<Args>(args)... );
         }

         template<typename ObjectType, typename IndexBy, typename... Args>
         const auto* find( Args&&... args ) {
            return control->db().find<ObjectType,IndexBy>( forward<Args>(args)... );
         }

         template< typename KeyType = fc::ecc::private_key_shim >
         static private_key_type get_private_key( name keyname, string role = "owner" ) {
            return private_key_type::regenerate<KeyType>(fc::sha256::hash(keyname.to_string()+role));
         }

         template< typename KeyType = fc::ecc::private_key_shim >
         static public_key_type get_public_key( name keyname, string role = "owner" ) {
            return get_private_key<KeyType>( keyname, role ).get_public_key();
         }

         void              set_code( account_name name, const char* wast, const private_key_type* signer = nullptr );
         void              set_code( account_name name, const vector<uint8_t> wasm, const private_key_type* signer = nullptr  );
         void              set_abi( account_name name, const char* abi_json, const private_key_type* signer = nullptr );

         bool                          chain_has_transaction( const transaction_id_type& txid ) const;
         const transaction_receipt&    get_transaction_receipt( const transaction_id_type& txid ) const;

         asset                         get_currency_balance( const account_name& contract,
                                                             const symbol&       asset_symbol,
                                                             const account_name& account ) const;

         vector<char> get_row_by_account( name code, name scope, name table, const account_name& act ) const;

         map<account_name, block_id_type> get_last_produced_block_map()const { return last_produced_block; };
         void set_last_produced_block_map( const map<account_name, block_id_type>& lpb ) { last_produced_block = lpb; }

         static vector<uint8_t> to_uint8_vector(const string& s);

         static vector<uint8_t> to_uint8_vector(uint64_t x);

         static uint64_t to_uint64(fc::variant x);

         static string to_string(fc::variant x);

         static action_result success() { return string(); }

         static action_result error( const string& msg ) { return msg; }

         static action_result wasm_assert_msg( const string& msg ) { return "assertion failure with message: " + msg; }

         static action_result wasm_assert_code( uint64_t error_code ) { return "assertion failure with error code: " + std::to_string(error_code); }

         auto get_resolver() {
            return [this]( const account_name& name ) -> optional<abi_serializer> {
               try {
                  const auto& accnt = control->db().get<account_object, by_name>( name );
                  abi_def abi;
                  if( abi_serializer::to_abi( accnt.abi, abi )) {
                     return abi_serializer( abi, abi_serializer_max_time );
                  }
                  return optional<abi_serializer>();
               } FC_RETHROW_EXCEPTIONS( error, "Failed to find or parse ABI for ${name}", ("name", name))
            };
         }

         void sync_with(base_tester& other);

         const table_id_object* find_table( name code, name scope, name table );

         // method treats key as a name type, if this is not appropriate in your case, pass require == false and report the correct behavior
         template<typename Object>
         bool get_table_entry(Object& obj, account_name code, account_name scope, account_name table, uint64_t key, bool require = true) {
            auto* maybe_tid = find_table(code, scope, table);
            if( maybe_tid == nullptr ) {
               BOOST_FAIL( "table for code=\"" + code.to_string()
                            + "\" scope=\"" + scope.to_string()
                            + "\" table=\"" + table.to_string()
                            + "\" does not exist"                 );
            }

            auto* o = control->db().find<key_value_object, by_scope_primary>(boost::make_tuple(maybe_tid->id, key));
            if( o == nullptr ) {
               if( require )
                  BOOST_FAIL("object does not exist for primary_key=\"" + name(key).to_string() + "\"");

               return false;
            }

            fc::raw::unpack(o->value.data(), o->value.size(), obj);
            return true;
         }

         const controller::config& get_config() const {
            return cfg;
         }

         void schedule_protocol_features_wo_preactivation(const vector<digest_type> feature_digests);
         void preactivate_protocol_features(const vector<digest_type> feature_digests);
         void preactivate_all_builtin_protocol_features();

      protected:
         signed_block_ptr _produce_block( fc::microseconds skip_time, bool skip_pending_trxs = false );
         void             _start_block(fc::time_point block_time);
         signed_block_ptr _finish_block();

      // Fields:
      protected:
         // tempdir field must come before control so that during destruction the tempdir is deleted only after controller finishes
         fc::temp_directory                            tempdir;
      public:
         unique_ptr<controller> control;
         std::map<chain::public_key_type, chain::private_key_type> block_signing_private_keys;
      protected:
         controller::config                            cfg;
         map<transaction_id_type, transaction_receipt> chain_transactions;
         map<account_name, block_id_type>              last_produced_block;
         unapplied_transaction_queue                   unapplied_transactions;

      public:
         vector<digest_type>                           protocol_features_to_be_activated_wo_preactivation;
   };

   class tester : public base_tester {
   public:
      tester(setup_policy policy = setup_policy::full, db_read_mode read_mode = db_read_mode::SPECULATIVE) {
         init(policy, read_mode);
      }

      tester(controller::config config) {
         init(config);
      }

      tester(controller::config config, protocol_feature_set&& pfs) {
         init(config, std::move(pfs));
      }

      signed_block_ptr produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
         return _produce_block(skip_time, false);
      }

      signed_block_ptr produce_empty_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
         unapplied_transactions.add_aborted( control->abort_block() );
         return _produce_block(skip_time, true);
      }

      signed_block_ptr finish_block()override {
         return _finish_block();
      }

      bool validate() { return true; }
   };

   class validating_tester : public base_tester {
   public:
      virtual ~validating_tester() {
         if( !validating_node ) {
            elog( "~validating_tester() called with empty validating_node; likely in the middle of failure" );
            return;
         }
         try {
            if( num_blocks_to_producer_before_shutdown > 0 )
               produce_blocks( num_blocks_to_producer_before_shutdown );
            if (!skip_validate)
               BOOST_REQUIRE_EQUAL( validate(), true );
         } catch( const fc::exception& e ) {
            wdump((e.to_detail_string()));
         }
      }
      controller::config vcfg;

      static controller::config default_config(fc::temp_directory& tempdir) {
         controller::config vcfg;
         vcfg.blocks_dir      = tempdir.path() / std::string("v_").append(config::default_blocks_dir_name);
         vcfg.state_dir  = tempdir.path() /  std::string("v_").append(config::default_state_dir_name);
         vcfg.state_size = 1024*1024*8;
         vcfg.state_guard_size = 0;
         vcfg.reversible_cache_size = 1024*1024*8;
         vcfg.reversible_guard_size = 0;
         vcfg.contracts_console = false;

         vcfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
         vcfg.genesis.initial_key = get_public_key( config::system_account_name, "active" );

         for(int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
            if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wavm"))
               vcfg.wasm_runtime = chain::wasm_interface::vm_type::wavm;
            else if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wabt"))
               vcfg.wasm_runtime = chain::wasm_interface::vm_type::wabt;
         }
         return vcfg;
      }

      validating_tester(const flat_set<account_name>& trusted_producers = flat_set<account_name>()) {
         vcfg = default_config(tempdir);

         vcfg.trusted_producers = trusted_producers;

         validating_node = std::make_unique<controller>(vcfg, make_protocol_feature_set());
         validating_node->add_indices();
         validating_node->startup( []() { return false; } );

         init();
      }

      validating_tester(controller::config config) {
         FC_ASSERT( config.blocks_dir.filename().generic_string() != "."
                    && config.state_dir.filename().generic_string() != ".", "invalid path names in controller::config" );

         vcfg = config;
         vcfg.blocks_dir = vcfg.blocks_dir.parent_path() / std::string("v_").append( vcfg.blocks_dir.filename().generic_string() );
         vcfg.state_dir  = vcfg.state_dir.parent_path() / std::string("v_").append( vcfg.state_dir.filename().generic_string() );

         validating_node = std::make_unique<controller>(vcfg, make_protocol_feature_set());
         validating_node->add_indices();
         validating_node->startup( []() { return false; } );

         init(config);
      }

      signed_block_ptr produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
         auto sb = _produce_block(skip_time, false);
         auto bsf = validating_node->create_block_state_future( sb );
         validating_node->push_block( bsf, forked_branch_callback{}, trx_meta_cache_lookup{} );

         return sb;
      }

      signed_block_ptr produce_block_no_validation( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) ) {
         return _produce_block(skip_time, false);
      }

      void validate_push_block(const signed_block_ptr& sb) {
         auto bs = validating_node->create_block_state_future( sb );
         validating_node->push_block( bs, forked_branch_callback{}, trx_meta_cache_lookup{} );
      }

      signed_block_ptr produce_empty_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) )override {
         unapplied_transactions.add_aborted( control->abort_block() );
         auto sb = _produce_block(skip_time, true);
         auto bsf = validating_node->create_block_state_future( sb );
         validating_node->push_block( bsf, forked_branch_callback{}, trx_meta_cache_lookup{} );

         return sb;
      }

      signed_block_ptr finish_block()override {
         return _finish_block();
      }

      bool validate() {


        auto hbh = control->head_block_state()->header;
        auto vn_hbh = validating_node->head_block_state()->header;
        bool ok = control->head_block_id() == validating_node->head_block_id() &&
               hbh.previous == vn_hbh.previous &&
               hbh.timestamp == vn_hbh.timestamp &&
               hbh.transaction_mroot == vn_hbh.transaction_mroot &&
               hbh.action_mroot == vn_hbh.action_mroot &&
               hbh.producer == vn_hbh.producer;

        validating_node.reset();
        validating_node = std::make_unique<controller>(vcfg, make_protocol_feature_set());
        validating_node->add_indices();
        validating_node->startup( []() { return false; } );

        return ok;
      }

      unique_ptr<controller>   validating_node;
      uint32_t                 num_blocks_to_producer_before_shutdown = 0;
      bool                     skip_validate = false;
   };

   /**
    * Utility predicate to check whether an fc::exception message is equivalent to a given string
    */
   struct fc_exception_message_is {
      fc_exception_message_is( const string& msg )
            : expected( msg ) {}

      bool operator()( const fc::exception& ex );

      string expected;
   };

  /**
   * Utility predicate to check whether an fc::exception message starts with a given string
   */
  struct fc_exception_message_starts_with {
     fc_exception_message_starts_with( const string& msg )
           : expected( msg ) {}

     bool operator()( const fc::exception& ex );

     string expected;
  };

  /**
   * Utility predicate to check whether an fc::assert_exception message is equivalent to a given string
   */
  struct fc_assert_exception_message_is {
     fc_assert_exception_message_is( const string& msg )
           : expected( msg ) {}

     bool operator()( const fc::assert_exception& ex );

     string expected;
  };

  /**
   * Utility predicate to check whether an fc::assert_exception message starts with a given string
   */
  struct fc_assert_exception_message_starts_with {
     fc_assert_exception_message_starts_with( const string& msg )
           : expected( msg ) {}

     bool operator()( const fc::assert_exception& ex );

     string expected;
  };

  /**
   * Utility predicate to check whether an eosio_assert message is equivalent to a given string
   */
  struct eosio_assert_message_is {
     eosio_assert_message_is( const string& msg )
           : expected( "assertion failure with message: " ) {
        expected.append( msg );
     }

     bool operator()( const eosio_assert_message_exception& ex );

     string expected;
  };

  /**
   * Utility predicate to check whether an eosio_assert message starts with a given string
   */
  struct eosio_assert_message_starts_with {
     eosio_assert_message_starts_with( const string& msg )
           : expected( "assertion failure with message: " ) {
        expected.append( msg );
     }

     bool operator()( const eosio_assert_message_exception& ex );

     string expected;
  };

  /**
   * Utility predicate to check whether an eosio_assert_code error code is equivalent to a given number
   */
  struct eosio_assert_code_is {
     eosio_assert_code_is( uint64_t error_code )
           : expected( "assertion failure with error code: " ) {
        expected.append( std::to_string(error_code) );
     }

     bool operator()( const eosio_assert_code_exception& ex );

     string expected;
  };

} } /// eosio::testing
