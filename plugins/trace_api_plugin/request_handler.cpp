#include <eosio/trace_api_plugin/request_handler.hpp>

#include <fc/variant_object.hpp>
#include <fc/crypto/base64.hpp>

namespace {
   using namespace eosio::trace_api_plugin;

   std::string to_iso8601_datetime( const fc::time_point& t) {
      return (std::string)t + "Z";
   }

   template<typename Yield>
   fc::variants process_authorizations(const std::vector<authorization_trace_v0>& authorizations, Yield&& yield ) {
      fc::variants result;
      result.reserve(authorizations.size());
      for ( const auto& a: authorizations) {
         yield();

         result.emplace_back(fc::mutable_variant_object()
            ("account", a.account.to_string())
            ("permission", a.permission.to_string())
         );
      }

      return result;

   }

   template<typename Yield>
   fc::variants process_actions(const std::vector<action_trace_v0>& actions, const data_handler_function& data_handler, Yield&& yield ) {
      fc::variants result;
      result.reserve(actions.size());
      for ( const auto& a: actions) {
         yield();

         auto action_variant = fc::mutable_variant_object()
               ("receiver", a.receiver.to_string())
               ("account", a.account.to_string())
               ("action", a.action.to_string())
               ("authorization", process_authorizations(a.authorization, std::forward<Yield>(yield)))
               ("data", fc::to_hex(a.data.data(), a.data.size()));

         auto params = data_handler(a);
         if (!params.is_null()) {
            action_variant("params", params);
         }

         result.emplace_back( std::move(action_variant) );
      }

      return result;

   }

   template<typename Yield>
   fc::variants process_transactions(const std::vector<transaction_trace_v0>& transactions, const data_handler_function& data_handler, Yield&& yield  ) {
      fc::variants result;
      result.reserve(transactions.size());
      for ( const auto& t: transactions) {
         yield();

         result.emplace_back(fc::mutable_variant_object()
            ("id", t.id.str())
            ("actions", process_actions(t.actions, data_handler, std::forward<Yield>(yield)))
         );
      }

      return result;
   }

}

namespace eosio::trace_api_plugin::detail {
   fc::variant response_formatter::process_block( const block_trace_v0& trace, bool irreversible, const data_handler_function& data_handler, const now_function& now, const fc::time_point& deadline ) {
      auto yield = [&now, &deadline]() {
         if (now() >= deadline) {
            throw deadline_exceeded("Provided deadline exceeded while processing transaction data");
         }
      };

      return fc::mutable_variant_object()
         ("id", trace.id.str() )
         ("number", trace.number )
         ("previous_id", trace.previous_id.str() )
         ("status", irreversible ? "irreversible" : "pending" )
         ("timestamp", to_iso8601_datetime(trace.timestamp))
         ("producer", trace.producer.to_string())
         ("transactions", process_transactions(trace.transactions, data_handler, yield ));
   }
}
