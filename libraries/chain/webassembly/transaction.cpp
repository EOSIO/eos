#include <eosio/chain/webassembly/interface.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/apply_context.hpp>

namespace eosio { namespace chain { namespace webassembly {
   void interface::send_inline( legacy_span<const char> data ) {
      //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
      EOS_ASSERT( data.size() < context.control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
                 "inline action too big" );

      action act;
      fc::raw::unpack<action>(data.data(), data.size(), act);
      context.execute_inline(std::move(act));
   }

   void interface::send_context_free_inline( legacy_span<const char> data ) {
      //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
      EOS_ASSERT( data.size() < context.control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
                "inline action too big" );

      action act;
      fc::raw::unpack<action>(data.data(), data.size(), act);
      context.execute_context_free_inline(std::move(act));
   }

   void interface::send_deferred( legacy_ptr<const uint128_t> sender_id, account_name payer, legacy_span<const char> data, uint32_t replace_existing) {
      transaction trx;
      fc::raw::unpack<transaction>(data.data(), data.size(), trx);
      context.schedule_deferred_transaction(sender_id.load(), payer, std::move(trx), replace_existing);
   }

   bool interface::cancel_deferred( legacy_ptr<const uint128_t> val ) {
      return context.cancel_deferred_transaction( val.load() );
   }
}}} // ns eosio::chain::webassembly
