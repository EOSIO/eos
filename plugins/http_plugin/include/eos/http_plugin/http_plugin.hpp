#pragma once
#include <appbase/application.hpp>

namespace eos {
   using namespace appbase;

   /** this method type should be called by the url handler and should
    * be passed the HTTP code and BODY to respond with.
    */
   using url_response_callback = std::function<void(int,string)>;

   /** url_handler receives the url, body, and callback handler 
    *
    * The handler must gaurantee that url_response_callback() is called or
    * the connection will hang and result in a memory leak.
    **/
   using url_handler = std::function<void(string,string,url_response_callback)>;

   /**
    *  This plugin starts an HTTP server and dispatches queries to
    *  registered handles based upon URL. The handler is passed the
    *  URL that was requested and a callback method that should be
    *  called with the responce code and body.
    *
    *  The handler will be called from the appbase application io_service
    *  thread.  The callback can be called from any thread and will 
    *  automatically propagate the call to the http thread.
    *
    *  The HTTP service will run in its own thread with its own io_service to
    *  make sure that HTTP request processing does not interfer with other
    *  plugins.  
    */
   class http_plugin : public appbase::plugin<http_plugin>
   {
      public:
        http_plugin();
        ~http_plugin();

        APPBASE_PLUGIN_REQUIRES()
        virtual void set_program_options(options_description&, options_description& cfg) override;

        void plugin_initialize(const variables_map& options);
        void plugin_startup();
        void plugin_shutdown();

        void add_handler(const string& url, const url_handler&);
      private:
        std::unique_ptr<class http_plugin_impl> my;
   };

}
