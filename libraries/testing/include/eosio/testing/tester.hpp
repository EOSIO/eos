#pragma once
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>
#include <fc/io/json.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/test/unit_test.hpp>

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

   using namespace eosio::chain;

   fc::variant_object filter_fields(const fc::variant_object& filter, const fc::variant_object& value);

   /**
    *  @class tester
    *  @brief provides utility function to simplify the creation of unit tests
    */
   class base_tester {
      public:
         typedef string action_result;

         static const uint32_t DEFAULT_EXPIRATION_DELTA = 6;

         void              init(bool push_genesis = true, chain_controller::runtime_limits limits = chain_controller::runtime_limits());
         void              init(chain_controller::controller_config config);

         void              close();
         void              open();

         virtual signed_block produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = skip_missed_block_penalty ) = 0;
         void                 produce_blocks( uint32_t n = 1 );
         void                 produce_blocks_until_end_of_round();
         signed_block         push_block(signed_block b);
         transaction_trace    push_transaction( packed_transaction& trx, uint32_t skip_flag = skip_nothing  );
         transaction_trace    push_transaction( signed_transaction& trx, uint32_t skip_flag = skip_nothing  );
         action_result        push_action(action&& cert_act, uint64_t authorizer);

         transaction_trace    push_action( const account_name& code, const action_name& acttype, const account_name& actor, const variant_object& data, uint32_t expiration = DEFAULT_EXPIRATION_DELTA, uint32_t delay_sec = 0 );
         transaction_trace    push_action( const account_name& code, const action_name& acttype, const vector<account_name>& actors, const variant_object& data, uint32_t expiration = DEFAULT_EXPIRATION_DELTA, uint32_t delay_sec = 0 );

         void                 set_tapos( signed_transaction& trx, uint32_t expiration = DEFAULT_EXPIRATION_DELTA ) const;

         void                 set_transaction_headers(signed_transaction& trx,
                                                      uint32_t expiration = DEFAULT_EXPIRATION_DELTA,
                                                      uint32_t delay_sec = 0)const;

         void                 create_accounts( vector<account_name> names, bool multisig = false ) {
            for( auto n : names ) create_account(n, config::system_account_name, multisig );
         }

         void                 push_genesis_block();
         producer_schedule_type  set_producers(const vector<account_name>& producer_names, const uint32_t version = 0);

         void link_authority( account_name account, account_name code,  permission_name req, action_name type = "" );
         void unlink_authority( account_name account, account_name code, action_name type = "" );
         void set_authority( account_name account, permission_name perm, authority auth,
                                     permission_name parent, const vector<permission_level>& auths, const vector<private_key_type>& keys );
         void set_authority( account_name account, permission_name perm, authority auth,
                                     permission_name parent = config::owner_name );
         void delete_authority( account_name account, permission_name perm,  const vector<permission_level>& auths, const vector<private_key_type>& keys );
         void delete_authority( account_name account, permission_name perm );

         void create_account( account_name name, account_name creator = config::system_account_name, bool multisig = false );

         transaction_trace push_reqauth( account_name from, const vector<permission_level>& auths, const vector<private_key_type>& keys );
         transaction_trace push_reqauth(account_name from, string role, bool multi_sig = false);
         // use when just want any old non-context free action
         transaction_trace push_dummy(account_name from, const string& v = "blah");
         transaction_trace transfer( account_name from, account_name to, asset amount, string memo, account_name currency );
         transaction_trace transfer( account_name from, account_name to, string amount, string memo, account_name currency );
         transaction_trace issue( account_name to, string amount, account_name currency );

         template<typename ObjectType>
         const auto& get(const chainbase::oid< ObjectType >& key) {
            return control->get_database().get<ObjectType>(key);
         }

         template<typename ObjectType, typename IndexBy, typename... Args>
         const auto& get( Args&&... args ) {
            return control->get_database().get<ObjectType,IndexBy>( forward<Args>(args)... );
         }

         template<typename ObjectType, typename IndexBy, typename... Args>
         const auto* find( Args&&... args ) {
            return control->get_database().find<ObjectType,IndexBy>( forward<Args>(args)... );
         }

         public_key_type   get_public_key( name keyname, string role = "owner" ) const;
         private_key_type  get_private_key( name keyname, string role = "owner" ) const;

         void              set_code( account_name name, const char* wast );
         void              set_code( account_name name, const vector<uint8_t> wasm );
         void              set_abi( account_name name, const char* abi_json );


         unique_ptr<chain_controller> control;

         bool                          chain_has_transaction( const transaction_id_type& txid ) const;
         const transaction_receipt&    get_transaction_receipt( const transaction_id_type& txid ) const;

         asset                         get_currency_balance( const account_name& contract,
                                                             const symbol&       asset_symbol,
                                                             const account_name& account ) const;

        vector<char> get_row_by_account( uint64_t code, uint64_t scope, uint64_t table, const account_name& act );

        static vector<uint8_t> to_uint8_vector(const string& s);

        static vector<uint8_t> to_uint8_vector(uint64_t x);

        static uint64_t to_uint64(fc::variant x);

        static string to_string(fc::variant x);

        static action_result success() { return string(); }

        static action_result error(const string& msg) { return msg; }

        auto get_resolver() {
           return [this](const account_name &name) -> optional<contracts::abi_serializer> {
              try {
                 const auto &accnt = control->get_database().get<account_object, by_name>(name);
                 contracts::abi_def abi;
                 if (contracts::abi_serializer::to_abi(accnt.abi, abi)) {
                    return contracts::abi_serializer(abi);
                 }
                 return optional<contracts::abi_serializer>();
              } FC_RETHROW_EXCEPTIONS(error, "Failed to find or parse ABI for ${name}", ("name", name))
           };
        }

       void sync_with(base_tester& other);

   protected:
         signed_block _produce_block( fc::microseconds skip_time, uint32_t skip_flag);
         fc::temp_directory                            tempdir;
         chain_controller::controller_config           cfg;
         map<transaction_id_type, transaction_receipt> chain_transactions;
   };

   class tester : public base_tester {
   public:
      tester(bool push_genesis, chain_controller::runtime_limits limits = chain_controller::runtime_limits()) {
         init(push_genesis, limits);
      }
      tester(chain_controller::runtime_limits limits = chain_controller::runtime_limits()) {
         init(true, limits);
      }

      tester(chain_controller::controller_config config) {
         init(config);
      }

      signed_block produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = skip_missed_block_penalty )override {
         return _produce_block(skip_time, skip_flag);
      }

      bool validate() { return true; }
   };

   class validating_tester : public base_tester {
   public:
      virtual ~validating_tester() {
         produce_block();
         BOOST_REQUIRE_EQUAL( validate(), true );
      }
      validating_tester(chain_controller::runtime_limits limits = chain_controller::runtime_limits()) {
         chain_controller::controller_config vcfg;
         vcfg.block_log_dir      = tempdir.path() / "blocklog";
         vcfg.shared_memory_dir  = tempdir.path() / "shared";
         vcfg.shared_memory_size = 1024*1024*8;

         vcfg.genesis.initial_timestamp = fc::time_point::from_iso_string("2020-01-01T00:00:00.000");
         vcfg.genesis.initial_key = get_public_key( config::system_account_name, "active" );
         vcfg.limits = limits;

         for(int i = 0; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
            if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--binaryen"))
               vcfg.wasm_runtime = chain::wasm_interface::vm_type::binaryen;
            else if(boost::unit_test::framework::master_test_suite().argv[i] == std::string("--wavm"))
               vcfg.wasm_runtime = chain::wasm_interface::vm_type::wavm;
         }

         validating_node = std::make_unique<chain_controller>(vcfg);
         init(true, limits);
      }

      validating_tester(chain_controller::controller_config config) {
         validating_node = std::make_unique<chain_controller>(config);
         init(config);
      }

      signed_block produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms), uint32_t skip_flag = skip_missed_block_penalty )override {
         auto sb = _produce_block(skip_time, skip_flag | 2);
         validating_node->push_block( sb );
         return sb;
      }

      bool validate() {
        auto hbh = control->head_block_header();
        auto vn_hbh = validating_node->head_block_header();
        return control->head_block_id() == validating_node->head_block_id() &&
               hbh.previous == vn_hbh.previous &&
               hbh.timestamp == vn_hbh.timestamp &&
               hbh.transaction_mroot == vn_hbh.transaction_mroot &&
               hbh.action_mroot == vn_hbh.action_mroot &&
               hbh.block_mroot == vn_hbh.block_mroot &&
               hbh.producer == vn_hbh.producer;
      }

      unique_ptr<chain_controller>                  validating_node;
   };

   /**
    * Utility predicate to check whether an FC_ASSERT message ends with a given string
    */
   struct assert_message_is {
      assert_message_is(string expected)
         :expected(expected)
      {}

      bool operator()( const fc::exception& ex ) {
         auto message = ex.get_log().at(0).get_message();
         return boost::algorithm::ends_with(message, expected);
      }

      string expected;
   };


} } /// eosio::testing
