#pragma once
#include <eosio/net_plugin/test_generic_message_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>

#include <appbase/application.hpp>
#include <eosio/chain/controller.hpp>

namespace eosio {
   namespace test_generic_message {

      class test_generic_message_api_plugin : public plugin<test_generic_message_api_plugin> {
      public:
         APPBASE_PLUGIN_REQUIRES((test_generic_message_plugin)( http_plugin ))

         test_generic_message_api_plugin();

         virtual ~test_generic_message_api_plugin();

         virtual void set_program_options( options_description&, options_description& ) override;

         void plugin_initialize( const variables_map& );

         void plugin_startup();

         void plugin_shutdown();

      private:
         unique_ptr<class test_generic_message_api_plugin_impl> my;
      };
   }
}
