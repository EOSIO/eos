#pragma once

#include <fc/io/json.hpp>

#include <eosio/chain/asset.hpp>
#include <eosio/chain/name.hpp>

#include <eosio/trace_api_plugin/data_log.hpp>
#include <eosio/trace_api_plugin/metadata_log.hpp>

namespace eosio::trace_api_plugin {
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
         fc::raw::pack(ps, from, to, quantity, memo);
         chain::bytes result(ps.tellp());

         if( result.size() ) {
            fc::datastream<char*>  ds( result.data(), size_t(result.size()) );
            fc::raw::pack(ds, from, to, quantity, memo);
         }
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
         lhs.status == rhs.status &&
         lhs.actions == rhs.actions;
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

   std::ostream& operator<<(std::ostream &os, const block_trace_v0 &bt) {
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
   std::ostream& operator<<(std::ostream &os, const fc::static_variant<Ts...>& v ) {
      os << fc::json::to_string(v, fc::time_point::maximum());
      return os;
   }
}