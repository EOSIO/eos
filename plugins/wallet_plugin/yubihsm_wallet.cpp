/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <appbase/application.hpp>

#include <eosio/wallet_plugin/yubihsm_wallet.hpp>
#include <eosio/chain/exceptions.hpp>
#include <yubihsm.h>

#include <fc/crypto/openssl.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <dlfcn.h>

namespace eosio { namespace wallet {

using namespace fc::crypto::r1;

namespace detail {

struct yubihsm_wallet_impl {
   using key_map_type = map<public_key_type,uint16_t>;

   yubihsm_wallet_impl(const string& ep, const uint16_t ak) : endpoint(ep), authkey(ak) {
      yh_rc rc;
      if((rc = yh_init()))
         FC_THROW("yubihsm init failure: ${c}", ("c", yh_strerror(rc)));
   }

   ~yubihsm_wallet_impl() {
      lock();
      yh_exit();
      //bizarre, is there no way to destroy a yh_connector??

      ///XXX Probably a race condition on timer shutdown and appbase destruction
   }

   bool is_locked() const {
      return !connector;
   }

   key_map_type::iterator populate_key_map_with_keyid(const uint16_t key_id) {
      yh_rc rc;
      size_t blob_sz = 128;
      uint8_t blob[blob_sz];
      if((rc = yh_util_get_public_key(session, key_id, blob, &blob_sz, nullptr)))
         FC_THROW_EXCEPTION(chain::wallet_exception, "yh_util_get_public_key failed: ${m}", ("m", yh_strerror(rc)));
      if(blob_sz != 64)
         FC_THROW_EXCEPTION(chain::wallet_exception, "unexpected pubkey size from yh_util_get_public_key");

      ///XXX This is junky and common with SE wallet; commonize it
      char serialized_pub_key[sizeof(public_key_data) + 1];
      serialized_pub_key[0] = 0x01; //means R1 key
      serialized_pub_key[1] = 0x02 + (blob[63]&1); //R1 header; even or odd Y
      memcpy(serialized_pub_key+2, blob, 32); //copy in the 32 bytes of X

      public_key_type pub_key;
      fc::datastream<const char *> ds(serialized_pub_key, sizeof(serialized_pub_key));
      fc::raw::unpack(ds, pub_key);

      return _keys.emplace(pub_key, key_id).first;
   }

