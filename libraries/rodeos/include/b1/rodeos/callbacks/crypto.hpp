#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>

namespace b1::rodeos {

template <typename Derived>
struct crypto_callbacks {
   template <typename T>
   T unimplemented(const char* name) {
      throw std::runtime_error("wasm called " + std::string(name) + ", which is unimplemented");
   }

   void assert_recover_key(int, int, int, int, int) { return unimplemented<void>("assert_recover_key"); }
   int  recover_key(int, int, int, int, int) { return unimplemented<int>("recover_key"); }
   void assert_sha256(int, int, int) { return unimplemented<void>("assert_sha256"); }
   void assert_sha1(int, int, int) { return unimplemented<void>("assert_sha1"); }
   void assert_sha512(int, int, int) { return unimplemented<void>("assert_sha512"); }
   void assert_ripemd160(int, int, int) { return unimplemented<void>("assert_ripemd160"); }

   void sha1(eosio::vm::span<const char> data, legacy_ptr<fc::sha1> hash_val) {
      *hash_val = fc::sha1::hash(data.data(), data.size());
   }

   void sha256(eosio::vm::span<const char> data, legacy_ptr<fc::sha256> hash_val) {
      *hash_val = fc::sha256::hash(data.data(), data.size());
   }

   void sha512(eosio::vm::span<const char> data, legacy_ptr<fc::sha512> hash_val) {
      *hash_val = fc::sha512::hash(data.data(), data.size());
   }

   void ripemd160(eosio::vm::span<const char> data, legacy_ptr<fc::ripemd160> hash_val) {
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
