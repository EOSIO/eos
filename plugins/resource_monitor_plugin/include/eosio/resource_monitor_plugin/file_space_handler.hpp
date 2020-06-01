#pragma once

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>

#include <appbase/application.hpp>
#include <eosio/chain/exceptions.hpp>

namespace bfs = boost::filesystem;

namespace eosio::resource_monitor {
   template<typename Space_provider>
   class file_space_handler {
   public:
      file_space_handler(Space_provider&& space_provider, boost::asio::io_context& ctx)
      :space_provider(std::move(space_provider)),
      timer{ctx}
      {
      }

      void set_sleep_time(uint32_t sleep_time) {
         sleep_time_in_secs = sleep_time;
      }

      void set_threshold(uint32_t new_threshold) {
         threshold = new_threshold;
      }

      bool is_threshold_exceeded() const {
         // Go over each monitored file system
         for (auto& fs: filesystems) {
            boost::system::error_code ec;
            auto info = space_provider.get_space(fs.path_name, ec);
            if ( ec ) {
               // As the system is running and this plugin is not a critical
               // part of the system, we should not exit.
               // Just report the failure and continue;
               wlog( "Unable to get space info for ${path_name}: [code: ${ec}] ${message}. Ignire this failure.",
                  ("path_name", fs.path_name.string())
                  ("ec", ec.value())
                  ("message", ec.message()));

               continue;
            }

            if (info.available < fs.available_required) {
               wlog("Space usage on ${path}'s file system exceeded configured threshold ${threshold}%, available: ${available}, Capacity: ${capacity}, available_required: ${available_required}", ("path", fs.path_name.string()) ("threshold", threshold) ("available", info.available) ("capacity", info.capacity) ("available_required", fs.available_required));

               return true;
            }
         }

         return false;
      }

      void add_file_system(const bfs::path& path_name) {
         // Get detailed information of the path
         struct stat statbuf;
         auto status = space_provider.get_stat(path_name.string().c_str(), &statbuf);
         EOS_ASSERT(status == 0, chain::plugin_config_exception,
                    "Failed to run stat on ${path} with status ${status}", ("path", path_name.string())("status", status));

         // If the file system containing the path is already
         // in the filesystem list, do not add it again
         for (auto& fs: filesystems) {
            if (statbuf.st_dev == fs.st_dev) { // Two files belong to the same file system if their device IDs are the same.
               ilog("${path_name}'s file system already monitored", ("path_name", path_name.string()));

               return;
            }
         }

         // For efficiency, precalculate threshold values to avoid calculating it
         // everytime we check space usage. Since bfs::space returns
         // available amount, we use minimum available amount as threshold. 
         boost::system::error_code ec;
         auto info = space_provider.get_space(path_name, ec);
         EOS_ASSERT(!ec, chain::plugin_config_exception,
            "Unable to get space info for ${path_name}: [code: ${ec}] ${message}",
            ("path_name", path_name.string())
            ("ec", ec.value())
            ("message", ec.message()));

         auto available_required = (100 - threshold) * (info.capacity / 100); // (100 - threshold) is the percentage of minimum number of available bytes the file system must maintain  

         // Add to the list
         filesystems.emplace_back(statbuf.st_dev, available_required, path_name);
         
         ilog("${path_name}'s file system monitored. available_required: ${available_required}, capacity: ${capacity}, threshold: ${threshold}", ("path_name", path_name.string()) ("available_required", available_required) ("capacity", info.capacity) ("threshold", threshold) );
      }
   
   int space_monitor_loop() {
      if ( is_threshold_exceeded() ) {
         wlog("Shut down");
         appbase::app().quit(); // This will gracefully stop Nodeos
         return 0;
      }

      timer.expires_from_now( boost::posix_time::seconds( sleep_time_in_secs ));
      timer.async_wait([this](auto ec) {
         if ( ec ) {
            wlog("Exit due to error: ${rc}, message: ${message}",
                 ("ec", ec.value())
                 ("message", ec.message()));
            return 1;
         } else {
           // Loop over
           space_monitor_loop();
         }

         return 2;
      });

      return 3;
   }

   private:
      Space_provider space_provider;

      boost::asio::deadline_timer timer;
   
      uint32_t sleep_time_in_secs;
      uint32_t threshold;

      struct   filesystem_info {
         dev_t      st_dev; // device id of file system containing "file_path"
         uintmax_t  available_required; // minimum number of available bytes the file system must maintain
         bfs::path  path_name;

         filesystem_info(dev_t dev, uintmax_t available, const bfs::path& path)
         : st_dev(dev),
         available_required(available),
         path_name(path)
         {
         }
      };

      // Stores file systems to be monitored. Duplicate
      // file systems are not stored.
      std::vector<filesystem_info> filesystems;

   };
}
