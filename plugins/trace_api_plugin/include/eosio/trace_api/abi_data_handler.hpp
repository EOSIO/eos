#pragma once

#include <eosio/chain/abi_def.hpp>
#include <eosio/trace_api/trace.hpp>
#include <eosio/trace_api/common.hpp>

namespace eosio {
   namespace chain {
      struct abi_serializer;
   }

   namespace trace_api {

   /**
    * Data Handler that uses eosio::chain::abi_serializer to decode data with a known set of ABI's
    * Can be used directly as a Data_handler_provider OR shared between request_handlers using the
    * ::shared_provider abstraction.
    */
   class abi_data_handler {
   public:
      explicit abi_data_handler( exception_handler except_handler )
      :except_handler( std::move( except_handler ) )
      {
      }

      /**
       * Add an ABI definition to this data handler
       * @param name - the name of the account/contract that this ABI belongs to
       * @param abi - the ABI definition of that ABI
       */
      void add_abi( const chain::name& name, const chain::abi_def& abi );

      /**
       * Given an action trace, produce a tuple representing the `data` and `return_value` fields in the trace
       *
       * @param action - trace of the action including metadata necessary for finding the ABI
       * @param yield - a yield function to allow cooperation during long running tasks
       * @return tuple with the first element is a variant representing the `data` field of the action interpreted by known ABIs OR an empty variant, the second element representing the `return_value` field of the trace.
       */

      std::tuple<fc::variant, std::optional<fc::variant>> serialize_to_variant(const std::variant<action_trace_v0, action_trace_v1> & action, const yield_function& yield );

      /**
       * Utility class that allows mulitple request_handlers to share the same abi_data_handler
       */
      class shared_provider {
         public:
         explicit shared_provider(const std::shared_ptr<abi_data_handler>& handler)
         :handler(handler)
         {}
         std::tuple<fc::variant, std::optional<fc::variant>> serialize_to_variant( const std::variant<action_trace_v0, action_trace_v1> & action, const yield_function& yield ) {
            return handler->serialize_to_variant(action, yield);
         }

         std::shared_ptr<abi_data_handler> handler;
      };

   private:
      std::map<chain::name, std::shared_ptr<chain::abi_serializer>> abi_serializer_by_account;
      exception_handler except_handler;
   };
} }
