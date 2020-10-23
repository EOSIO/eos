#pragma once
#include <memory>                                              /* shared_ptr */

#include <appbase/application.hpp>                             /* appbase */
#include <eosio/web_server_plugin/http_server_interface.hpp>   /* https_server_interface */
#include <eosio/web_server_plugin/global.hpp>                  /* logger */

namespace eosio {
   using namespace appbase;

   using http_server_generic_interface_ptr = std::shared_ptr<web::http_server_interface>;

   class web_server_plugin : public appbase::plugin<web_server_plugin>
   {
   public:
      using https_server_interface_ptr = std::shared_ptr<web::https_server_interface>;

      web_server_plugin();
      web_server_plugin(web::http_server_generic_factory_interface*);
      virtual ~web_server_plugin();

      APPBASE_PLUGIN_REQUIRES()
      void set_program_options(options_description&, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();
      void handle_sighup() override;

      void add_handler(std::string_view path, web::server_handler callback);

      http_server_generic_interface_ptr get_server(web::server_address&& name, boost::asio::io_context* context);

      private:
      std::unique_ptr<class web_server_plugin_impl> impl;
   };

}
