#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace eos {
   struct PublicKey
   {
       struct BinaryKey
       {
          BinaryKey() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       PublicKey();
       PublicKey( const fc::ecc::public_key_data& data );
       PublicKey( const fc::ecc::public_key& pubkey );
       explicit PublicKey( const std::string& base58str );
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const PublicKey& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const PublicKey& p1, const PublicKey& p2);
       friend bool operator != ( const PublicKey& p1, const PublicKey& p2);
       friend bool operator < ( const PublicKey& p1, const PublicKey& p2);
       bool is_valid_v1( const std::string& base58str );
   };
}

namespace fc
{
    void to_variant( const eos::PublicKey& var,  fc::variant& vo );
    void from_variant( const fc::variant& var,  eos::PublicKey& vo );
}

FC_REFLECT( eos::PublicKey, (key_data) )
FC_REFLECT( eos::PublicKey::BinaryKey, (data)(check) )
