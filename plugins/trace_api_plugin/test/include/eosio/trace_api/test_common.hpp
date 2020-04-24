#pragma once

#include <fc/bitutil.hpp>
#include <fc/io/json.hpp>
#include <fc/io/raw.hpp>

#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/asset.hpp>
#include <eosio/chain/block.hpp>
#include <eosio/chain/block_state.hpp>
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

      auto get_private_key( chain::name keyname, std::string role = "owner" ) {
         auto secret = fc::sha256::hash( keyname.to_string() + role );
         return chain::private_key_type::regenerate<fc::ecc::private_key_shim>( secret );
      }

      auto get_public_key( chain::name keyname, std::string role = "owner" ) {
         return get_private_key( keyname, role ).get_public_key();
      }

      chain::bytes make_transfer_data( chain::name from, chain::name to, chain::asset quantity, std::string&& memo) {
         fc::datastream<size_t> ps;
         fc::raw::pack(ps, from, to, quantity, memo);
         chain::bytes result( ps.tellp());

         if( result.size()) {
            fc::datastream<char *> ds( result.data(), size_t( result.size()));
            fc::raw::pack(ds, from, to, quantity, memo);
         }
            return result;
      }

      auto make_block_state( chain::block_id_type previous, uint32_t height, uint32_t slot, chain::name producer,
                             std::vector<chain::packed_transaction> trxs ) {
         chain::signed_block_ptr block = std::make_shared<chain::signed_block>();
         for( auto& trx : trxs ) {
            block->transactions.emplace_back( trx );
         }
         block->producer = producer;
         block->timestamp = chain::block_timestamp_type(slot);
         // make sure previous contains correct block # so block_header::block_num() returns correct value
         if( previous == chain::block_id_type() ) {
            previous._hash[0] &= 0xffffffff00000000;
            previous._hash[0] += fc::endian_reverse_u32(height - 1);
         }
         block->previous = previous;

         auto priv_key = get_private_key( block->producer, "active" );
         auto pub_key = get_public_key( block->producer, "active" );

         auto prev = std::make_shared<chain::block_state>();
         auto header_bmroot = chain::digest_type::hash( std::make_pair( block->digest(), prev->blockroot_merkle.get_root()));
         auto sig_digest = chain::digest_type::hash( std::make_pair( header_bmroot, prev->pending_schedule.schedule_hash ));
         block->producer_signature = priv_key.sign( sig_digest );

         std::vector<chain::private_key_type> signing_keys;
         signing_keys.emplace_back( std::move( priv_key ));
         auto signer = [&]( chain::digest_type d ) {
            std::vector<chain::signature_type> result;
            result.reserve( signing_keys.size());
            for( const auto& k: signing_keys )
               result.emplace_back( k.sign( d ));
            return result;
         };
         chain::pending_block_header_state pbhs;
         pbhs.producer = block->producer;
         pbhs.timestamp = block->timestamp;
         chain::producer_authority_schedule schedule = {0, {chain::producer_authority{block->producer,
                                                                                      chain::block_signing_authority_v0{1, {{pub_key, 1}}}}}};
         pbhs.active_schedule = schedule;
         pbhs.valid_block_signing_authority = chain::block_signing_authority_v0{1, {{pub_key, 1}}};
         auto bsp = std::make_shared<chain::block_state>(
            std::move( pbhs ),
            std::move( block ),
            std::vector<chain::transaction_metadata_ptr>(),
            chain::protocol_feature_set(),
            []( chain::block_timestamp_type timestamp,
                const fc::flat_set<chain::digest_type>& cur_features,
                const std::vector<chain::digest_type>& new_features ) {},
            signer
         );
         bsp->block_num = height;

         return bsp;
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
      os << fc::json::to_string( bt, fc::time_point::max() );
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
      os << fc::json::to_string(be, fc::time_point::max());
      return os;
   }

   std::ostream& operator<<(std::ostream& os, const lib_entry_v0& le) {
      os << fc::json::to_string(le, fc::time_point::max());
      return os;
   }
}

namespace fc {
   template<typename ...Ts>
   std::ostream& operator<<(std::ostream &os, const fc::static_variant<Ts...>& v ) {
      os << fc::json::to_string(v, fc::time_point::max());
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
      os << fc::json::to_string(abi, fc::time_point::max());
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
