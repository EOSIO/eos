#pragma once

#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>

#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/name.hpp>

#include <eosio/trace_api/data_log.hpp>
#include <eosio/trace_api/metadata_log.hpp>

namespace eosio::trace_api {
   /**
    * Utilities that make writing tests easier
    */

   namespace test_common {
      fc::sha256 operator"" _h(const char* input, std::size_t) {
         return fc::sha256(input);
      }

      chain::name operator"" _n(const char* input, std::size_t) {
         return chain::name(input);
      }

      chain::asset operator"" _t(const char* input, std::size_t) {
         return chain::asset::from_string(input);
      }

      chain::bytes make_transfer_data( chain::name from, chain::name to, chain::asset quantity, std::string&& memo) {
         fc::datastream<size_t> ps;
         fc::raw::pack( ps, from, to, quantity, memo );
         chain::bytes result( ps.tellp() );

         if( result.size() ) {
            fc::datastream<char*> ds( result.data(), size_t( result.size() ) );
            fc::raw::pack( ds, from, to, quantity, memo );
         }
         return result;
      }

      void to_kv_helper(const fc::variant& v, std::function<void(const std::string&, const std::string&)>&& append){
         if (v.is_object() ) {
            const auto& obj = v.get_object();
            static const std::string sep = ".";

            for (const auto& entry: obj) {
               to_kv_helper( entry.value(), [&append, &entry](const std::string& path, const std::string& value){
                  append(sep + entry.key() + path, value);
               });
            }
         } else if (v.is_array()) {
            const auto& arr = v.get_array();
            for (size_t idx = 0; idx < arr.size(); idx++) {
               const auto& entry = arr.at(idx);
               to_kv_helper( entry, [&append, idx](const std::string& path, const std::string& value){
                  append(std::string("[") + std::to_string(idx) + std::string("]") + path, value);
               });
            }
         } else if (!v.is_null()) {
            append("", v.as_string());
         }
      }

      auto to_kv(const fc::variant& v) {
         std::map<std::string, std::string> result;
         to_kv_helper(v, [&result](const std::string& k, const std::string& v){
            result.emplace(k, v);
         });
         return result;
      }
   }

   // TODO: promote these to the main files?
   // I prefer not to have these operators but they are convenient for BOOST TEST integration
   //

   bool operator==(const authorization_trace_v0& lhs, const authorization_trace_v0& rhs) {
      return
         lhs.account == rhs.account &&
         lhs.permission == rhs.permission;
   }

   bool operator==(const action_trace_v0& lhs, const action_trace_v0& rhs) {
      return
         lhs.global_sequence == rhs.global_sequence &&
         lhs.receiver == rhs.receiver &&
         lhs.account == rhs.account &&
         lhs.action == rhs.action &&
         lhs.authorization == rhs.authorization &&
         lhs.data == rhs.data;
   }

   bool operator==(const transaction_trace_v0& lhs,  const transaction_trace_v0& rhs) {
      return
         lhs.id == rhs.id &&
         lhs.actions == rhs.actions;
   }

   bool operator==(const transaction_trace_v2& lhs,  const transaction_trace_v2& rhs) {
      return
         lhs.id == rhs.id &&
         lhs.actions == rhs.actions &&
         lhs.status == rhs.status &&
         lhs.cpu_usage_us == rhs.cpu_usage_us &&
         lhs.net_usage_words == rhs.net_usage_words &&
         lhs.signatures == rhs.signatures &&
         lhs.trx_header.expiration == rhs.trx_header.expiration &&
         lhs.trx_header.ref_block_num == rhs.trx_header.ref_block_num &&
         lhs.trx_header.ref_block_prefix == rhs.trx_header.ref_block_prefix &&
         lhs.trx_header.max_net_usage_words == rhs.trx_header.max_net_usage_words &&
         lhs.trx_header.max_cpu_usage_ms == rhs.trx_header.max_cpu_usage_ms &&
         lhs.trx_header.delay_sec == rhs.trx_header.delay_sec ;
   }

   bool operator==(const block_trace_v0 &lhs, const block_trace_v0 &rhs) {
      return
         lhs.id == rhs.id &&
         lhs.number == rhs.number &&
         lhs.previous_id == rhs.previous_id &&
         lhs.timestamp == rhs.timestamp &&
         lhs.producer == rhs.producer &&
         lhs.transactions == rhs.transactions;
   }

   bool operator==(const block_trace_v2 &lhs, const block_trace_v2 &rhs) {
      return
         lhs.id == rhs.id &&
         lhs.number == rhs.number &&
         lhs.previous_id == rhs.previous_id &&
         lhs.timestamp == rhs.timestamp &&
         lhs.producer == rhs.producer &&
         lhs.transaction_mroot == rhs.transaction_mroot &&
         lhs.action_mroot == rhs.action_mroot &&
         lhs.schedule_version == rhs.schedule_version &&
         lhs.transactions == rhs.transactions;
   }

   std::ostream& operator<<(std::ostream &os, const block_trace_v0 &bt) {
      os << fc::json::to_string( bt, fc::time_point::maximum() );
      return os;
   }

   std::ostream& operator<<(std::ostream &os, const block_trace_v2 &bt) {
      os << fc::json::to_string( bt, fc::time_point::maximum() );
      return os;
   }

   bool operator==(const block_entry_v0& lhs, const block_entry_v0& rhs) {
      return
         lhs.id == rhs.id &&
         lhs.number == rhs.number &&
         lhs.offset == rhs.offset;
   }

   bool operator!=(const block_entry_v0& lhs, const block_entry_v0& rhs) {
      return !(lhs == rhs);
   }

   bool operator==(const lib_entry_v0& lhs, const lib_entry_v0& rhs) {
      return
         lhs.lib == rhs.lib;
   }

   bool operator!=(const lib_entry_v0& lhs, const lib_entry_v0& rhs) {
      return !(lhs == rhs);
   }

   std::ostream& operator<<(std::ostream& os, const block_entry_v0& be) {
      os << fc::json::to_string(be, fc::time_point::maximum());
      return os;
   }

   std::ostream& operator<<(std::ostream& os, const lib_entry_v0& le) {
      os << fc::json::to_string(le, fc::time_point::maximum());
      return os;
   }
}

namespace fc {
   template<typename ...Ts>
   std::ostream& operator<<(std::ostream &os, const std::variant<Ts...>& v ) {
      os << fc::json::to_string(v, fc::time_point::maximum());
      return os;
   }

   std::ostream& operator<<(std::ostream &os, const fc::microseconds& t ) {
      os << t.count();
      return os;
   }

}

namespace eosio::chain {
   bool operator==(const abi_def& lhs, const abi_def& rhs) {
      return fc::raw::pack(lhs) == fc::raw::pack(rhs);
   }

   bool operator!=(const abi_def& lhs, const abi_def& rhs) {
      return !(lhs == rhs);
   }

   std::ostream& operator<<(std::ostream& os, const abi_def& abi) {
      os << fc::json::to_string(abi, fc::time_point::maximum());
      return os;
   }
}

namespace std {
   /*
    * operator for printing to_kv entries
    */
   ostream& operator<<(ostream& os, const pair<string, string>& entry) {
      os << entry.first + "=" + entry.second;
      return os;
   }
}
