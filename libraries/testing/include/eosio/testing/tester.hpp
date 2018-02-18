#pragma once
#include <eosio/chain/chain_controller.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <iostream>

namespace eosio { namespace testing {

   using namespace eosio::chain;


   /**
    *  @class tester
    *  @brief provides utility function to simplify the creation of unit tests
    */
   class tester {
      public:
         typedef string action_result;

         tester(chain_controller::runtime_limits limits = chain_controller::runtime_limits(), bool process_genesis = true);

         void              close();
         void              open();
         void              push_genesis_block();

         signed_block      produce_block( fc::microseconds skip_time = fc::milliseconds(config::block_interval_ms) );
         void              produce_blocks( uint32_t n = 1 );

         transaction_trace push_transaction( packed_transaction& trx );
         transaction_trace push_transaction( signed_transaction& trx );
         action_result     push_action(action&& cert_act, uint64_t authorizer);

         transaction_trace push_action( const account_name& code, const action_name& act, const account_name& signer, const variant_object &data );


         void              set_tapos( signed_transaction& trx ) const;

         void              create_accounts( vector<account_name> names, bool multisig = false ) {
            for( auto n : names ) create_account(n, config::system_account_name, multisig );
         }


         void set_authority( account_name account, permission_name perm, authority auth,
                                     permission_name parent = config::owner_name );

         void              create_account( account_name name, account_name creator = config::system_account_name, bool multisig = false );

         transaction_trace push_reqauth( account_name from, const vector<permission_level>& auths, const vector<private_key_type>& keys );
         transaction_trace push_nonce( account_name from, const string& v = "blah" );
         transaction_trace transfer( account_name from, account_name to, asset amount, string memo, account_name currency );
         transaction_trace transfer( account_name from, account_name to, string amount, string memo, account_name currency );

         template<typename ObjectType, typename IndexBy, typename... Args>
         const auto& get( Args&&... args ) {
            return control->get_database().get<ObjectType,IndexBy>( forward<Args>(args)... );
         }

         public_key_type   get_public_key( name keyname, string role = "owner" ) const;
         private_key_type  get_private_key( name keyname, string role = "owner" ) const;

         void              set_code( account_name name, const char* wast );
         void              set_abi( account_name name, const char* abi_json );


         unique_ptr<chain_controller> control;

         bool                          chain_has_transaction( const transaction_id_type& txid ) const;
         const transaction_receipt&    get_transaction_receipt( const transaction_id_type& txid ) const;

         asset                         get_currency_balance( const account_name& contract,
                                                             const symbol&       asset_symbol,
                                                             const account_name& account ) const;

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


   private:
         fc::temp_directory                            tempdir;
         chain_controller::controller_config           cfg;

         map<transaction_id_type, transaction_receipt> chain_transactions;
   };

   /**
    * Utility predicate to check whether an FC_ASSERT message ends with a given string
    */
   struct assert_message_is {
      assert_message_is(string expected)
         :expected(expected)
      {}

      bool operator()( const fc::assert_exception& ex ) {
         auto message = ex.get_log().at(0).get_message();
         return boost::algorithm::ends_with(message, expected);
      }

      string expected;
   };


} } /// eosio::testing
