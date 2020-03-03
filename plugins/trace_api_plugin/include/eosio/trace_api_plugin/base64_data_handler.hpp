#pragma once

#include <eosio/trace_api_plugin/trace.hpp>

namespace eosio::trace_api_plugin {
   class base64_data_handler {
   public:
      base64_data_handler()
      {}

      fc::variant process_data( const action_trace_v0& action, const fc::time_point& ) {
         return fc::base64_encode(action.data.data(), action.data.size());
      }
   };
}
