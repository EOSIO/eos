#include <eosio/trace_api/request_handler.hpp>

#include <algorithm>

#include <fc/variant_object.hpp>

namespace {
   using namespace eosio::trace_api;

   std::string to_iso8601_datetime( const fc::time_point& t) {
      return (std::string)t + "Z";
   }

   fc::variants process_authorizations(const std::vector<authorization_trace_v0>& authorizations, const yield_function& yield ) {
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

   fc::variants process_actions(const std::vector<action_trace_v0>& actions, const data_handler_function& data_handler, const yield_function& yield ) {
      fc::variants result;
      result.reserve(actions.size());

      // create a vector of indices to sort based on actions to avoid copies
      std::vector<int> indices(actions.size());
      std::iota(indices.begin(), indices.end(), 0);
      std::sort(indices.begin(), indices.end(), [&actions](const int& lhs, const int& rhs) -> bool {
         return actions.at(lhs).global_sequence < actions.at(rhs).global_sequence;
      });

      for ( int index : indices) {
         yield();

         const auto& a = actions.at(index);
         auto action_variant = fc::mutable_variant_object()
               ("global_sequence", a.global_sequence)
               ("receiver", a.receiver.to_string())
               ("account", a.account.to_string())
               ("action", a.action.to_string())
               ("authorization", process_authorizations(a.authorization, yield))
               ("data", fc::to_hex(a.data.data(), a.data.size()));

         auto params = data_handler(a, yield);
         if (!params.is_null()) {
            action_variant("params", params);
         }

         result.emplace_back( std::move(action_variant) );
      }

      return result;

   }

   fc::variants process_transactions(const std::vector<transaction_trace_v0>& transactions, const data_handler_function& data_handler, const yield_function& yield ) {
      fc::variants result;
      result.reserve(transactions.size());
      for ( const auto& t: transactions) {
         yield();

         result.emplace_back(fc::mutable_variant_object()
            ("id", t.id.str())
            ("actions", process_actions(t.actions, data_handler, yield))
         );
      }

      return result;
   }

    fc::variants process_transactions(const std::vector<transaction_trace_v1>& transactions, const data_handler_function& data_handler, const yield_function& yield ) {
        fc::variants result;
        result.reserve(transactions.size());
        for ( const auto& t: transactions) {
            yield();
            result.emplace_back(fc::mutable_variant_object()
               ("id", t.id.str())
               ("actions", process_actions(t.actions, data_handler, yield))
               ("status", t.status)
               ("cpu_usage_us", t.cpu_usage_us)
               ("net_usage_words", t.net_usage_words)
               ("signatures", t.signatures)
               ("transaction_header", t.trx_header)
            );
        }

        return result;
    }

}

namespace eosio::trace_api::detail {
   fc::variant process_block_trace( const block_trace_v0& trace, bool irreversible, const data_handler_function& data_handler, const yield_function& yield ) {
      return fc::mutable_variant_object()
         ("id", trace.id.str() )
         ("number", trace.number )
         ("previous_id", trace.previous_id.str() )
         ("status", irreversible ? "irreversible" : "pending" )
         ("timestamp", to_iso8601_datetime(trace.timestamp))
         ("producer", trace.producer.to_string())
         ("transactions", process_transactions(trace.transactions, data_handler, yield ));
   }

    fc::variant process_block_trace( const block_trace_v1& trace, bool irreversible, const data_handler_function& data_handler, const yield_function& yield ) {
        return fc::mutable_variant_object()
        ("id", trace.id.str() )
        ("number", trace.number )
        ("previous_id", trace.previous_id.str() )
        ("status", irreversible ? "irreversible" : "pending" )
        ("timestamp", to_iso8601_datetime(trace.timestamp))
        ("producer", trace.producer.to_string())
        ("transaction_mroot", trace.transaction_mroot)
        ("action_mroot", trace.action_mroot)
        ("schedule_version", trace.schedule_version)
        ("transactions", process_transactions(trace.transactions_v1, data_handler, yield ));
    }
    fc::variant response_formatter::process_block( const data_log_entry& trace, bool irreversible, const data_handler_function& data_handler, const yield_function& yield ) {
        if (trace.contains<block_trace_v0>()) return process_block_trace(trace.get<block_trace_v0>(), irreversible, data_handler, yield);
        else return process_block_trace(trace.get<block_trace_v1>(), irreversible, data_handler, yield);
    }
}
