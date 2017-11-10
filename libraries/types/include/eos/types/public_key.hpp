/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <fc/crypto/elliptic.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

namespace eosio { namespace types {
   struct public_key
   {
       struct binary_key
       {
          binary_key() {}
          uint32_t                 check = 0;
          fc::ecc::public_key_data data;
       };
       fc::ecc::public_key_data key_data;
       public_key();
       public_key(const fc::ecc::public_key_data& data);
       public_key(const fc::ecc::public_key& pubkey);
       explicit public_key(const std::string& base58str);
       operator fc::ecc::public_key_data() const;
       operator fc::ecc::public_key() const;
       explicit operator std::string() const;
       friend bool operator == ( const public_key& p1, const fc::ecc::public_key& p2);
       friend bool operator == ( const public_key& p1, const public_key& p2);
       friend bool operator != ( const public_key& p1, const public_key& p2);
       friend bool operator < ( const public_key& p1, const public_key& p2);
       friend std::ostream& operator<< (std::ostream& s, const public_key& k);
       bool is_valid_v1(const std::string& base58str);
   };
}} // namespace eosio::types

namespace fc
{
    void to_variant(const eosio::types::public_key& var,  fc::variant& vo);
    void from_variant(const fc::variant& var,  eosio::types::public_key& vo);
}

FC_REFLECT(eosio::types::public_key, (key_data))
FC_REFLECT(eosio::types::public_key::binary_key, (data)(check))
