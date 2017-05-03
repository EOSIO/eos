#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace eos { namespace types {
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
       PublicKey(const fc::ecc::public_key_data& data);
       PublicKey(const fc::ecc::public_key& pubkey);
       explicit PublicKey(const std::string& base58str);
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const PublicKey& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const PublicKey& p1, const PublicKey& p2);
       friend bool operator != ( const PublicKey& p1, const PublicKey& p2);
       friend bool operator < ( const PublicKey& p1, const PublicKey& p2);
       friend std::ostream& operator<< (std::ostream& s, const PublicKey& k);
       bool is_valid_v1(const std::string& base58str);
   };
}} // namespace eos::types

namespace fc
{
    void to_variant(const eos::types::PublicKey& var,  fc::variant& vo);
    void from_variant(const fc::variant& var,  eos::types::PublicKey& vo);
}

FC_REFLECT(eos::types::PublicKey, (key_data))
FC_REFLECT(eos::types::PublicKey::BinaryKey, (data)(check))
