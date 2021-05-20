#pragma once

#include <fc/variant.hpp>
#include <eosio/trace_api/metadata_log.hpp>
#include <eosio/trace_api/data_log.hpp>
#include <eosio/trace_api/common.hpp>

namespace eosio::trace_api {
   using data_handler_function = std::function<std::tuple<fc::variant, std::optional<fc::variant>>( const std::variant<action_trace_v0, action_trace_v1> & action_trace_t, const yield_function&)>;

   namespace detail {
      class response_formatter {
      public:
         static fc::variant process_block( const data_log_entry& trace, bool irreversible, const data_handler_function& data_handler, const yield_function& yield );
      };
   }

   template<typename LogfileProvider, typename DataHandlerProvider>
   class request_handler {
   public:
      request_handler(LogfileProvider&& logfile_provider, DataHandlerProvider&& data_handler_provider)
      :logfile_provider(std::move(logfile_provider))
      ,data_handler_provider(std::move(data_handler_provider))
      {
      }

      /**
       * Fetch the trace for a given block height and convert it to a fc::variant for conversion to a final format
       * (eg JSON)
       *
       * @param block_height - the height of the block whose trace is requested
       * @param yield - a yield function to allow cooperation during long running tasks
       * @return a properly formatted variant representing the trace for the given block height if it exists, an
       * empty variant otherwise.
       * @throws yield_exception if a call to `yield` throws.
       * @throws bad_data_exception when there are issues with the underlying data preventing processing.
       */
      fc::variant get_block_trace( uint32_t block_height, const yield_function& yield = {}) {
         auto data = logfile_provider.get_block(block_height, yield);
         if (!data) {
            return {};
         }

         yield();

         auto data_handler = [this](const auto& action, const yield_function& yield) -> std::tuple<fc::variant, std::optional<fc::variant>> {
            return std::visit([&](const auto& action_trace_t) {
               return data_handler_provider.serialize_to_variant(action_trace_t, yield);
            }, action);
         };

         return detail::response_formatter::process_block(std::get<0>(*data), std::get<1>(*data), data_handler, yield);
      }

   private:
      LogfileProvider logfile_provider;
      DataHandlerProvider data_handler_provider;
   };


}
