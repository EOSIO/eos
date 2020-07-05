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
   fc::variants process_actions(trace_api_type_enum  e_action_trace, const vec_action_trace_variant  && actions, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1,  const return_handler_function & return_handler, const yield_function& yield ) {
      fc::variants result;
      result.reserve(actions.size());

      // create a vector of indices to sort based on actions to avoid copies
      std::vector<int> indices(actions.size());
      std::iota(indices.begin(), indices.end(), 0);
      std::sort(indices.begin(), indices.end(), [e_action_trace, &actions](const int& lhs, const int& rhs) -> bool {
         return  e_action_trace == e_action_trace_v0 ? std::get<action_trace_v0>(actions.at(lhs)).global_sequence < std::get<action_trace_v0>(actions.at(rhs)).global_sequence :
		 	std::get<action_trace_v1>(actions.at(lhs)).global_sequence < std::get<action_trace_v1>(actions.at(rhs)).global_sequence;
      });

      for ( int index : indices) {
         yield();

         const auto& a = actions.at(index);
         auto action_variant =  e_action_trace ==  e_action_trace_v0?   fc::mutable_variant_object()
               ("global_sequence", std::get<action_trace_v0>(a).global_sequence)
               ("receiver", std::get<action_trace_v0>(a).receiver.to_string())
               ("account", std::get<action_trace_v0>(a).account.to_string())
               ("action", std::get<action_trace_v0>(a).action.to_string())
               ("authorization", process_authorizations(std::get<action_trace_v0>(a).authorization, yield))
               ("data", fc::to_hex(std::get<action_trace_v0>(a).data.data(), std::get<action_trace_v0>(a).data.size())) :
            e_action_trace == e_action_trace_v1 ?  fc::mutable_variant_object()
               ("global_sequence", std::get<action_trace_v1>(a).global_sequence)
               ("receiver", std::get<action_trace_v1>(a).receiver.to_string())
               ("account", std::get<action_trace_v1>(a).account.to_string())
               ("action", std::get<action_trace_v1>(a).action.to_string())
               ("authorization", process_authorizations(std::get<action_trace_v1>(a).authorization, yield))
               ("data", fc::to_hex(std::get<action_trace_v1>(a).data.data(), std::get<action_trace_v1>(a).data.size())) 
               ("return_value", fc::to_hex(std::get<action_trace_v1>(a).return_value.data(), std::get<action_trace_v1>(a).return_value.size())) : fc::mutable_variant_object();

         auto params = e_action_trace == e_action_trace_v0 ? data_handler_v0(std::get<action_trace_v0>(a), yield) : data_handler_v1(std::get<action_trace_v1>(a), yield) ;
         if (!params.is_null()) {
            action_variant("params", params);
         }
         if(e_action_trace == e_action_trace_v1){
            auto return_data = return_handler(std::get<action_trace_v1>(a), yield);
            if(!return_data.is_null()){
               action_variant("return_data", return_data);
            }
         }

         result.emplace_back( std::move(action_variant) );

      }

      return result;

   }

  
   template<typename action_trace>
   vec_action_trace_variant to_action_trace_variant(const std::vector<action_trace> & actions){
       vec_action_trace_variant vv;
	for (auto &  action : actions){
		vv.push_back(action);
	}
	return vv;
   }

   fc::variants process_transactions(trace_api_type_enum e_transaction_trace, vec_transaction_trace_variant && transactions, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1,  const return_handler_function & return_handler, const yield_function& yield ) {
      fc::variants result;
      result.reserve(transactions.size());
      for ( const auto& t: transactions) {
         yield();

         result.emplace_back(
            e_transaction_trace == e_transaction_trace_v0 ? fc::mutable_variant_object()
            ("id", std::get<transaction_trace_v0>(t).id.str())
            ("actions", process_actions(e_action_trace_v0, 
                  to_action_trace_variant<action_trace_v0>(std::get<transaction_trace_v0>(t).actions), data_handler_v0, data_handler_v1, return_handler, yield))  :
            e_transaction_trace == e_transaction_trace_v1 ? fc::mutable_variant_object()
               ("id", std::get<transaction_trace_v1>(t).id.str())
               ("actions", process_actions(e_action_trace_v0, 
                   to_action_trace_variant<action_trace_v0>(std::get<transaction_trace_v1>(t).actions), data_handler_v0, data_handler_v1, return_handler, yield))
               ("status", std::get<transaction_trace_v1>(t).status)
               ("cpu_usage_us", std::get<transaction_trace_v1>(t).cpu_usage_us)
               ("net_usage_words", std::get<transaction_trace_v1>(t).net_usage_words)
               ("signatures", std::get<transaction_trace_v1>(t).signatures)
               ("transaction_header", std::get<transaction_trace_v1>(t).trx_header)  :
            e_transaction_trace == e_transaction_trace_v2 ? fc::mutable_variant_object()
            ("id", std::get<transaction_trace_v2>(t).id.str())
            ("actions", process_actions(e_action_trace_v1, 
                 to_action_trace_variant<action_trace_v1>(std::get<transaction_trace_v2>(t).actions), data_handler_v0, data_handler_v1, return_handler,  yield)) :
            e_transaction_trace == e_transaction_trace_v3 ? fc::mutable_variant_object()
               ("id", std::get<transaction_trace_v3>(t).id.str())
               ("actions", process_actions(e_action_trace_v1, 
                  to_action_trace_variant<action_trace_v1>(std::get<transaction_trace_v3>(t).actions), data_handler_v0, data_handler_v1, return_handler, yield))
               ("status", std::get<transaction_trace_v3>(t).status)
               ("cpu_usage_us", std::get<transaction_trace_v3>(t).cpu_usage_us)
               ("net_usage_words", std::get<transaction_trace_v3>(t).net_usage_words)
               ("signatures", std::get<transaction_trace_v3>(t).signatures)
               ("transaction_header", std::get<transaction_trace_v3>(t).trx_header) : fc::mutable_variant_object()
         );
      }

      return result;
   }

}

