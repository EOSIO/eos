#include "rocksdb_plugin.hpp"

#include <boost/filesystem.hpp>
#include <fc/exception/exception.hpp>
#include "rdb_stdout_logger.hpp"
namespace b1 {

using namespace appbase;
using namespace std::literals;

struct rocksdb_plugin_impl {
   boost::filesystem::path             db_path        = {};
   std::optional<uint32_t>             threads        = {};
   std::optional<uint32_t>             max_open_files = {};
   std::optional<bfs::path>            options_file_name = {};
   std::shared_ptr<chain_kv::database> database       = {};
   std::mutex                          mutex          = {};
   bool                                console_log    = false;
};

static abstract_plugin& _rocksdb_plugin = app().register_plugin<rocksdb_plugin>();

rocksdb_plugin::rocksdb_plugin() : my(std::make_shared<rocksdb_plugin_impl>()) {}

rocksdb_plugin::~rocksdb_plugin() {}

void rocksdb_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op = cfg.add_options();
   op("rdb-database", bpo::value<bfs::path>()->default_value("rodeos.rocksdb"),
      "Database path (absolute path or relative to application data dir)");
   op("rdb-threads", bpo::value<uint32_t>(),
      "Increase number of background RocksDB threads. Only used with cloner_plugin. Recommend 8 for full history "
      "on large chains.");
   op("rdb-max-files", bpo::value<uint32_t>(),
      "RocksDB limit max number of open files (default unlimited). This should be smaller than 'ulimit -n #'. "
      "# should be a very large number for full-history nodes.");
   op("rdb-options-file-name", bpo::value<bfs::path>(),
      "Store RocksDB options. Must follow INI file format. Consult RocksDB documentation for details."
      "If rdb-options-file-name is present, rdb-threads and rdb-max-files are ignored.");
   op("rdb-console-log", bpo::bool_switch()->default_value(false), "output rockdb log to stdout");
}

void rocksdb_plugin::plugin_initialize(const variables_map& options) {
   try {
      auto db_path = options.at("rdb-database").as<bfs::path>();
      if (db_path.is_relative())
         my->db_path = app().data_dir() / db_path;
      else
         my->db_path = db_path;
      if (!options["rdb-threads"].empty())
         my->threads = options["rdb-threads"].as<uint32_t>();
      if (!options["rdb-max-files"].empty())
         my->max_open_files = options["rdb-max-files"].as<uint32_t>();
      if (!options["rdb-options-file-name"].empty()) {
         my->options_file_name = options["rdb-options-file-name"].as<bfs::path>();
         EOS_ASSERT( bfs::exists(*my->options_file_name), eosio::chain::plugin_config_exception, "options file ${f} does not exist.", ("f", my->options_file_name->string()) );
      }
      my->console_log = options["rdb-console-log"].as<bool>();
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

      auto logger  = my->console_log ? std::make_shared<rdb_stdout_logger>() : std::shared_ptr<rdb_stdout_logger>{};
      my->database = std::make_shared<chain_kv::database>(my->db_path.c_str(), true, my->threads, my->max_open_files,
                                                          my->options_file_name, logger);
   }
   return my->database;
}

} // namespace b1
