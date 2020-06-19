#pragma once

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/crypto/sha256.hpp>

#include <set>

namespace eosio::secure_enclave {

class secure_enclave_key {
   public:
      const fc::crypto::public_key& public_key() const;
      fc::crypto::signature sign(const fc::sha256& digest) const;

      secure_enclave_key(const secure_enclave_key&);
      secure_enclave_key(secure_enclave_key&&);

      bool operator<(const secure_enclave_key& r) const {
         return public_key() < r.public_key();
      }

      //only for use by get_all_keys()/create_key()
      secure_enclave_key(void*);

      ~secure_enclave_key() = default;
   private:
      struct impl;
      constexpr static size_t fwd_size = 128;
      fc::fwd<impl,fwd_size> my;

      friend void delete_key(secure_enclave_key&& key);
};

bool hardware_supports_secure_enclave();
bool application_signed();

std::set<secure_enclave_key> get_all_keys();
secure_enclave_key create_key();
void delete_key(secure_enclave_key&& key);

}