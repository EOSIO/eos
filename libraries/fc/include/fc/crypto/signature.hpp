#pragma once
#include <fc/static_variant.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/elliptic_r1.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace fc { namespace crypto {
   namespace config {
      constexpr const char* signature_base_prefix = "EOS";
      constexpr const char* signature_prefix[] = {
         "K1",
         "R1"
      };
   };

   class signature
   {
      public:
         using storage_type = static_variant<ecc::signature_shim, r1::signature_shim>;

         signature() = default;
         signature( signature&& ) = default;
         signature( const signature& ) = default;
         signature& operator= (const signature& ) = default;

         // serialize to/from string
         explicit signature(const string& base58str);
         explicit operator string() const;

      private:
         storage_type _storage;

         signature( storage_type&& other_storage )
         :_storage(std::forward<storage_type>(other_storage))
         {}

         friend bool operator == ( const signature& p1, const signature& p2);
         friend bool operator != ( const signature& p1, const signature& p2);
         friend bool operator < ( const signature& p1, const signature& p2);
         friend std::size_t hash_value(const signature& b); //not cryptographic; for containers
         friend struct reflector<signature>;
         friend class private_key;
         friend class public_key;
   }; // public_key

} }  // fc::crypto

namespace fc {
   void to_variant(const crypto::signature& var,  variant& vo);

   void from_variant(const variant& var, crypto::signature& vo);
} // namespace fc

FC_REFLECT(fc::crypto::signature, (_storage) )
