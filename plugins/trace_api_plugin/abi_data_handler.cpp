#include <eosio/trace_api_plugin/abi_data_handler.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/trace_api_plugin/request_handler.hpp>

namespace eosio::trace_api_plugin {

   void abi_data_handler::add_abi( const chain::name& name, const chain::abi_def& abi ) {
      abi_serializer_by_account.emplace(name, std::make_shared<chain::abi_serializer>(abi, fc::microseconds::maximum()));
   }

   fc::variant abi_data_handler::process_data(const action_trace_v0& action, const fc::time_point& deadline) {
      if (abi_serializer_by_account.count(action.account) == 0) {
         return fc::to_hex(action.data.data(), action.data.size());
      }

      const auto& serializer_p = abi_serializer_by_account.at(action.account);
      auto type_name = serializer_p->get_action_type(action.action);
      return serializer_p->binary_to_variant( type_name, action.data, fc::microseconds::maximum() );
   }
}