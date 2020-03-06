#pragma once

#include <eosio/chain/abi_def.hpp>
#include <eosio/trace_api_plugin/trace.hpp>
#include <eosio/trace_api_plugin/common.hpp>

namespace eosio {
   namespace chain {
      struct abi_serializer;
   }

   namespace trace_api_plugin {

   /**
    * Data Handler that uses eosio::chain::abi_serializer to decode data with a known set of ABI's
    * Can be used directly as a Data_handler_provider OR shared between request_handlers using the
    * ::shared_provider abstraction.
    */
   class abi_data_handler {
   public:
      explicit abi_data_handler(std::function<void(const exception_with_context&)> log = {})
      :log(log)
      {
      }

      /**
       * Add an ABI definition to this data handler
       * @param name - the name of the account/contract that this ABI belongs to
       * @param abi - the ABI definition of that ABI
       */
      void add_abi( const chain::name& name, const chain::abi_def& abi );

      /**
       * Given an action trace, produce a variant that represents the `data` field in the trace
       *
       * @param action - trace of the action including metadata necessary for finding the ABI
       * @param deadline - deadline for processing
       * @return variant representing the `data` field of the action interpreted by known ABIs OR an empty variant
       */
      fc::variant process_data( const action_trace_v0& action, const fc::time_point& deadline);

      /**
       * Utility class that allows mulitple request_handlers to share the same abi_data_handler
       */
      class shared_provider {
         public:
         explicit shared_provider(const std::shared_ptr<abi_data_handler>& handler)
         :handler(handler)
         {}

         fc::variant process_data( const action_trace_v0& action, const fc::time_point& deadline) {
            return handler->process_data(action, deadline);
         }

         std::shared_ptr<abi_data_handler> handler;
      };

   private:
      std::map<chain::name, std::shared_ptr<chain::abi_serializer>> abi_serializer_by_account;
      std::function<void(const exception_with_context&)> log;
   };
} }
