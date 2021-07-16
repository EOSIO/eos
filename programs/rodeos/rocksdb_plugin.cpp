#include "rocksdb_plugin.hpp"

#include <boost/filesystem.hpp>
#include <fc/exception/exception.hpp>
namespace b1 {

using namespace appbase;
using namespace std::literals;

struct rocksdb_plugin_impl {
   boost::filesystem::path             db_path        = {};
   std::optional<bfs::path>            options_file_name = {};
   std::shared_ptr<chain_kv::database> database       = {};
   std::mutex                          mutex          = {};
};

static abstract_plugin& _rocksdb_plugin = app().register_plugin<rocksdb_plugin>();

rocksdb_plugin::rocksdb_plugin() : my(std::make_shared<rocksdb_plugin_impl>()) {}

rocksdb_plugin::~rocksdb_plugin() {}

void rocksdb_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("rdb-database", bpo::value<bfs::path>()->default_value("rodeos.rocksdb"),
      "Database path (absolute path or relative to application data dir)");
   op("rdb-options-file", bpo::value<bfs::path>(),
      "File (including path) store RocksDB options. Must follow INI file format. Consult RocksDB documentation for details.");
}

void rocksdb_plugin::plugin_initialize(const variables_map& options) {
   try {
      auto db_path = options.at("rdb-database").as<bfs::path>();
      if (db_path.is_relative())
         my->db_path = app().data_dir() / db_path;
      else
         my->db_path = db_path;
      if (!options["rdb-options-file"].empty()) {
         my->options_file_name = options["rdb-options-file"].as<bfs::path>();
         EOS_ASSERT( bfs::exists(*my->options_file_name), eosio::chain::plugin_config_exception, "options file ${f} does not exist.", ("f", my->options_file_name->string()) );
      } else {
         wlog("--rdb-options-file is not configured! RocksDB system default options will be used. Check <build-dir>/programs/rodeos/rocksdb_options.ini on how to set options appropriate to your application.");
      }
   }
   FC_LOG_AND_RETHROW()
}

void rocksdb_plugin::plugin_startup() {}

void rocksdb_plugin::plugin_shutdown() {}

std::shared_ptr<chain_kv::database> rocksdb_plugin::get_db() {
   std::lock_guard<std::mutex> lock(my->mutex);
   if (!my->database) {
      ilog("rodeos database is ${d}", ("d", my->db_path.string()));
      if (!bfs::exists(my->db_path.parent_path()))
         bfs::create_directories(my->db_path.parent_path());

      my->database = std::make_shared<chain_kv::database>(my->db_path.c_str(), true, my->options_file_name);
   }
   return my->database;
}

} // namespace b1
