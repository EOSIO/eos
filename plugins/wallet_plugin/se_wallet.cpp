/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/wallet_plugin/se_wallet.hpp>
#include <eosio/wallet_plugin/macos_user_auth.h>
#include <eosio/chain/exceptions.hpp>

#include <fc/crypto/openssl.hpp>

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

#include <Security/Security.h>

#include <future>

namespace eosio { namespace wallet {

using namespace fc::crypto::r1;

namespace detail {

static void auth_callback(int success, void* data) {
   promise<bool>* prom = (promise<bool>*)data;
   prom->set_value(success);
}

struct se_wallet_impl {

   static public_key_data get_public_key_data(SecKeyRef key) {
      SecKeyRef pubkey = SecKeyCopyPublicKey(key);

      CFErrorRef error = nullptr;
      CFDataRef keyrep = nullptr;
      keyrep = SecKeyCopyExternalRepresentation(pubkey, &error);

      public_key_data pub_key_data;
      if(!error) {
         const UInt8* cfdata = CFDataGetBytePtr(keyrep);
         memcpy(pub_key_data.data+1, cfdata+1, 32);
         pub_key_data.data[0] = 0x02 + (cfdata[64]&1);
      }

      CFRelease(keyrep);
      CFRelease(pubkey);

      if(error) {
         string error_string = string_for_cferror(error);
         CFRelease(error);
         FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to get public key from Secure Enclave: ${m}", ("m", error_string));
      }

      return pub_key_data;
   }

   static public_key_type get_public_key(SecKeyRef key) {
      char serialized_pub_key[sizeof(public_key_data) + 1];
      serialized_pub_key[0] = 0x01;

      public_key_data pub_key_data = get_public_key_data(key);
      memcpy(serialized_pub_key+1, pub_key_data.data, sizeof(pub_key_data));

      public_key_type pub_key;
      fc::datastream<const char *> ds(serialized_pub_key, sizeof(serialized_pub_key));
      fc::raw::unpack(ds, pub_key);

      return pub_key;
   }

   static string string_for_cferror(CFErrorRef error) {
      CFStringRef errorString = CFCopyDescription(error);
      char buff[CFStringGetLength(errorString) + 1];
      string ret;
      if(CFStringGetCString(errorString, buff, sizeof(buff), kCFStringEncodingUTF8))
         ret = buff;
      else
         ret = "Unknown";
      CFRelease(errorString);
      return ret;
   }

#define XSTR(A) STR(A)
#define STR(A) #A

