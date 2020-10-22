#pragma once

#include <b1/rodeos/callbacks/vm_types.hpp>

namespace b1::rodeos {

template <typename Derived>
struct unimplemented_query_callbacks {
   template <typename T>
   T unimplemented(const char* name) {
      throw std::runtime_error("wasm called " + std::string(name) + ", which is unimplemented");
   }

   // crypto_api
   // note: if these are ever implemented in queries, they'll need protection against exceeding the
   //       query timeout to prevent DOS attack. Filters don't need this protection.
   void assert_recover_key(int, int, int, int, int) { return unimplemented<void>("assert_recover_key"); }
   int  recover_key(int, int, int, int, int) { return unimplemented<int>("recover_key"); }
   void assert_sha256(int, int, int) { return unimplemented<void>("assert_sha256"); }
   void assert_sha1(int, int, int) { return unimplemented<void>("assert_sha1"); }
   void assert_sha512(int, int, int) { return unimplemented<void>("assert_sha512"); }
   void assert_ripemd160(int, int, int) { return unimplemented<void>("assert_ripemd160"); }
   void sha1(int, int, int) { return unimplemented<void>("sha1"); }
   void sha256(int, int, int) { return unimplemented<void>("sha256"); }
   void sha512(int, int, int) { return unimplemented<void>("sha512"); }
   void ripemd160(int, int, int) { return unimplemented<void>("ripemd160"); }

   template <typename Rft>
   static void register_callbacks() {
      // crypto_api
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
};   // unimplemented_query_callbacks

} // namespace b1::rodeos
