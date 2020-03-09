#pragma once

#include <eosio/trace_api/trace.hpp>
#include <eosio/trace_api/common.hpp>

namespace eosio::trace_api {
   class base64_data_handler {
   public:
      base64_data_handler()
      {}

      fc::variant process_data( const action_trace_v0& action, const yield_function& = {} ) {
         return fc::base64_encode(action.data.data(), action.data.size());
      }
   };
}