   void unlock(const string& password) {
      yh_rc rc;

      try {
         if((rc = yh_init_connector(endpoint.c_str(), &connector)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "Failled to initialize yubihsm connector URL: ${c}", ("c", yh_strerror(rc)));
         if((rc = yh_connect(connector, 0)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to connect to YubiHSM connector: ${m}", ("m", yh_strerror(rc)));
         if((rc = yh_create_session_derived(connector, authkey, (const uint8_t *)password.data(), password.size(), false, &session)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to create YubiHSM session: ${m}", ("m", yh_strerror(rc)));
         if((rc = yh_authenticate_session(session)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to authenticate YubiHSM session: ${m}", ("m", yh_strerror(rc)));

         yh_object_descriptor authkey_desc;
         if((rc = yh_util_get_object_info(session, authkey, YH_AUTHENTICATION_KEY, &authkey_desc)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to get authkey info: ${m}", ("m", yh_strerror(rc)));

         authkey_caps = authkey_desc.capabilities;
         authkey_domains = authkey_desc.domains;

         if(!yh_check_capability(&authkey_caps, "sign-ecdsa"))
            FC_THROW_EXCEPTION(chain::wallet_exception, "Given authkey cannot perform signing");

         size_t found_objects_n = 64*1024;
         yh_object_descriptor found_objs[found_objects_n];
         yh_capabilities find_caps;
         yh_string_to_capabilities("sign-ecdsa", &find_caps);
         if((rc = yh_util_list_objects(session, 0, YH_ASYMMETRIC_KEY, 0, &find_caps, YH_ALGO_EC_P256, nullptr, found_objs, &found_objects_n)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "yh_util_list_objects failed: ${m}", ("m", yh_strerror(rc)));

         for(size_t i = 0; i < found_objects_n; ++i)
            populate_key_map_with_keyid(found_objs[i].id);
      }
      catch(chain::wallet_exception& e) {
         lock();
         throw;
      }

      prime_keepalive_timer();
   }

   void lock() {
      if(session) {
         yh_util_close_session(session);
         yh_destroy_session(&session);
      }
      session = nullptr;
      if(connector)
         yh_disconnect(connector);
      //it would seem like this would leak-- there is no destroy() call for it. But I clearly can't reuse connectors
      // as that fails with a "Unable to find a suitable connector"
      connector = nullptr;

      _keys.clear();
      keepalive_timer.cancel();
   }

   void prime_keepalive_timer() {
      keepalive_timer.expires_at(std::chrono::steady_clock::now() + std::chrono::seconds(20));
      keepalive_timer.async_wait([this](auto ec){
         if(ec || !session)
            return;

         uint8_t data, resp;
         yh_cmd resp_cmd;
         size_t resp_sz = 1;
         if(yh_send_secure_msg(session, YHC_ECHO, &data, 1, &resp_cmd, &resp, &resp_sz))
            lock();
         else
            prime_keepalive_timer();
      });
   }

   optional<signature_type> try_sign_digest(const digest_type d, const public_key_type public_key) {
      auto it = _keys.find(public_key);
      if(it == _keys.end())
         return optional<signature_type>{};

      size_t der_sig_sz = 128;
      uint8_t der_sig[der_sig_sz];
      yh_rc rc;
      if((rc = yh_util_sign_ecdsa(session, it->second, (uint8_t*)d.data(), d.data_size(), der_sig, &der_sig_sz))) {
         lock();
         FC_THROW_EXCEPTION(chain::wallet_exception, "yh_util_sign_ecdsa failed: ${m}", ("m", yh_strerror(rc)));
      }

      ///XXX a lot of this below is similar to SE wallet; commonize it in non-junky way
      fc::ecdsa_sig sig = ECDSA_SIG_new();
      BIGNUM *r = BN_new(), *s = BN_new();
      BN_bin2bn(der_sig+4, der_sig[3], r);
      BN_bin2bn(der_sig+6+der_sig[3], der_sig[4+der_sig[3]+1], s);
      ECDSA_SIG_set0(sig, r, s);

      char pub_key_shim_data[64];
      fc::datastream<char *> eds(pub_key_shim_data, sizeof(pub_key_shim_data));
      fc::raw::pack(eds, it->first);
      public_key_data* kd = (public_key_data*)(pub_key_shim_data+1);

      compact_signature compact_sig;
      compact_sig = signature_from_ecdsa(key, *kd, sig, d);

      char serialized_signature[sizeof(compact_sig) + 1];
      serialized_signature[0] = 0x01;
      memcpy(serialized_signature+1, compact_sig.data, sizeof(compact_sig));

      signature_type final_signature;
      fc::datastream<const char *> ds(serialized_signature, sizeof(serialized_signature));
      fc::raw::unpack(ds, final_signature);
      return final_signature;
   }

   public_key_type create() {
      if(!yh_check_capability(&authkey_caps, "generate-asymmetric-key"))
         FC_THROW_EXCEPTION(chain::wallet_exception, "Given authkey cannot create keys");

      yh_rc rc;
      uint16_t new_key_id = 0;
      yh_capabilities creation_caps = {};
      if(yh_string_to_capabilities("sign-ecdsa:export-wrapped", &creation_caps))
         FC_THROW_EXCEPTION(chain::wallet_exception, "Cannot create caps mask");

      try {
         if((rc = yh_util_generate_ec_key(session, &new_key_id, "keosd created key", authkey_domains, &creation_caps, YH_ALGO_EC_P256)))
            FC_THROW_EXCEPTION(chain::wallet_exception, "yh_util_generate_ec_key failed: ${m}", ("m", yh_strerror(rc)));
         return populate_key_map_with_keyid(new_key_id)->first;
      }
      catch(chain::wallet_exception& e) {
         lock();
         throw;
      }
   }

   yh_connector* connector = nullptr;
   yh_session* session = nullptr;
   string endpoint;
   uint16_t authkey;

   map<public_key_type,uint16_t> _keys;

   yh_capabilities authkey_caps;
   uint16_t authkey_domains;

   boost::asio::steady_timer keepalive_timer{appbase::app().get_io_service()};
   fc::ec_key key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
};


}

yubihsm_wallet::yubihsm_wallet(const string& connector, const uint16_t authkey) : my(new detail::yubihsm_wallet_impl(connector, authkey)) {
}

yubihsm_wallet::~yubihsm_wallet() {
}

private_key_type yubihsm_wallet::get_private_key(public_key_type pubkey) const {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Obtaining private key for a key stored in YubiHSM is impossible");
}

bool yubihsm_wallet::is_locked() const {
   return my->is_locked();
}
void yubihsm_wallet::lock() {
   FC_ASSERT(!is_locked());
   my->lock();
}

void yubihsm_wallet::unlock(string password) {
   my->unlock(password);
}
void yubihsm_wallet::check_password(string password) {
   //just leave this as a noop for now; remove_key from wallet_mgr calls through here
}
void yubihsm_wallet::set_password(string password) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "YubiHSM wallet cannot have a password set");
}

map<public_key_type, private_key_type> yubihsm_wallet::list_keys() {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Getting the private keys from the YubiHSM wallet is impossible");
}
flat_set<public_key_type> yubihsm_wallet::list_public_keys() {
   flat_set<public_key_type> keys;
   boost::copy(my->_keys | boost::adaptors::map_keys, std::inserter(keys, keys.end()));
   return keys;
}

bool yubihsm_wallet::import_key(string wif_key) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "It is not possible to import a key in to the YubiHSM wallet");
}

string yubihsm_wallet::create_key(string key_type) {
   return (string)my->create();
}

bool yubihsm_wallet::remove_key(string key) {
   FC_ASSERT(!is_locked());
   return true;
}

optional<signature_type> yubihsm_wallet::try_sign_digest(const digest_type digest, const public_key_type public_key) {
   return my->try_sign_digest(digest, public_key);
}

}}