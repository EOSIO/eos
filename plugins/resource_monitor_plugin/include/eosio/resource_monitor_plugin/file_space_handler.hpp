#pragma once

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>

#include <appbase/application.hpp>
#include <eosio/chain/exceptions.hpp>

namespace bfs = boost::filesystem;

namespace eosio::resource_monitor {
   template<typename SpaceProvider>
   class file_space_handler {
   public:
      file_space_handler(SpaceProvider&& space_provider, boost::asio::io_context& ctx)
      :space_provider(std::move(space_provider)),
      timer{ctx}
      {
      }

      void set_sleep_time(uint32_t sleep_time) {
         sleep_time_in_secs = sleep_time;
      }

      // warning_threshold must be less than shutdown_threshold.
      // set them together so it is simpler to check.
      void set_threshold(uint32_t new_threshold, uint32_t new_warning_threshold) {
         EOS_ASSERT(new_warning_threshold < new_threshold, chain::plugin_config_exception,
                    "warning_threshold ${new_warning_threshold} must be less than threshold ${new_threshold}", ("new_warning_threshold", new_warning_threshold) ("new_threshold", new_threshold));

         shutdown_threshold = new_threshold;
         warning_threshold = new_warning_threshold;
      }

      void set_shutdown_on_exceeded(bool new_shutdown_on_exceeded) {
         shutdown_on_exceeded = new_shutdown_on_exceeded;
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
               wlog( "Unable to get space info for ${path_name}: [code: ${ec}] ${message}. Ignore this failure.",
                  ("path_name", fs.path_name.string())
                  ("ec", ec.value())
                  ("message", ec.message()));

               continue;
            }

            if ( info.available < fs.warning_available ) {
               wlog("Space usage on ${path}'s file system approaching threshold. available: ${available}, warning_available: ${warning_available}", ("path", fs.path_name.string()) ("available", info.available) ("warning_available", fs.warning_available));
               if ( shutdown_on_exceeded ) {
                  wlog("nodeos will shutdown when space usage exceeds threshold ${threshold}%", ("threshold", shutdown_threshold));
               }

               if ( info.available < fs.shutdown_available ) {
                  wlog("Space usage on ${path}'s file system exceeded threshold ${threshold}%, available: ${available}, Capacity: ${capacity}, shutdown_available: ${shutdown_available}", ("path", fs.path_name.string()) ("threshold", shutdown_threshold) ("available", info.available) ("capacity", info.capacity) ("shutdown_available", fs.shutdown_available));

                  return true;
               }
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

         dlog("${path_name}'s file system to be monitored", ("path_name", path_name.string()));

         // If the file system containing the path is already
         // in the filesystem list, do not add it again
         for (auto& fs: filesystems) {
            if (statbuf.st_dev == fs.st_dev) { // Two files belong to the same file system if their device IDs are the same.
               dlog("${path_name}'s file system already monitored", ("path_name", path_name.string()));

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

         auto shutdown_available = (100 - shutdown_threshold) * (info.capacity / 100); // (100 - shutdown_threshold)/100 is the percentage of minimum number of available bytes the file system must maintain  
         auto warning_available = (100 - warning_threshold) * (info.capacity / 100);

         // Add to the list
         filesystems.emplace_back(statbuf.st_dev, shutdown_available, path_name, warning_available);
         
         ilog("${path_name}'s file system monitored. shutdown_available: ${shutdown_available}, capacity: ${capacity}, threshold: ${threshold}", ("path_name", path_name.string()) ("shutdown_available", shutdown_available) ("capacity", info.capacity) ("threshold", shutdown_threshold) );
      }
   
   void space_monitor_loop() {
      if ( is_threshold_exceeded() && shutdown_on_exceeded ) {
         wlog("Shutting down");
         appbase::app().quit(); // This will gracefully stop Nodeos
         return;
      }

      timer.expires_from_now( boost::posix_time::seconds( sleep_time_in_secs ));
      timer.async_wait([this](auto& ec) {
         if ( ec ) {
            wlog("Exit due to error: ${rc}, message: ${message}",
                 ("ec", ec.value())
                 ("message", ec.message()));
            return;
         } else {
           // Loop over
           space_monitor_loop();
         }
      });
   }

   private:
      SpaceProvider space_provider;

      boost::asio::deadline_timer timer;
   
      uint32_t sleep_time_in_secs {2};
      uint32_t shutdown_threshold {90};
      uint32_t warning_threshold {85};
      bool     shutdown_on_exceeded {true};

      struct   filesystem_info {
         dev_t      st_dev; // device id of file system containing "file_path"
         uintmax_t  shutdown_available {0}; // minimum number of available bytes the file system must maintain
         bfs::path  path_name;
         uintmax_t  warning_available {0};  // warning is issued when availabla number of bytese drops below warning_available

         filesystem_info(dev_t dev, uintmax_t available, const bfs::path& path, uintmax_t warning)
         : st_dev(dev),
         shutdown_available(available),
         path_name(path),
         warning_available(warning)
         {
         }
      };

      // Stores file systems to be monitored. Duplicate
      // file systems are not stored.
      std::vector<filesystem_info> filesystems;
   };
}
