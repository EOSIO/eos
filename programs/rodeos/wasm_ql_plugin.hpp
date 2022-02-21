#pragma once
#include "rocksdb_plugin.hpp"

namespace b1 {

class wasm_ql_plugin : public appbase::plugin<wasm_ql_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES((rocksdb_plugin))

   wasm_ql_plugin();
   virtual ~wasm_ql_plugin();

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;
   void         plugin_initialize(const appbase::variables_map& options);
   void         plugin_startup();
   void         plugin_shutdown();

 private:
   std::shared_ptr<struct wasm_ql_plugin_impl> my;
};

} // namespace b1
