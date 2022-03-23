#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/contract_types.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/controller.hpp>
#include <fc/io/json.hpp>


namespace eosio { namespace chain {

action_trace::action_trace(
   const transaction_trace& trace, const action& act, account_name receiver, bool context_free,
   uint32_t action_ordinal, uint32_t creator_action_ordinal, uint32_t closest_unnotified_ancestor_action_ordinal
)
:action_ordinal( action_ordinal )
,creator_action_ordinal( creator_action_ordinal )
,closest_unnotified_ancestor_action_ordinal( closest_unnotified_ancestor_action_ordinal )
,receiver( receiver )
,act( act )
,context_free( context_free )
,trx_id( trace.id )
,block_num( trace.block_num )
,block_time( trace.block_time )
,producer_block_id( trace.producer_block_id )
{}

action_trace::action_trace(
   const transaction_trace& trace, action&& act, account_name receiver, bool context_free,
   uint32_t action_ordinal, uint32_t creator_action_ordinal, uint32_t closest_unnotified_ancestor_action_ordinal
)
:action_ordinal( action_ordinal )
,creator_action_ordinal( creator_action_ordinal )
,closest_unnotified_ancestor_action_ordinal( closest_unnotified_ancestor_action_ordinal )
,receiver( receiver )
,act( std::move(act) )
,context_free( context_free )
,trx_id( trace.id )
,block_num( trace.block_num )
,block_time( trace.block_time )
,producer_block_id( trace.producer_block_id )
{}

namespace trace {
   template<typename Container>
   void to_trimmed_trace_container_string(string& result, const char* name, const Container& vec, const controller& chain);

   void to_trimmed_string(string& result, const action_trace& at, const controller& chain)  {

      result += "\"action_ordinal\":" + std::to_string(at.action_ordinal) + ","
              + "\"creator_action_ordinal\":" + std::to_string(at.creator_action_ordinal) + ","
              + "\"closest_unnotified_ancestor_action_ordinal\":" + std::to_string(at.closest_unnotified_ancestor_action_ordinal) + ",";
      if (at.receipt.has_value()) {
         result += "\"receipt\":{";
         result += "\"receiver\":\"" + at.receipt->receiver.to_string() + "\"" + ","
                 + "\"act_digest\":\"" + at.receipt->act_digest.str() + "\"" + ","
                 + "\"global_sequence\":" + std::to_string(at.receipt->global_sequence) + ","
                 + "\"recv_sequence\":" + std::to_string(at.receipt->recv_sequence) + ",";
         result += "\"auth_sequence\":[";
         auto itr = at.receipt->auth_sequence.find(at.receipt->receiver);
         if (itr != at.receipt->auth_sequence.end()){
            result += "[\"" + itr->first.to_string() + "\"," + std::to_string(itr->second) + "]";
         } else {
            result += "[]";
         }
         result += "],";
         result += "\"code_sequence\":" + std::to_string(at.receipt->code_sequence) + ","
                 + "\"abi_sequence\":" + std::to_string(at.receipt->abi_sequence);
         result += "}";
      } else {
         result += "null";
      }
      result += ",";
      result += "\"receiver\":\"" + at.receiver.to_string() + "\"" + ",";

      // action trace
      auto a = at.act;
      result += "\"act\":{"; //act begin
      result += "\"account\":\"" + a.account.to_string() + "\","
                + "\"name\":\"" + a.name.to_string() + "\",";
      to_trimmed_trace_container_string(result, "authorization", a.authorization, chain);
      result += ",";

      if( a.account == config::system_account_name && a.name == "setcode"_n ) {
         auto setcode_act = a.data_as<eosio::chain::setcode>();
         if( setcode_act.code.size() > 0 ) {
            result += "\"code_hash\":";
            fc::sha256 code_hash = fc::sha256::hash(setcode_act.code.data(), (uint32_t) setcode_act.code.size());
            result += "\"" + code_hash.str() + "\",";
         }
      }

      result += "\"data\":";
      abi_serializer::yield_function_t yield = abi_serializer::create_yield_function(chain.get_abi_serializer_max_time());
      auto abi = chain.get_abi_serializer(a.account, yield);
      fc::variant output;
      if (abi) {
         auto type = abi->get_action_type(a.name);
         if (!type.empty()) {
            try {
               output = abi->binary_to_log_variant(type, a.data, yield);
               result += fc::json::to_string(output, fc::time_point::maximum());
               result += ",";
               result += "\"hex_data\":{";
            } catch (...) {
               // any failure to serialize data, then leave as not serialized
               result += "{";
            }
         } else {
            result += "{";
         }
      } else {
         result += "{";
      }

      result += "\"size\":" + std::to_string(a.data.size()) + ",";
      if( a.data.size() > impl::hex_log_max_size ) {
         result += "\"trimmed_hex\":\"" + fc::to_hex(std::vector(a.data.begin(), a.data.begin() + impl::hex_log_max_size)) + "\"";
      } else {
         result += "\"hex\":\"" + fc::to_hex(a.data) + "\"";
      }
      result += "}";
      result += "}"; //act end
      result += ",";
      // action trace end

      result = result + "\"context_free\":" + (at.context_free ? "true" : "false") + ",";
      result += "\"elapsed\":" + std::to_string(at.elapsed.count()) + ","
              + "\"console\":\"" + at.console + "\","
              + "\"trx_id\":\"" + at.trx_id.str() + "\","
              + "\"block_num\":" + std::to_string(at.block_num) + ","
              + "\"block_time\":\"" + (std::string)at.block_time.to_time_point() + "\","
              + "\"producer_block_id\":";
      if (at.producer_block_id.has_value()) {
         result += "\"" + at.producer_block_id->str() + "\"";
      } else {
         result += "null";
      }
      result += ",";

      // account_ram_deltas
      to_trimmed_trace_container_string(result, "account_ram_deltas", at.account_ram_deltas, chain);
      result += ",";
      to_trimmed_trace_container_string(result, "account_disk_deltas", at.account_disk_deltas, chain);
      result += ",";

      result += "\"except\":";
      if (at.except.has_value()) {
         ;//TODO...
      } else {
         result += "null";
      }
      result += ",";
      result += "\"error_code\":";
      if (at.error_code.has_value()) {
         ;//TODO...
      } else {
         result += "null";
      }
      result += ",";

      result += "\"return_value\":\"" + std::string(at.return_value.begin(), at.return_value.end()) + "\"";
   }
   
