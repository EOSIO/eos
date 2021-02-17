#pragma once

#include <eosio/ship_protocol.hpp>
#include <eosio/state_history/types.hpp>
#include <eosio/chain_types.hpp>

namespace eosio {
namespace state_history {

template <typename T>
auto to_uint64_t(T n) -> std::enable_if_t<std::is_same_v<T, eosio::chain::name>, decltype(n.value)> {
   return n.value;
}
template <typename T>
auto to_uint64_t(T n) -> std::enable_if_t<std::is_same_v<T, eosio::chain::name>, decltype(n.to_uint64_t())> {
   return n.to_uint64_t();
}

eosio::checksum256 convert(const eosio::chain::checksum_type& obj) {
   static_assert( sizeof(eosio::checksum256) == sizeof(eosio::chain::checksum_type), "convert may need updated" );
   std::array<uint8_t, 32> bytes;
   static_assert(bytes.size() == sizeof(obj));
   memcpy(bytes.data(), &obj, bytes.size());
   return eosio::checksum256(bytes);
}

eosio::ship_protocol::account_delta convert(const eosio::chain::account_delta& obj) {
   static_assert( sizeof(eosio::ship_protocol::account_delta) == sizeof(eosio::chain::account_delta), "convert may need updated" );
   static_assert( fc::reflector<eosio::chain::account_delta>::total_member_count == 2, "convert may need updated" );
   eosio::ship_protocol::account_delta result;
   result.account.value = to_uint64_t(obj.account);
   result.delta         = obj.delta;
   return result;
}

eosio::ship_protocol::action_receipt_v0 convert(const eosio::chain::action_receipt& obj) {
   static_assert( fc::reflector<eosio::chain::action_receipt>::total_member_count == 7, "convert may need updated" );
   eosio::ship_protocol::action_receipt_v0 result;
   result.receiver.value  = to_uint64_t(obj.receiver);
   result.act_digest      = convert(obj.act_digest);
   result.global_sequence = obj.global_sequence;
   result.recv_sequence   = obj.recv_sequence;
   for (auto& auth : obj.auth_sequence)
      result.auth_sequence.push_back({ eosio::name{ to_uint64_t(auth.first) }, auth.second });
   result.code_sequence.value = obj.code_sequence.value;
   result.abi_sequence.value  = obj.abi_sequence.value;
   return result;
}

eosio::ship_protocol::action convert(const eosio::chain::action& obj) {
   static_assert( sizeof(eosio::ship_protocol::action) == sizeof(std::tuple<eosio::name,eosio::name,std::vector<permission_level>,eosio::input_stream>), "convert may need updated" );
   static_assert( fc::reflector<eosio::chain::action>::total_member_count == 4, "convert may need updated" );
   eosio::ship_protocol::action result;
   result.account.value = to_uint64_t(obj.account);
   result.name.value    = to_uint64_t(obj.name);
   for (auto& auth : obj.authorization)
      result.authorization.push_back(
            { eosio::name{ to_uint64_t(auth.actor) }, eosio::name{ to_uint64_t(auth.permission) } });
   result.data = { obj.data.data(), obj.data.data() + obj.data.size() };
   return result;
}

eosio::ship_protocol::action_trace_v1 convert(const eosio::chain::action_trace& obj) {
   static_assert( fc::reflector<eosio::chain::action_trace>::total_member_count == 18, "convert may need updated" );
   eosio::ship_protocol::action_trace_v1 result;
   result.action_ordinal.value         = obj.action_ordinal.value;
   result.creator_action_ordinal.value = obj.creator_action_ordinal.value;
   if (obj.receipt)
      result.receipt = convert(*obj.receipt);
   result.receiver.value = to_uint64_t(obj.receiver);
   result.act            = convert(obj.act);
   result.context_free   = obj.context_free;
   result.elapsed        = obj.elapsed.count();
   result.console        = obj.console;
   for (auto& delta : obj.account_ram_deltas) result.account_ram_deltas.push_back(convert(delta));
   for (auto& delta : obj.account_disk_deltas) result.account_disk_deltas.push_back(convert(delta));
   if (obj.except)
      result.except = obj.except->to_string();
   if (obj.error_code)
      result.error_code = *obj.error_code;
   result.return_value = { obj.return_value.data(), obj.return_value.size() };
   return result;
}

eosio::ship_protocol::transaction_trace_v0 convert(const eosio::chain::transaction_trace& obj) {
   static_assert( fc::reflector<eosio::chain::transaction_trace>::total_member_count == 13, "convert may need updated" );
   eosio::ship_protocol::transaction_trace_v0 result{};
   result.id = convert(obj.id);
   if (obj.receipt) {
      result.status          = (eosio::ship_protocol::transaction_status)obj.receipt->status.value;
      result.cpu_usage_us    = obj.receipt->cpu_usage_us;
      result.net_usage_words = obj.receipt->net_usage_words.value;
   } else {
      result.status = eosio::ship_protocol::transaction_status::hard_fail;
   }
   result.elapsed   = obj.elapsed.count();
   result.net_usage = obj.net_usage;
   result.scheduled = obj.scheduled;
   for (auto& at : obj.action_traces) result.action_traces.push_back(convert(at));
   if (obj.account_ram_delta)
      result.account_ram_delta = convert(*obj.account_ram_delta);
   if (obj.except)
      result.except = obj.except->to_string();
   if (obj.error_code)
      result.error_code = *obj.error_code;
   if (obj.failed_dtrx_trace)
      result.failed_dtrx_trace.push_back({ convert(*obj.failed_dtrx_trace) });
   return result;
}

}}