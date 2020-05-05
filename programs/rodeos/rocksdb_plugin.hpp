#pragma once
#include <appbase/application.hpp>
#include <b1/chain_kv/chain_kv.hpp>

namespace b1 {

class rocksdb_plugin : public appbase::plugin<rocksdb_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES()

   rocksdb_plugin();
   virtual ~rocksdb_plugin();

   virtual void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) override;
   void         plugin_initialize(const appbase::variables_map& options);
   void         plugin_startup();
   void         plugin_shutdown();

   std::shared_ptr<b1::chain_kv::database> get_db();

 private:
   std::shared_ptr<struct rocksdb_plugin_impl> my;
};

} // namespace b1
