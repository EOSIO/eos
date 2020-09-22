#pragma once
#include <memory>                                     /* shared_ptr */

#include <appbase/application.hpp>                    /* appbase */
#include <eosio/web_client_plugin/Ihttp_client.hpp>  /* Ihttps_client */

namespace eosio {
   using namespace appbase;

   class web_client_plugin : public appbase::plugin<web_client_plugin>
   {
   public:
      web_client_plugin();
      web_client_plugin(web::Ihttps_client* client);
      virtual ~web_client_plugin();

      APPBASE_PLUGIN_REQUIRES()
      virtual void set_program_options(options_description&, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      private:
      std::shared_ptr<web::Ihttps_client> impl;
   };

}
