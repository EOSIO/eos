#pragma once
#include <memory>                                     /* shared_ptr */

#include <appbase/application.hpp>                    /* appbase */
#include <eosio/web_server_plugin/Ihttp_server.hpp>   /* Ihttps_server */

namespace eosio {
   using namespace appbase;

   using Ihttp_server_ptr = std::shared_ptr<web::Ihttp_server>;

   class web_server_plugin : public appbase::plugin<web_server_plugin>
   {
   public:
      using Ihttps_server_ptr = std::shared_ptr<web::Ihttps_server>;

      web_server_plugin();
      web_server_plugin(web::Ihttps_server_factory*);
      virtual ~web_server_plugin();

      APPBASE_PLUGIN_REQUIRES()
      virtual void set_program_options(options_description&, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      void add_handler(std::string_view path, web::server_handler callback);

      Ihttp_server_ptr get_server(web::server_address&& name, boost::asio::io_context* context);

      private:
      std::unique_ptr<class web_server_plugin_impl> impl;
   };

}
