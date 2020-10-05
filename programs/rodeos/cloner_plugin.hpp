#pragma once
#include "rocksdb_plugin.hpp"

namespace b1 {

class cloner_plugin : public appbase::plugin<cloner_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES((rocksdb_plugin))

   cloner_plugin();
   virtual ~cloner_plugin();

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;
   void         plugin_initialize(const appbase::variables_map& options);
   void         plugin_startup();
   void         plugin_shutdown();

   void set_streamer(std::function<void(const char* data, uint64_t data_size)> streamer_function);

 private:
   std::shared_ptr<struct cloner_plugin_impl> my;
};

} // namespace b1
