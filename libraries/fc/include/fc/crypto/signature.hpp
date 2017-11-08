#pragma once
#include <fc/static_variant.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace fc { namespace crypto {
   namespace config {
      static const char*  public_key_base_prefix = "EOS";
      static const char* const public_key_prefix[] = {
         "K1",
         "H1"
      };
   };

   class signature
   {
      public:
         using storage_type = static_variant<ecc::compact_signature, sha256>;

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
