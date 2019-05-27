/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <appbase/application.hpp>
#include <microfido/microfido.hpp>

#include <eosio/wallet_plugin/webauthn_wallet.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/crypto/openssl.hpp>
#include <fc/crypto/elliptic_r1.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

namespace eosio { namespace wallet {

using namespace fc::crypto::webauthn;

namespace detail {

struct webauthn_wallet_impl {
   webauthn_wallet_impl(const webauthn_wallet::public_key_credential_id_map& keys) : _keys(keys) {}

   bool is_locked() const {
      return !fido;
   }

   void unlock() {
      fido = std::make_unique<eosio::fido_device>();
   }

   void lock() {
      fido.reset();
   }

   fc::optional<signature_type> try_sign_digest(const digest_type d, const public_key_type public_key) {
      auto it = _keys.find(public_key);
      if(it == _keys.end())
         return fc::optional<signature_type>{};

      std::string client_data = "{\"origin\":\"https://keosd.invalid\",\"type\":\"webauthn.get\",\"challenge\":\"" + fc::base64url_encode(d.data(), d.data_size()) + "\"}";
      fc::sha256 client_data_hash = fc::sha256::hash(client_data);

      fido_assertion_result assertion = fido->do_assertion(client_data_hash, it->second);

      fc::ecdsa_sig sig = ECDSA_SIG_new();
      const uint8_t* der_begin = assertion.der_signature.data();
      d2i_ECDSA_SIG(&sig.obj, &der_begin, assertion.der_signature.size());

      //need to get the public_key_data for this webauthn key which we know lives at index 1 to 34
      char pub_key_shim_data[sizeof(fc::crypto::r1::public_key_data) + 1];
      fc::datastream<char*> eds(pub_key_shim_data, sizeof(pub_key_shim_data));
      try {
         fc::raw::pack(eds, it->first);
      } catch(fc::out_of_range_exception& e) {}
      FC_ASSERT(eds.tellp() == sizeof(pub_key_shim_data), "didn't serialize pubkeydata correctly");
      fc::crypto::r1::public_key_data* kd = (fc::crypto::r1::public_key_data*)(pub_key_shim_data+1);

      //the signature is sha256(auth_data || client_data_hash), so we need to compute that now to do key recovery computation
      fc::sha256::encoder e;
      e.write((char*)assertion.auth_data.data(), assertion.auth_data.size());
      e.write(client_data_hash.data(), client_data_hash.data_size());

      fc::crypto::r1::compact_signature compact_sig = fc::crypto::r1::signature_from_ecdsa(key, *kd, sig, e.result());

      signature webauthnsig(compact_sig, assertion.auth_data, client_data);
      //fc::crypto::signature still doesn't allow creation of itself with an instance of a storage_type
      fc::datastream<size_t> dsz;
      fc::raw::pack(dsz, webauthnsig);
      char packed_sig[dsz.tellp()+1];
      fc::datastream<char*> ds(packed_sig, sizeof(packed_sig));
      fc::raw::pack(ds, '\x02'); //webauthn type
      fc::raw::pack(ds, webauthnsig);
      ds.seekp(0);

      signature_type final_signature;
      fc::raw::unpack(ds, final_signature);
      return final_signature;
   }

   std::unique_ptr<eosio::fido_device> fido;

   webauthn_wallet::public_key_credential_id_map _keys;

   fc::ec_key key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
};


}

webauthn_wallet::webauthn_wallet(const public_key_credential_id_map& key_map) : my(new detail::webauthn_wallet_impl(key_map)) {
}

webauthn_wallet::~webauthn_wallet() {
}

private_key_type webauthn_wallet::get_private_key(public_key_type pubkey) const {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Obtaining private key for a webauthn key is impossible");
}

bool webauthn_wallet::is_locked() const {
   return my->is_locked();
}
void webauthn_wallet::lock() {
   FC_ASSERT(!is_locked());
   my->lock();
}

void webauthn_wallet::unlock(string password) {
   my->unlock();
}
void webauthn_wallet::check_password(string password) {
   //just leave this as a noop for now; remove_key from wallet_mgr calls through here
}
void webauthn_wallet::set_password(string password) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "webauthn wallet cannot have a password set");
}

map<public_key_type, private_key_type> webauthn_wallet::list_keys() {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Getting the private keys from the webauthn wallet is impossible");
}
flat_set<public_key_type> webauthn_wallet::list_public_keys() {
   flat_set<public_key_type> keys;
   boost::copy(my->_keys | boost::adaptors::map_keys, std::inserter(keys, keys.end()));
   return keys;
}

bool webauthn_wallet::import_key(string wif_key) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "It is not possible to import a key in to the webauthn wallet");
}

string webauthn_wallet::create_key(string key_type) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "webauthn wallet cannot create keys");
}

bool webauthn_wallet::remove_key(string key) {
   FC_ASSERT(!is_locked());
   FC_THROW_EXCEPTION(chain::wallet_exception, "webauthn wallet cannot remove keys");
   return true;
}

fc::optional<signature_type> webauthn_wallet::try_sign_digest(const digest_type digest, const public_key_type public_key) {
   return my->try_sign_digest(digest, public_key);
}

}}
