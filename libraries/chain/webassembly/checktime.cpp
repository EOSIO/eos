#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/transaction_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::checktime() {
      context.trx_context.checktime();
   }
}}} // ns eosio::chain::webassembly