   void to_trimmed_string(string& result, const account_delta& ad, const controller& chain) {
      result += "\"account\":\"" + ad.account.to_string() + "\","
                + "\"delta\":\"" + std::to_string(ad.delta) + "\"";
   }

   void to_trimmed_string(string& result, const permission_level& perm, const controller& chain) {
      result += "\"actor\":\"" + perm.actor.to_string() + "\","
                + "\"permission\":\"" + perm.permission.to_string() + "\"";
   }

   template<typename Container>
   void to_trimmed_trace_container_string(string& result, const char* name, const Container& vec, const controller& chain) {
      result = result +  "\"" + name + "\":[";
      for (const auto& v : vec) {
         result += "{";
         to_trimmed_string(result, v, chain);
         result += "},";
      }
      if (!vec.empty())
         result.pop_back(); //remove the last `,`
      result += "]";
   }

   void to_trimmed_trace_string(string& result, const transaction_trace& t, const controller& chain) {
      result = "{";
      result += "\"id\":\"" + (std::string)t.id +  "\","
                + "\"block_num\":" + std::to_string(t.block_num) + ","
                + "\"block_time\":\"" + (std::string)t.block_time.to_time_point() + "\"" + ","
                + "\"producer_block_id\":" + ( t.producer_block_id.has_value() ? (std::string)t.producer_block_id.value() : "null" )  + ",";
      if (t.receipt.has_value()) {
         result += "\"receipt\":{";
         result += "\"status\":\"" + (std::string)t.receipt->status + "\"" + ","
                 + "\"cpu_usage_us\":" + std::to_string(t.receipt->cpu_usage_us) + ","
                 + "\"net_usage_words\":" + std::to_string(t.receipt->net_usage_words);
         result += "}";
      } else{
         result += "null";
      }
      result += ",";
      result += "\"elapsed\":" + std::to_string(t.elapsed.count()) + ","
              + "\"net_usage\":" + std::to_string(t.net_usage) + ","
              + "\"scheduled\":" + (t.scheduled ? "true" : "false") + ",";

      // action_trace
      to_trimmed_trace_container_string(result, "action_traces", t.action_traces, chain);
      result += ",";

      result += "\"account_ram_delta\":";
      if (t.account_ram_delta.has_value()) {
         result += "null"; // TODO...
      } else {
         result += "null";
      }
      result += ",";

      result += "\"failed_dtrx_trace\":";
      result += "null"; // TODO...
      result += ",";

      result += "\"except\":";
      if (t.except.has_value()) {
         ;//TODO...
      } else {
         result += "null";
      }
      result += ",";
      result += "\"error_code\":";
      if (t.error_code.has_value()) {
         ;//TODO...
      } else {
         result += "null";
      }
      result += ",";
      result += "\"except_ptr\":";
      result += "null"; //TODO...

      result += "}";
   }
}

} } // eosio::chain
