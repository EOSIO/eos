#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <eosio/chain/config.hpp>
#include <fc/crypto/public_key.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/io/datastream.hpp>

namespace b1::rodeos {

template <typename Derived>
struct crypto_callbacks {
   template <typename T>
   T unimplemented(const char* name) {
      throw std::runtime_error("wasm called " + std::string(name) + ", which is unimplemented");
   }

   void assert_recover_key( legacy_ptr<const fc::sha256> digest,
                            legacy_span<const char> sig,
                            legacy_span<const char> pub ) const {
      fc::crypto::signature s;
      fc::crypto::public_key p;
      fc::datastream<const char*> ds( sig.data(), sig.size() );
      fc::datastream<const char*> pubds ( pub.data(), pub.size() );

      fc::raw::unpack( ds, s );
      fc::raw::unpack( pubds, p );

      // TODO: enforce supported key types
      // EOS_ASSERT(static_cast<unsigned>(s.which()) < db.get<protocol_state_object>().num_supported_key_types,
      //            eosio::chain::unactivated_signature_type,
      //            "Unactivated signature type used during assert_recover_key");
      // EOS_ASSERT(static_cast<unsigned>(p.which()) < db.get<protocol_state_object>().num_supported_key_types,
      //            eosio::chain::unactivated_key_type,
      //            "Unactivated key type used during assert_recover_key");

      // TODO: enforce subjective signature length limit maximum-variable-signature-length in queries. Filters don't need this protection.
      //   EOS_ASSERT(s.variable_size() <= configured_subjective_signature_length_limit(),
      //              eosio::chain::sig_variable_size_limit_exception, "signature variable length component size greater than subjective maximum");

      auto check = fc::crypto::public_key( s, *digest, false );
      EOS_ASSERT( check == p, eosio::chain::crypto_api_exception, "Error expected key different than recovered key" );
   }

   int32_t recover_key( legacy_ptr<const fc::sha256> digest,
                                   legacy_span<const char> sig,
                                   legacy_span<char> pub ) const {
      fc::crypto::signature s;
      fc::datastream<const char*> ds( sig.data(), sig.size() );
      fc::raw::unpack(ds, s);

      // TODO: enforce supported key types
      // EOS_ASSERT(static_cast<unsigned>(s.which()) < db.get<protocol_state_object>().num_supported_key_types,
      //            eosio::chain::unactivated_signature_type,
      //            "Unactivated signature type used during assert_recover_key");

      // TODO: enforce subjective signature length limit maximum-variable-signature-length in queries. Filters don't need this protection.
      //   EOS_ASSERT(s.variable_size() <= configured_subjective_signature_length_limit(),
      //              eosio::chain::sig_variable_size_limit_exception, "signature variable length component size greater than subjective maximum");

      auto recovered = fc::crypto::public_key(s, *digest, false);

      // the key types newer than the first 2 may be varible in length
      if (static_cast<unsigned>(s.which()) >= eosio::chain::config::genesis_num_supported_key_types ) {
         EOS_ASSERT(pub.size() >= 33, eosio::chain::wasm_execution_error,
                    "destination buffer must at least be able to hold an ECC public key");
         auto packed_pubkey = fc::raw::pack(recovered);
         auto copy_size = std::min<size_t>(pub.size(), packed_pubkey.size());
         std::memcpy(pub.data(), packed_pubkey.data(), copy_size);
         return packed_pubkey.size();
      } else {
         // legacy behavior, key types 0 and 1 always pack to 33 bytes.
         // this will do one less copy for those keys while maintaining the rules of
         //    [0..33) dest sizes: assert (asserts in fc::raw::pack)
         //    [33..inf) dest sizes: return packed size (always 33)
         fc::datastream<char*> out_ds( pub.data(), pub.size() );
         fc::raw::pack(out_ds, recovered);
         return out_ds.tellp();
      }
   }

   void assert_sha256(legacy_span<const char> data, legacy_ptr<const fc::sha256> hash_val) { 
      auto result = fc::sha256::hash( data.data(), data.size() );
      EOS_ASSERT( result == *hash_val, crypto_api_exception, "hash mismatch" );
   }

   void assert_sha1(legacy_span<const char> data, legacy_ptr<const fc::sha1> hash_val) { 
      auto result = fc::sha1::hash( data.data(), data.size() );
      EOS_ASSERT( result == *hash_val, crypto_api_exception, "hash mismatch" );
   }

   void assert_sha512(legacy_span<const char> data, legacy_ptr<const fc::sha512> hash_val) { 
      auto result = fc::sha512::hash( data.data(), data.size() );
      EOS_ASSERT( result == *hash_val, crypto_api_exception, "hash mismatch" );
   }

   void assert_ripemd160(legacy_span<const char> data, legacy_ptr<const fc::ripemd160> hash_val) { 
      auto result = fc::ripemd160::hash( data.data(), data.size() );
      EOS_ASSERT( result == *hash_val, crypto_api_exception, "hash mismatch" );
   }

   void sha1(eosio::vm::span<const char> data, legacy_ptr<fc::sha1> hash_val) {
      // TODO: needs checktime protection in queries. Filters don't need this protection.
      *hash_val = fc::sha1::hash(data.data(), data.size());
   }

   void sha256(eosio::vm::span<const char> data, legacy_ptr<fc::sha256> hash_val) {
      // TODO: needs checktime protection in queries. Filters don't need this protection.
      *hash_val = fc::sha256::hash(data.data(), data.size());
   }

   void sha512(eosio::vm::span<const char> data, legacy_ptr<fc::sha512> hash_val) {
      // TODO: needs checktime protection in queries. Filters don't need this protection.
      *hash_val = fc::sha512::hash(data.data(), data.size());
   }

   void ripemd160(eosio::vm::span<const char> data, legacy_ptr<fc::ripemd160> hash_val) {
      // TODO: needs checktime protection in queries. Filters don't need this protection.
      *hash_val = fc::ripemd160::hash(data.data(), data.size());
   }

   template <typename Rft>
   static void register_callbacks() {
      RODEOS_REGISTER_CALLBACK(Rft, Derived, assert_recover_key);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, recover_key);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, assert_sha256);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, assert_sha1);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, assert_sha512);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, assert_ripemd160);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, sha1);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, sha256);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, sha512);
      RODEOS_REGISTER_CALLBACK(Rft, Derived, ripemd160);
   } // register_callbacks()
};   // crypto_callbacks

} // namespace b1::rodeos