namespace eosio::trace_api::detail {

   template<typename transaction_trace>
   vec_transaction_trace_variant to_transaction_trace_variant(const std::vector<transaction_trace> & transactions){
       vec_transaction_trace_variant vv;
	for (auto &  transaction : transactions){
		vv.push_back(transaction);
	}
	return vv;
   }
   fc::variant process_block_trace( trace_api_type_enum e_block_trace, const data_log_entry& trace, bool irreversible, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1,  const return_handler_function & return_handler,  const yield_function& yield ) {
      return 
      e_block_trace == e_block_trace_v0 ?    fc::mutable_variant_object()
         ("id", trace.get<block_trace_v0>().id.str() )
         ("number", trace.get<block_trace_v0>().number )
         ("previous_id", trace.get<block_trace_v0>().previous_id.str() )
         ("status", irreversible ? "irreversible" : "pending" )
         ("timestamp", to_iso8601_datetime(trace.get<block_trace_v0>().timestamp))
         ("producer", trace.get<block_trace_v0>().producer.to_string())
         ("transactions", process_transactions(e_transaction_trace_v0, 
              to_transaction_trace_variant<transaction_trace_v0>(trace.get<block_trace_v0>().transactions), data_handler_v0, data_handler_v1, return_handler, yield )) : 
      e_block_trace == e_block_trace_v1 ?    fc::mutable_variant_object()
        ("id", trace.get<block_trace_v1>().id.str() )
        ("number", trace.get<block_trace_v1>().number )
        ("previous_id", trace.get<block_trace_v1>().previous_id.str() )
        ("status", irreversible ? "irreversible" : "pending" )
        ("timestamp", to_iso8601_datetime(trace.get<block_trace_v1>().timestamp))
        ("producer", trace.get<block_trace_v1>().producer.to_string())
        ("transaction_mroot", trace.get<block_trace_v1>().transaction_mroot)
        ("action_mroot", trace.get<block_trace_v1>().action_mroot)
        ("schedule_version", trace.get<block_trace_v1>().schedule_version)
        ("transactions", process_transactions(e_transaction_trace_v1, 
            to_transaction_trace_variant<transaction_trace_v1>(trace.get<block_trace_v1>().transactions_v1), data_handler_v0, data_handler_v1, return_handler, yield )) :
      e_block_trace == e_block_trace_v2 ?  fc::mutable_variant_object()
         ("id", trace.get<block_trace_v2>().id.str() )
         ("number", trace.get<block_trace_v2>().number )
         ("previous_id", trace.get<block_trace_v2>().previous_id.str() )
         ("status", irreversible ? "irreversible" : "pending" )
         ("timestamp", to_iso8601_datetime(trace.get<block_trace_v2>().timestamp))
         ("producer", trace.get<block_trace_v2>().producer.to_string())
         ("transactions", process_transactions(e_transaction_trace_v2, 
            to_transaction_trace_variant<transaction_trace_v2>(trace.get<block_trace_v2>().transactions), data_handler_v0, data_handler_v1, return_handler, yield )) : 
      e_block_trace == e_block_trace_v3  ? fc::mutable_variant_object()
        ("id", trace.get<block_trace_v3>().id.str() )
        ("number", trace.get<block_trace_v3>().number )
        ("previous_id", trace.get<block_trace_v3>().previous_id.str() )
        ("status", irreversible ? "irreversible" : "pending" )
        ("timestamp", to_iso8601_datetime(trace.get<block_trace_v3>().timestamp))
        ("producer", trace.get<block_trace_v3>().producer.to_string())
        ("transaction_mroot", trace.get<block_trace_v3>().transaction_mroot)
        ("action_mroot", trace.get<block_trace_v3>().action_mroot)
        ("schedule_version", trace.get<block_trace_v3>().schedule_version)
        ("transactions", process_transactions(e_transaction_trace_v3, 
            to_transaction_trace_variant<transaction_trace_v3>(trace.get<block_trace_v3>().transactions_v3), data_handler_v0, data_handler_v1, return_handler, yield )) : fc::mutable_variant_object();
   }

   fc::variant response_formatter::process_block( const data_log_entry& trace, bool irreversible, const data_handler_function_v0& data_handler_v0, const data_handler_function_v1 & data_handler_v1 , const return_handler_function & return_handler,    const yield_function& yield ) {
        if (trace.contains<block_trace_v0>()) return process_block_trace(e_block_trace_v0, trace, irreversible, data_handler_v0, data_handler_v1,return_handler, yield);
        else if(trace.contains<block_trace_v1>()) return process_block_trace(e_block_trace_v1, trace, irreversible, data_handler_v0, data_handler_v1, return_handler, yield);
        else if(trace.contains<block_trace_v2>()) return process_block_trace(e_block_trace_v2, trace,  irreversible, data_handler_v0, data_handler_v1, return_handler,  yield); 
        else return process_block_trace(e_block_trace_v3, trace, irreversible, data_handler_v0, data_handler_v1, return_handler, yield);
    }
}
