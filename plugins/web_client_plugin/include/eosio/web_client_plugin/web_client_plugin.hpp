#pragma once
#include <memory>                                     /* shared_ptr */

#include <appbase/application.hpp>                    /* appbase */
#include <eosio/web_client_plugin/http_client_interface.hpp>  /* https_client_interface */

namespace eosio {
   using namespace appbase;

   class web_client_plugin : public appbase::plugin<web_client_plugin>
   {
   public:
      web_client_plugin();
      web_client_plugin(web::https_client_interface* client);
      virtual ~web_client_plugin();

      APPBASE_PLUGIN_REQUIRES()
      virtual void set_program_options(options_description&, options_description& cfg) override;

      void plugin_initialize(const variables_map& options);
      void plugin_startup();
      void plugin_shutdown();

      /**
       * @brief raw method for making request. see auth as an alternative 
       * @param schema enum. current values are: HTTP or HTTPS
       * @param host host name in string format. e.g. 127.0.0.1
       * @param port port to use
       * @param method enum. Current values: GET or POST
       * @param path url path in string format
       * @param callback handler to be called on call completion
       * @param post_data post data in case of POST method
       * @param header optional parameter to specify castom headers
       */
      void exec(web::schema_type schema,
                std::string_view host,
                web::port_type port,
                web::method_type method, 
                std::string_view path,
                web::client_handler callback,
                std::string_view post_data,
                const web::http_client_interface::header_map* header);
      /**
       * @brief returns authority object. Authority object itself has a http_client_interface reference,
       * so it is convenient to make http(s) calls from there.
       * example:
       * auth(HTTPS, "127.0.0.1", 8081).url("/v1/snapshot").exec(callback);
       * that way you can retain auth object or any object that it can return (see http_client_interface.hpp) and 
       * use it multiple times:
       * auto server = plugin.auth(HTTPS, "127.0.0.1", 8081);
       * server.path("/v1/snapshot").exec(callback);
       * server.path("/v1/block1").exec(callback);
       * @param schema enum. current values are: HTTP or HTTPS
       * @param host string representation of host name. e.g. 127.0.0.1
       * @param port port to use
       */
      web::authority auth(web::schema_type schema, const std::string& host, web::port_type port);



      private:
      std::shared_ptr<web::https_client_interface> impl;
   };

}
