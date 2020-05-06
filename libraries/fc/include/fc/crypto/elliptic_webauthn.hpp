#pragma once
#include <fc/crypto/bigint.hpp>
#include <fc/crypto/common.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/elliptic_r1.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/fwd.hpp>
#include <fc/array.hpp>
#include <fc/io/raw_fwd.hpp>

namespace fc { namespace crypto { namespace webauthn {

class signature;

class public_key {
   public:
      using public_key_data_type = fc::array<char, 33>;

      //Used for base58 de/serialization
      using data_type = public_key;
      public_key serialize() const { return *this; }

      enum class user_presence_t : uint8_t {
         USER_PRESENCE_NONE,
         USER_PRESENCE_PRESENT,
         USER_PRESENCE_VERIFIED
      };

      bool valid() const { return rpid.size(); }

      public_key() {}
      public_key(const public_key_data_type& p, const user_presence_t& t, const std::string& s) : 
         public_key_data(p), user_verification_type(t), rpid(s) {
            post_init();
         }
      public_key(const signature& c, const fc::sha256& digest, bool check_canonical = true);

      bool operator==(const public_key& o) const {
         return        public_key_data == o.public_key_data        &&
                user_verification_type == o.user_verification_type &&
                                  rpid == o.rpid;
      }
      bool operator<(const public_key& o) const {
         return std::tie(public_key_data, user_verification_type, rpid) < std::tie(o.public_key_data, o.user_verification_type, o.rpid);
      }

      template<typename Stream>
      friend Stream& operator<<(Stream& ds, const public_key& k) {
         fc::raw::pack(ds, k.public_key_data);
         fc::raw::pack(ds, static_cast<uint8_t>(k.user_verification_type));
         fc::raw::pack(ds, k.rpid);
         return ds;
      }

      template<typename Stream>
      friend Stream& operator>>(Stream& ds, public_key& k) {
         fc::raw::unpack(ds, k.public_key_data);
         uint8_t t;
         fc::raw::unpack(ds, t);
         k.user_verification_type = static_cast<user_presence_t>(t);
         fc::raw::unpack(ds, k.rpid);
         k.post_init();
         return ds;
      }

   private:
      public_key_data_type public_key_data;
      user_presence_t user_verification_type = user_presence_t::USER_PRESENCE_NONE;
      std::string rpid;

      void post_init();
};

class signature {
   public:
      //used for base58 de/serialization
      using data_type = signature;
      signature serialize()const { return *this; }

      signature() {}
      signature(const fc::crypto::r1::compact_signature& s, const std::vector<uint8_t>& a, const std::string& j) :
         compact_signature(s), auth_data(a), client_json(j) {}

      public_key recover(const sha256& digest, bool check_canonical) const {
         return public_key(*this, digest, check_canonical);
      }

      size_t variable_size() const {
         return auth_data.size() + client_json.size();
      }

      bool operator==(const signature& o) const {
         return compact_signature == o.compact_signature &&
                        auth_data == o.auth_data         &&
                      client_json == o.client_json;
      }

      bool operator<(const signature& o) const {
         return std::tie(compact_signature, auth_data, client_json) < std::tie(o.compact_signature, o.auth_data, o.client_json);
      }

      //for container usage
      size_t get_hash() const {
         return *(size_t*)&compact_signature.data[32-sizeof(size_t)] + *(size_t*)&compact_signature.data[64-sizeof(size_t)];
      }

      friend struct fc::reflector<signature>;
      friend class public_key;
   private:
      fc::crypto::r1::compact_signature compact_signature;
      std::vector<uint8_t>              auth_data;
      std::string                       client_json;
};

}

template<>
struct eq_comparator<webauthn::signature> {
   static bool apply(const webauthn::signature& a, const webauthn::signature& b) {
      return a == b;
   }
};

template<>
struct less_comparator<webauthn::signature> {
   static bool apply(const webauthn::signature& a, const webauthn::signature& b) {
      return a < b;
   }
};

template<>
struct eq_comparator<webauthn::public_key> {
   static bool apply(const webauthn::public_key& a, const webauthn::public_key& b) {
      return a == b;
   }
};

template<>
struct less_comparator<webauthn::public_key> {
   static bool apply(const webauthn::public_key& a, const webauthn::public_key& b) {
      return a < b;
   }
};

}}
#include <fc/reflect/reflect.hpp>

FC_REFLECT(fc::crypto::webauthn::signature, (compact_signature)(auth_data)(client_json))