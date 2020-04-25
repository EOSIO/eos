#pragma once
#include <appbase/application.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <fc/variant.hpp>
#include <memory>

namespace fc { class variant; }

namespace eosio {
   namespace test_generic_message {
      using namespace appbase;
      typedef std::shared_ptr<struct test_generic_message_plugin_impl> test_generic_message_ptr;

      namespace test_generic_message_apis {
         struct empty {
         };

         class read_write {

         public:
            read_write( const test_generic_message_ptr& test_generic_message )
               : my( test_generic_message ) {}

            struct type1 {
               bool     f1;
               string   f2;
               uint64_t f3;
            };
            struct send_type1_params {
               std::vector<string> endpoints;
               type1               data;
            };
            using send_type1_results = empty;

            send_type1_results send_type1( const send_type1_params& params ) const;

            struct type2 {
               uint64_t f1;
               bool     f2;
               string   f3;
               string   f4;
            };
            struct send_type2_params {
               std::vector<string> endpoints;
               type2               data;
            };
            using send_type2_results = empty;

            send_type2_results send_type2( const send_type2_params& params ) const;

            struct type3 {
               string   f1;
            };
            struct send_type3_params {
               std::vector<string> endpoints;
               type3               data;
            };
            using send_type3_results = empty;

            send_type3_results send_type3( const send_type3_params& params ) const;

            struct registered_types_params {
               bool ignore_no_support;
            };
            struct node {
               string         endpoint;
               vector<string> types;
            };
            struct registered_types_results {
               vector<node> nodes;
            };

            registered_types_results registered_types( const registered_types_params& params ) const;

            struct received_data_params {
               vector<string> endpoints;
            };
            struct message {
               string      endpoint;
               fc::variant data;
            };
            struct received_data_results {
               vector<message> messages;
            };

            received_data_results received_data( const received_data_params& params ) const;

         private:
            test_generic_message_ptr my;
         };
      } // namespace test_generic_message_apis

      struct test_generic_message_plugin : public plugin<test_generic_message_plugin> {
         APPBASE_PLUGIN_REQUIRES((chain_plugin))

         test_generic_message_plugin();

         test_generic_message_plugin( const test_generic_message_plugin& ) = delete;

         test_generic_message_plugin( test_generic_message_plugin&& ) = delete;

         test_generic_message_plugin& operator=( const test_generic_message_plugin& ) = delete;

         test_generic_message_plugin& operator=( test_generic_message_plugin&& ) = delete;

         virtual ~test_generic_message_plugin() override = default;

         virtual void set_program_options( options_description& cli, options_description& cfg ) override;

         void plugin_initialize( const variables_map& options );

         void plugin_startup();

         void plugin_shutdown();

         test_generic_message_apis::read_write get_read_write_api() const {
            return test_generic_message_apis::read_write( my );
         }

         test_generic_message_ptr my;
      };

   }
}

FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::empty, )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::type1, (f1)(f2)(f3) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::send_type1_params, (endpoints)(data) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::type2, (f1)(f2)(f3)(f4) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::send_type2_params, (endpoints)(data) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::type3, (f1) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::send_type3_params, (endpoints)(data) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::registered_types_params, (ignore_no_support) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::node, (endpoint)(types) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::registered_types_results, (nodes) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::received_data_params, (endpoints) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::message, (endpoint)(data) )
FC_REFLECT(eosio::test_generic_message::test_generic_message_apis::read_write::received_data_results, (messages) )
