#pragma once
#include <appbase/application.hpp>
#include <memory>

namespace eosio {

class cppkin_plugin : public appbase::plugin<cppkin_plugin> {

 public:
   APPBASE_PLUGIN_REQUIRES()

   cppkin_plugin();
   virtual ~cppkin_plugin() = default;

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;
   void plugin_initialize(const appbase::variables_map& options);
   void plugin_startup();
   void plugin_shutdown();
   void handle_sighup() override;

 private:
   std::unique_ptr<struct cppkin_plugin_impl> my;
};

} // namespace eosio
