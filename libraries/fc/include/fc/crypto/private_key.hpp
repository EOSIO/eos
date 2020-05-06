#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/crypto/elliptic_r1.hpp>
#include <fc/crypto/public_key.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/static_variant.hpp>

namespace fc { namespace crypto {

   namespace config {
      constexpr const char* private_key_base_prefix = "PVT";
      constexpr const char* private_key_prefix[] = {
         "K1",
         "R1"
      };
   };

   class private_key
   {
      public:
         using storage_type = static_variant<ecc::private_key_shim, r1::private_key_shim>;

         private_key() = default;
         private_key( private_key&& ) = default;
         private_key( const private_key& ) = default;
         private_key& operator= (const private_key& ) = default;

         public_key     get_public_key() const;
         signature      sign( const sha256& digest, bool require_canonical = true ) const;
         sha512         generate_shared_secret( const public_key& pub ) const;

         template< typename KeyType = ecc::private_key_shim >
         static private_key generate() {
            return private_key(storage_type(KeyType::generate()));
         }

         template< typename KeyType = r1::private_key_shim >
         static private_key generate_r1() {
            return private_key(storage_type(KeyType::generate()));
         }

         template< typename KeyType = ecc::private_key_shim >
         static private_key regenerate( const typename KeyType::data_type& data ) {
            return private_key(storage_type(KeyType(data)));
         }

         // serialize to/from string
         explicit private_key(const string& base58str);
         std::string to_string(const fc::yield_function_t& yield = fc::yield_function_t()) const;

      private:
         storage_type _storage;

         private_key( storage_type&& other_storage )
         :_storage(forward<storage_type>(other_storage))
         {}

         friend bool operator == ( const private_key& p1, const private_key& p2);
         friend bool operator != ( const private_key& p1, const private_key& p2);
         friend bool operator < ( const private_key& p1, const private_key& p2);
         friend struct reflector<private_key>;
   }; // private_key

} }  // fc::crypto

namespace fc {
   void to_variant(const crypto::private_key& var, variant& vo, const fc::yield_function_t& yield = fc::yield_function_t());

   void from_variant(const variant& var, crypto::private_key& vo);
} // namespace fc

FC_REFLECT(fc::crypto::private_key, (_storage) )
