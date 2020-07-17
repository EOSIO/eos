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

   template<typename ActionTrace>
   fc::variants process_actions(const std::vector<ActionTrace>& actions, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1,  const yield_function& yield ) {
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

         auto action_variant = fc::mutable_variant_object();
         if constexpr(std::is_same_v<ActionTrace, action_trace_v0>){
            action_variant
               ("global_sequence", a.global_sequence)
               ("receiver", a.receiver.to_string())
               ("account", a.account.to_string())
               ("action", a.action.to_string())
               ("authorization", process_authorizations(a.authorization, yield))
               ("data", fc::to_hex(a.data.data(), a.data.size()));
            auto [params, return_data] = data_handler_v0(a, yield);
            if (!params.is_null()) {
               action_variant("params", params);
            }
         }
         else if constexpr(std::is_same_v<ActionTrace, action_trace_v1>){
            action_variant
               ("global_sequence", a.global_sequence)
               ("receiver", a.receiver.to_string())
               ("account", a.account.to_string())
               ("action", a.action.to_string())
               ("authorization", process_authorizations(a.authorization, yield))
               ("data", fc::to_hex(a.data.data(), a.data.size()))
               ("return_value", fc::to_hex(a.return_value.data(),a.return_value.size())) ;

            auto [params, return_data] = data_handler_v1(a, yield);
            if (!params.is_null()) {
               action_variant("params", params);
            }
            if(return_data.valid()){
               action_variant("return_data", return_data);
            }
         }

         result.emplace_back( std::move(action_variant) );

      }

      return result;

   }

   template<typename TransactionTrace>
   fc::variants process_transactions(const std::vector<TransactionTrace>& transactions, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1,  const yield_function& yield ) {
      fc::variants result;
      result.reserve(transactions.size());
      for ( const auto& t: transactions) {
         yield();
         if constexpr(std::is_same_v<TransactionTrace, transaction_trace_v0>){
            result.emplace_back(
               fc::mutable_variant_object()
                  ("id", t.id.str())
                  ("actions", process_actions<action_trace_v0>(t.actions, data_handler_v0, data_handler_v1, yield))); 
         }
         else if constexpr(std::is_same_v<TransactionTrace, transaction_trace_v1>){
            result.emplace_back(
               fc::mutable_variant_object()
                  ("id", t.id.str())
                  ("actions", process_actions<action_trace_v0>(t.actions, data_handler_v0, data_handler_v1, yield))
                  ("status", t.status)
                  ("cpu_usage_us", t.cpu_usage_us)
                  ("net_usage_words", t.net_usage_words)
                  ("signatures", t.signatures)
                  ("transaction_header", t.trx_header));
            }
         else if constexpr(std::is_same_v<TransactionTrace, transaction_trace_v2>){
            result.emplace_back(
               fc::mutable_variant_object()
                  ("id", t.id.str())
                  ("actions", process_actions<action_trace_v1>(t.actions, data_handler_v0, data_handler_v1, yield))
                  ("status", t.status)
                  ("cpu_usage_us", t.cpu_usage_us)
                  ("net_usage_words", t.net_usage_words)
                  ("signatures", t.signatures)
                  ("transaction_header", t.trx_header));
           }

     }
     return result;
   }
}

namespace eosio::trace_api::detail {

   fc::variant process_block_trace(const data_log_entry& trace, bool irreversible, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1,   const yield_function& yield ) {
      if  (trace.contains<block_trace_v0>()){
         auto& block_trace = trace.get<block_trace_v0>();
    	   return  fc::mutable_variant_object()
            ("id", block_trace.id.str())
            ("number", block_trace.number )
            ("previous_id", block_trace.previous_id.str())
            ("status", irreversible ? "irreversible" : "pending" )
            ("timestamp", to_iso8601_datetime(block_trace.timestamp))
            ("producer", block_trace.producer.to_string())
            ("transactions", process_transactions<transaction_trace_v0>(block_trace.transactions, data_handler_v0, data_handler_v1, yield )); 
      }
      else if  (trace.contains<block_trace_v1>()) {
         auto& block_trace = trace.get<block_trace_v1>();
    	   return	fc::mutable_variant_object()
            ("id", block_trace.id.str())
            ("number", block_trace.number )
            ("previous_id", block_trace.previous_id.str())
            ("status", irreversible ? "irreversible" : "pending" )
            ("timestamp", to_iso8601_datetime(block_trace.timestamp))
            ("producer", block_trace.producer.to_string())
            ("transaction_mroot", block_trace.transaction_mroot)
            ("action_mroot", block_trace.action_mroot)
            ("schedule_version", block_trace.schedule_version)
            ("transactions", process_transactions<transaction_trace_v1>( block_trace.transactions_v1, data_handler_v0, data_handler_v1, yield )) ;
      }
      else if (trace.contains<block_trace_v2>()){
         auto& block_trace = trace.get<block_trace_v2>();
         return  fc::mutable_variant_object()
            ("id", block_trace.id.str())
            ("number", block_trace.number)
            ("previous_id", block_trace.previous_id.str())
            ("status", irreversible ? "irreversible" : "pending" )
            ("timestamp", to_iso8601_datetime(block_trace.timestamp))
            ("producer", block_trace.producer.to_string())
            ("transaction_mroot", block_trace.transaction_mroot)
            ("action_mroot", block_trace.action_mroot)
            ("schedule_version", block_trace.schedule_version)
            ("transactions", process_transactions<transaction_trace_v2>(block_trace.transactions_v2, data_handler_v0, data_handler_v1, yield )) ;
      }
      else{
         return fc::mutable_variant_object();
      }
   }

   fc::variant response_formatter::process_block( const data_log_entry& trace, bool irreversible, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1 ,   const yield_function& yield ) {
      return process_block_trace( trace, irreversible, data_handler_v0, data_handler_v1, yield);
   }
}
