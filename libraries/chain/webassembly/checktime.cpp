#include <eosio/chain/webassembly/interface.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::checktime() {
      ctx.trx_context.checktime();
   }
}}} // ns eosio::chain::webassembly
