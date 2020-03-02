#pragma once

#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/metadata_log.hpp>
#include <eosio/trace_api_plugin/data_log.hpp>

namespace eosio::trace_api_plugin {

   using now_function = std::function<fc::time_point()>;

   namespace detail {
      class response_formatter {
      public:
         static fc::variant process_block( const block_trace_v0& trace, const  std::optional<lib_entry_v0>& best_lib, const now_function& now, const fc::time_point& deadline );
      };
   }

   class deadline_exceeded : public std::runtime_error {
   public:
      explicit deadline_exceeded(const char* what_arg)
      :std::runtime_error(what_arg)
      {}
      explicit deadline_exceeded(const std::string& what_arg)
            :std::runtime_error(what_arg)
      {}
   };

   class bad_data_exception : public std::runtime_error {
   public:
      explicit bad_data_exception(const char* what_arg)
      :std::runtime_error(what_arg)
      {}

      explicit bad_data_exception(const std::string& what_arg)
      :std::runtime_error(what_arg)
      {}
   };

   template<typename LogfileProvider>
   class request_handler {
   public:
      request_handler(LogfileProvider&& logfile_provider, const now_function& now)
      :logfile_provider(logfile_provider)
      ,now(now)
      {
      }

      /**
       * Fetch the trace for a given block height and convert it to a fc::variant for conversion to a final format
       * (eg JSON)
       *
       * @param block_height - the height of the block whose trace is requested
       * @param deadline - deadline for the processing of the request
       * @return a properly formatted variant representing the trace for the given block height if it exists, an
       * empty variant otherwise.
       * @throws deadline_exceeded when the processing of the request cannot be completed by the provided time.
       */
      fc::variant get_block_trace( uint32_t block_height, const fc::time_point& deadline ) {
         std::optional<uint64_t> block_offset;
         std::optional<lib_entry_v0> best_lib;
         bool deadline_past = false;

         // scan for the block offset and latest LIB
         logfile_provider.scan_metadata_log_from(block_height, 0, [&](const metadata_log_entry& e) -> bool {
            if (e.contains<block_entry_v0>()) {
               const auto& block = e.get<block_entry_v0>();
               if (block.number == block_height) {
                  block_offset = block.offset;
               }
            } else if (e.contains<lib_entry_v0>()) {
               best_lib = e.get<lib_entry_v0>();
               if (best_lib->lib > block_height) {
                  return false;
               }
            }

            if (now() >= deadline) {
               deadline_past = true;
               return false;
            }

            return true;
         });

         if (deadline_past) {
            throw deadline_exceeded("Provided deadline exceeded while parsing metadata");
         }

         // failed to find a metadata entry for the requested block
         if (!block_offset) {
            return {};
         }

         // fetch the block entry
         auto data_entry = logfile_provider.read_data_log(block_height, *block_offset);
         if (now() >= deadline) {
            throw deadline_exceeded("Provided deadline exceeded while fetching block data");
         }

         // data expected but missing
         if (!data_entry) {
            return {};
         }

         if (!data_entry->template contains<block_trace_v0>()) {
            throw bad_data_exception("Expected block_trace_v0 from data log at offset " + std::to_string(*block_offset));
         }

         return detail::response_formatter::process_block(data_entry->template get<block_trace_v0>(), best_lib, now, deadline);
      }

   private:
      LogfileProvider logfile_provider;
      now_function now;
   };


}
