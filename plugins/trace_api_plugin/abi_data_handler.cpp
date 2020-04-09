#include <eosio/trace_api/abi_data_handler.hpp>
#include <eosio/chain/abi_serializer.hpp>

namespace eosio::trace_api {

   void abi_data_handler::add_abi( const chain::name& name, const chain::abi_def& abi ) {
      // currently abis are operator provided so no need to protect against abuse
      abi_serializer_by_account.emplace(name,
            std::make_shared<chain::abi_serializer>(abi, chain::abi_serializer::create_yield_function(fc::microseconds::maximum())));
   }

   fc::variant abi_data_handler::process_data(const action_trace_v0& action, const yield_function& yield ) {
      if (abi_serializer_by_account.count(action.account) > 0) {
         const auto& serializer_p = abi_serializer_by_account.at(action.account);
         auto type_name = serializer_p->get_action_type(action.action);

         if (!type_name.empty()) {
            try {
               // abi_serializer expects a yield function that takes a recursion depth
               auto abi_yield = [yield](size_t recursion_depth) {
                  yield();
                  EOS_ASSERT( recursion_depth < chain::abi_serializer::max_recursion_depth, chain::abi_recursion_depth_exception,
                              "exceeded max_recursion_depth ${r} ", ("r", chain::abi_serializer::max_recursion_depth) );
               };
               return serializer_p->binary_to_variant(type_name, action.data, abi_yield);
            } catch (...) {
               except_handler(MAKE_EXCEPTION_WITH_CONTEXT(std::current_exception()));
            }
         }
      }

      return {};
   }
}