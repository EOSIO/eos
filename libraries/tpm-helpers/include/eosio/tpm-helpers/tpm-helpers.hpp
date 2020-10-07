#pragma once

#include <fc/crypto/public_key.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/crypto/sha256.hpp>

#include <boost/container/flat_set.hpp>

#include <set>

namespace eosio::tpm {

class tpm_key {
public:
   tpm_key(const std::string& tcti, const fc::crypto::public_key& pubkey, const std::vector<unsigned>& pcrs);
   ~tpm_key();

   tpm_key(const tpm_key&) = delete;
   tpm_key& operator=(const tpm_key&) = delete;

   fc::crypto::signature sign(const fc::sha256& digest);

private:
   struct impl;
   constexpr static size_t fwd_size = 256;
   fc::fwd<impl,fwd_size> my;
};

boost::container::flat_set<fc::crypto::public_key> get_all_persistent_keys(const std::string& tcti);
fc::crypto::public_key create_key(const std::string& tcti, const std::vector<unsigned>& pcrs);

class nv_data {
public:
   nv_data(const std::string& tcti, unsigned nv_index, const std::vector<unsigned>& pcrs);
   ~nv_data();

   nv_data(const nv_data&) = delete;
   nv_data& operator=(const nv_data&) = delete;

   std::optional<std::vector<char>> data();
   void set_data(const std::vector<char>& data);

private:
   struct impl;
   constexpr static size_t fwd_size = 128;
   fc::fwd<impl,fwd_size> my;
};

class swtpm {
public:
   swtpm();
   ~swtpm();

   swtpm(const swtpm&) = delete;
   swtpm& operator=(const swtpm&) = delete;

   std::string tcti() const;

private:
   struct impl;
   constexpr static size_t fwd_size = 128;
   fc::fwd<impl,fwd_size> my;
};

}