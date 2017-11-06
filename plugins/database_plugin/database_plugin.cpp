/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eos/database_plugin/database_plugin.hpp>

#include <fc/filesystem.hpp>
#include <fc/log/logger.hpp>

namespace eosio {

class database_plugin_impl {
public:
   bool      readonly = false;
   uint64_t  shared_memory_size = 0;
   bfs::path shared_memory_dir;

   fc::optional<chainbase::database> db;
};

database_plugin::database_plugin() : my(new database_plugin_impl){}
database_plugin::~database_plugin(){}

void database_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("readonly", bpo::value<bool>()->default_value(false), "open the database in read only mode")
         ("shared-file-dir", bpo::value<bfs::path>()->default_value("blockchain"),
          "the location of the chain shared memory files (absolute path or relative to application data dir)")
         ("shared-file-size", bpo::value<uint64_t>()->default_value(8*1024),
            "Minimum size MB of database shared memory file")
         ;
}

void database_plugin::wipe_database() {
   if (!my->shared_memory_dir.empty() && !my->db.valid())
      fc::remove_all(my->shared_memory_dir);
   else
      elog("ERROR: database_plugin::wipe_database() called before configuration or after startup. Ignoring.");
}

void database_plugin::plugin_initialize(const variables_map& options) {
   my->shared_memory_dir = app().data_dir() / "blockchain";
   if(options.count("shared-file-dir")) {
      auto sfd = options.at("shared-file-dir").as<bfs::path>();
      if(sfd.is_relative())
         my->shared_memory_dir = app().data_dir() / sfd;
      else
         my->shared_memory_dir = sfd;
   }
   my->shared_memory_size = options.at("shared-file-size").as<uint64_t>() * 1024 * 1024;
   my->readonly = options.at("readonly").as<bool>();
}

void database_plugin::plugin_startup() {
   using chainbase::database;
   my->db = chainbase::database(my->shared_memory_dir,
                                my->readonly? database::read_only : database::read_write,
                                my->shared_memory_size);
}

void database_plugin::plugin_shutdown() {
   ilog("closing database");
   my->db.reset();
   ilog("database closed successfully");
}

chainbase::database& database_plugin::db() {
   assert(my->db.valid());
   return *my->db;
}

const chainbase::database& database_plugin::db() const {
   assert(my->db.valid());
   return *my->db;
}

}
