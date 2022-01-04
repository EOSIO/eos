#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/transaction_context.hpp>
#include <eosio/chain/apply_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   /* these are both unfortunate that we didn't make the return type an int64_t */
   uint64_t interface::current_time() const {
      return static_cast<uint64_t>( context.control.pending_block_time().time_since_epoch().count() );
   }

   uint64_t interface::publication_time() const {
      return static_cast<uint64_t>( context.trx_context.published.time_since_epoch().count() );
   }

   bool interface::is_feature_activated( legacy_ptr<const digest_type> feature_digest ) const {
      return context.control.is_protocol_feature_activated( *feature_digest );
   }

   name interface::get_sender() const {
      return context.get_sender();
   }
}}} // ns eosio::chain::webassembly
