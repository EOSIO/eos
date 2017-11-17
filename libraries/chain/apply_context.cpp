#include <eosio/chain/apply_context.hpp>
#include <eosio/chain/chain_controller.hpp>
#include <eosio/chain/wasm_interface.hpp>

namespace eosio { namespace chain {
void apply_context::exec()
{
   auto native = mutable_controller.find_apply_handler( receiver, act.scope, act.name );
   if( native ) {
      (*native)(*this);
   } else {
      const auto& a = mutable_controller.get_database().get<account_object,by_name>(receiver);

      if (a.code.size() > 0) {
         // get code from cache
         auto code = mutable_controller.get_wasm_cache().checkout_scoped(a.code_version, a.code.data(), a.code.size());

         // get wasm_interface
         auto &wasm = wasm_interface::get();
         wasm.apply(code, *this);
      }
   }
} /// exec()

} } /// eosio::chain
