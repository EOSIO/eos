#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::call_depth_assert() {
      EOS_ASSERT(false, wasm_execution_error, "Exceeded call depth maximum");
   }
}}} // ns eosio::chain::webassembly
