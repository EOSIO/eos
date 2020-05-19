#include <eosio/wallet_plugin/se_wallet.hpp>
#include <eosio/wallet_plugin/macos_user_auth.h>
#include <eosio/chain/exceptions.hpp>
#include <eosio/se-helpers/se-helpers.hpp>

#include <Security/Security.h>

#include <future>

namespace eosio { namespace wallet {

namespace detail {

static void auth_callback(int success, void* data) {
   promise<bool>* prom = (promise<bool>*)data;
   prom->set_value(success);
}

struct se_wallet_impl {
   bool locked = true;
};

}

se_wallet::se_wallet() : my(new detail::se_wallet_impl()) {
   if(!secure_enclave::application_signed()) {
      wlog("Application does not have a valid signature; Secure Enclave support disabled");
      EOS_THROW(secure_enclave_exception, "");
   }

   if(!secure_enclave::hardware_supports_secure_enclave())
      EOS_THROW(secure_enclave_exception, "Secure Enclave not supported on this hardware");
}

se_wallet::~se_wallet() {
}

private_key_type se_wallet::get_private_key(public_key_type pubkey) const {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Obtaining private key for a key stored in Secure Enclave is impossible");
}

bool se_wallet::is_locked() const {
   return my->locked;
}
void se_wallet::lock() {
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "You can not lock an already locked wallet");
   my->locked = true;
}

void se_wallet::unlock(string password) {
   promise<bool> prom;
   future<bool> fut = prom.get_future();
   macos_user_auth(detail::auth_callback, &prom, CFSTR("unlock your EOSIO wallet"));
   if(!fut.get())
      FC_THROW_EXCEPTION(chain::wallet_invalid_password_exception, "Local user authentication failed");
   my->locked = false;
}
void se_wallet::check_password(string password) {
   //just leave this as a noop for now; remove_key from wallet_mgr calls through here
}
void se_wallet::set_password(string password) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Secure Enclave wallet cannot have a password set");
}

map<public_key_type, private_key_type> se_wallet::list_keys() {
   FC_THROW_EXCEPTION(chain::wallet_exception, "Getting the private keys from the Secure Enclave wallet is impossible");
}
flat_set<public_key_type> se_wallet::list_public_keys() {
   flat_set<public_key_type> ret;
   for(const secure_enclave::secure_enclave_key& sekey : secure_enclave::get_all_keys())
      ret.emplace(sekey.public_key());
   return ret;
}

bool se_wallet::import_key(string wif_key) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "It is not possible to import a key in to the Secure Enclave wallet");
}

string se_wallet::create_key(string key_type) {
   EOS_ASSERT(key_type.empty() || key_type == "R1", chain::unsupported_key_type_exception, "Secure Enclave wallet only supports R1 keys");
   return secure_enclave::create_key().public_key().to_string();
}

bool se_wallet::remove_key(string key) {
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "You can not remove a key from a locked wallet");

   auto se_keys = secure_enclave::get_all_keys();

   for(auto it = se_keys.begin(); it != se_keys.end(); ++it)
      if(it->public_key().to_string() == key) {
         eosio::secure_enclave::delete_key(std::move(se_keys.extract(it).value()));
         return true;
      }

   FC_THROW_EXCEPTION(chain::wallet_exception, "Given key to delete not found in Secure Enclave wallet");
}

fc::optional<signature_type> se_wallet::try_sign_digest(const digest_type digest, const public_key_type public_key) {
   auto se_keys = secure_enclave::get_all_keys();

   for(auto it = se_keys.begin(); it != se_keys.end(); ++it)
      if(it->public_key() == public_key)
         return it->sign(digest);

   return fc::optional<signature_type>{};
}

}}