   void populate_existing_keys() {
      const void* keyAttrKeys[] = {
         kSecClass,
         kSecAttrKeyClass,
         kSecMatchLimit,
         kSecReturnRef,
         kSecAttrTokenID,
         kSecAttrAccessGroup
      };
      const void* keyAttrValues[] = {
         kSecClassKey,
         kSecAttrKeyClassPrivate,
         kSecMatchLimitAll,
         kCFBooleanTrue,
         kSecAttrTokenIDSecureEnclave,
#ifdef MAS_KEYCHAIN_GROUP
         CFSTR(XSTR(MAS_KEYCHAIN_GROUP))
#endif
      };
      CFDictionaryRef keyAttrDic = CFDictionaryCreate(nullptr, keyAttrKeys, keyAttrValues, sizeof(keyAttrValues)/sizeof(keyAttrValues[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

      CFArrayRef keyRefs = nullptr;
      if(SecItemCopyMatching(keyAttrDic, (CFTypeRef*)&keyRefs) || !keyRefs) {
         CFRelease(keyAttrDic);
         return;
      }

      CFIndex count = CFArrayGetCount(keyRefs);
      for(long i = 0; i < count; ++i) {
         public_key_type pub;
         try {
            SecKeyRef key = (SecKeyRef)CFRetain(CFArrayGetValueAtIndex(keyRefs, i));
            _keys[get_public_key(key)] = key;
         }
         catch(chain::wallet_exception&) {}
      }
      CFRelease(keyRefs);
      CFRelease(keyAttrDic);
   }

   public_key_type create() {
      SecAccessControlRef accessControlRef = SecAccessControlCreateWithFlags(nullptr, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, kSecAccessControlPrivateKeyUsage, nullptr);

      int keySizeValue = 256;
      CFNumberRef keySizeNumber = CFNumberCreate(NULL, kCFNumberIntType, &keySizeValue);

      const void* keyAttrKeys[] = {
         kSecAttrIsPermanent,
         kSecAttrAccessControl,
         kSecAttrAccessGroup
      };
      const void* keyAttrValues[] = {
         kCFBooleanTrue,
         accessControlRef,
#ifdef MAS_KEYCHAIN_GROUP
         CFSTR(XSTR(MAS_KEYCHAIN_GROUP))
#endif
      };
      CFDictionaryRef keyAttrDic = CFDictionaryCreate(NULL, keyAttrKeys, keyAttrValues, sizeof(keyAttrValues)/sizeof(keyAttrValues[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

      const void* attrKeys[] = {
         kSecAttrKeyType,
         kSecAttrKeySizeInBits,
         kSecAttrTokenID,
         kSecPrivateKeyAttrs
      };
      const void* atrrValues[] = {
         kSecAttrKeyTypeECSECPrimeRandom,
         keySizeNumber,
         kSecAttrTokenIDSecureEnclave,
         keyAttrDic
      };
      CFDictionaryRef attributesDic = CFDictionaryCreate(NULL, attrKeys, atrrValues, sizeof(attrKeys)/sizeof(attrKeys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

      CFErrorRef error = NULL;
      SecKeyRef privateKey = SecKeyCreateRandomKey(attributesDic, &error);
      string error_string;
      if(error) {
         error_string = string_for_cferror(error);
         CFRelease(error);
      }

      CFRelease(attributesDic);
      CFRelease(keyAttrDic);
      CFRelease(keySizeNumber);
      CFRelease(accessControlRef);

      if(error_string.size())
         FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to create key in Secure Enclave: ${m}", ("m", error_string));

      public_key_type pub;
      try {
         pub = get_public_key(privateKey);
      }
      catch(chain::wallet_exception&) {
         //possibly we should delete the key here?
         CFRelease(privateKey);
         throw;
      }
      _keys[pub] = privateKey;
      return pub;
   }

   optional<signature_type> try_sign_digest(const digest_type d, const public_key_type public_key) {
      auto it = _keys.find(public_key);
      if(it == _keys.end())
         return optional<signature_type>{};

      fc::ecdsa_sig sig = ECDSA_SIG_new();
      CFErrorRef error = nullptr;

      CFDataRef digestData = CFDataCreateWithBytesNoCopy(nullptr, (UInt8*)d.data(), d.data_size(), kCFAllocatorNull);
      CFDataRef signature = SecKeyCreateSignature(it->second, kSecKeyAlgorithmECDSASignatureDigestX962SHA256, digestData, &error);
      if(error) {
         string error_string = string_for_cferror(error);
         CFRelease(error);
         CFRelease(digestData);
         FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to sign digest in Secure Enclave: ${m}", ("m", error_string));
      }

      const UInt8* der_bytes = CFDataGetBytePtr(signature);
      long derSize = CFDataGetLength(signature);
      d2i_ECDSA_SIG(&sig.obj, &der_bytes, derSize);

      public_key_data kd;
      compact_signature compact_sig;
      try {
         kd = get_public_key_data(it->second);
         compact_sig = signature_from_ecdsa(key, kd, sig, d);
      } catch(chain::wallet_exception&) {
         CFRelease(signature);
         CFRelease(digestData);
         throw;
      }

      CFRelease(signature);
      CFRelease(digestData);

      char serialized_signature[sizeof(compact_sig) + 1];
      serialized_signature[0] = 0x01;
      memcpy(serialized_signature+1, compact_sig.data, sizeof(compact_sig));

      signature_type final_signature;
      fc::datastream<const char *> ds(serialized_signature, sizeof(serialized_signature));
      fc::raw::unpack(ds, final_signature);
      return final_signature;
   }

   bool remove_key(string public_key) {
      auto it = _keys.find(public_key_type{public_key});
      if(it == _keys.end())
         FC_THROW_EXCEPTION(chain::wallet_exception, "Given key to delete not found in Secure Enclave wallet");

      promise<bool> prom;
      future<bool> fut = prom.get_future();
      macos_user_auth(auth_callback, &prom, CFSTR("remove a key from your EOSIO wallet"));
      if(!fut.get())
         FC_THROW_EXCEPTION(chain::wallet_invalid_password_exception, "Local user authentication failed");

      CFDictionaryRef deleteDic = CFDictionaryCreate(nullptr, (const void**)&kSecValueRef, (const void**)&it->second, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

      OSStatus ret = SecItemDelete(deleteDic);
      CFRelease(deleteDic);

      if(ret)
         FC_THROW_EXCEPTION(chain::wallet_exception, "Failed to getremove key from Secure Enclave");

      CFRelease(it->second);
      _keys.erase(it);

      return true;
   }

   ~se_wallet_impl() {
      for(auto& k : _keys)
         CFRelease(k.second);
   }

   map<public_key_type,SecKeyRef> _keys;
   fc::ec_key key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
   bool locked = true;
};

static void check_signed() {
   OSStatus is_valid{0};
   pid_t pid = getpid();
   SecCodeRef code = nullptr;
   CFNumberRef pidnumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
   CFDictionaryRef piddict = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&kSecGuestAttributePid, (const void**)&pidnumber, 1, nullptr, nullptr);
   if(!SecCodeCopyGuestWithAttributes(nullptr, piddict, kSecCSDefaultFlags, &code)) {
      is_valid = SecCodeCheckValidity(code, kSecCSDefaultFlags, 0);
      CFRelease(code);
   }
   CFRelease(piddict);
   CFRelease(pidnumber);

   if(is_valid != errSecSuccess) {
      wlog("Application does not have a valid signature; Secure Enclave support disabled");
      EOS_THROW(secure_enclave_exception, "");
   }
}

}

se_wallet::se_wallet() : my(new detail::se_wallet_impl()) {
   detail::check_signed();

   //How to figure out of SE is available?!
   char model[256];
   size_t model_size = sizeof(model);
   if(sysctlbyname("hw.model", model, &model_size, nullptr, 0) == 0) {
      if(strncmp(model, "iMacPro", strlen("iMacPro")) == 0) {
         my->populate_existing_keys();
         return;
      }
      unsigned int major, minor;
      if(sscanf(model, "MacBookPro%u,%u", &major, &minor) == 2) {
         if((major >= 15) || (major >= 13 && minor >= 2)) {
            my->populate_existing_keys();
            return;
         }
      }
      if(sscanf(model, "Macmini%u", &major) == 1 && major >= 8) {
         my->populate_existing_keys();
         return;
      }
      if(sscanf(model, "MacBookAir%u", &major) == 1 && major >= 8) {
         my->populate_existing_keys();
         return;
      }
   }

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
   flat_set<public_key_type> keys;
   boost::copy(my->_keys | boost::adaptors::map_keys, std::inserter(keys, keys.end()));
   return keys;
}

bool se_wallet::import_key(string wif_key) {
   FC_THROW_EXCEPTION(chain::wallet_exception, "It is not possible to import a key in to the Secure Enclave wallet");
}

string se_wallet::create_key(string key_type) {
   return (string)my->create();
}

bool se_wallet::remove_key(string key) {
   EOS_ASSERT(!is_locked(), wallet_locked_exception, "You can not remove a key from a locked wallet");
   return my->remove_key(key);
}

optional<signature_type> se_wallet::try_sign_digest(const digest_type digest, const public_key_type public_key) {
   return my->try_sign_digest(digest, public_key);
}

}}