#pragma once

#include <fc/variant.hpp>
#include <eosio/trace_api_plugin/metadata_log.hpp>
#include <eosio/trace_api_plugin/data_log.hpp>
#include <eosio/trace_api_plugin/common.hpp>

namespace eosio::trace_api_plugin {
   using data_handler_function = std::function<fc::variant(const action_trace_v0&)>;

   namespace detail {
      class response_formatter {
      public:
         static fc::variant process_block( const block_trace_v0& trace, bool irreversible, const data_handler_function& data_handler, const now_function& now, const fc::time_point& deadline );
      };
   }

   template<typename LogfileProvider, typename DataHandlerProvider>
   class request_handler {
   public:
      request_handler(LogfileProvider&& logfile_provider, DataHandlerProvider&& data_handler_provider, const now_function& now)
      :logfile_provider(std::move(logfile_provider))
      ,data_handler_provider(std::move(data_handler_provider))
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
       * @throws bad_data_exception when there are issues with the underlying data preventing processing.
       */
      fc::variant get_block_trace( uint32_t block_height, const fc::time_point& deadline ) {
         auto data = logfile_provider.get_block(block_height);
         if (!data) {
            return {};
         }

         if (now() >= deadline) {
            throw deadline_exceeded("Provided deadline exceeded while parsing metadata");
         }
         auto data_handler = [this, &deadline](const action_trace_v0& action) -> fc::variant {
            return data_handler_provider.process_data(action, deadline);
         };

         return detail::response_formatter::process_block(std::get<0>(*data), std::get<1>(*data), data_handler, now, deadline);
      }

   private:
      LogfileProvider logfile_provider;
      DataHandlerProvider data_handler_provider;
      now_function now;
   };


}
