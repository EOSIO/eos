#include <eosio/se-helpers/se-helpers.hpp>

#include <fc/fwd_impl.hpp>
#include <fc/scoped_exit.hpp>

namespace eosio::secure_enclave {

static std::string string_for_cferror(CFErrorRef error) {
   CFStringRef errorString = CFCopyDescription(error);
   auto release_errorString = fc::make_scoped_exit([&errorString](){CFRelease(errorString);});

   std::string ret(CFStringGetLength(errorString), '\0');
   if(!CFStringGetCString(errorString, ret.data(), ret.size(), kCFStringEncodingUTF8))
      ret = "Unknown";
   return ret;
}

secure_enclave_key::impl::~impl() {
  if(key_ref)
      CFRelease(key_ref);
  key_ref = NULL;
}

void secure_enclave_key::impl::populate_public_key() {
  //without a good way to create fc::public_key direct, create a serialized version to create a public_key from
  char serialized_public_key[1 + sizeof(fc::crypto::r1::public_key_data)] = {fc::get_index<fc::crypto::public_key::storage_type, fc::crypto::r1::public_key_shim>()};

  SecKeyRef pubkey = SecKeyCopyPublicKey(key_ref);

  CFErrorRef error = NULL;
  CFDataRef keyrep = NULL;
  keyrep = SecKeyCopyExternalRepresentation(pubkey, &error);

  if(!error) {
      const UInt8 *cfdata = CFDataGetBytePtr(keyrep);
      memcpy(serialized_public_key + 2, cfdata + 1, 32);
      serialized_public_key[1] = 0x02u + (cfdata[64] & 1u);
  }

  CFRelease(keyrep);
  CFRelease(pubkey);

  if(error) {
      auto release_error = fc::make_scoped_exit([&error](){CFRelease(error);});
      FC_ASSERT(false, "Failed to get public key from Secure Enclave: ${m}", ("m", string_for_cferror(error)));
  }

  fc::datastream<const char*> ds(serialized_public_key, sizeof(serialized_public_key));
  fc::raw::unpack(ds, pub_key);
}

secure_enclave_key::secure_enclave_key(void* seckeyref) {
   my.key_ref = (SecKeyRef)(seckeyref);
   my.populate_public_key();
   CFRetain(my.key_ref);
}

secure_enclave_key::secure_enclave_key(const secure_enclave_key& o) {
   my.key_ref = (SecKeyRef)CFRetain(o.my.key_ref);
   my.pub_key = o.public_key();
}

secure_enclave_key::secure_enclave_key(secure_enclave_key&& o) {
   my.key_ref = o.my.key_ref;
   o.my.key_ref = NULL;
   my.pub_key = o.my.pub_key;
}

const fc::crypto::public_key &secure_enclave_key::public_key() const {
   return my.pub_key;
}

fc::crypto::signature secure_enclave_key::sign(const fc::sha256& digest) const {
   fc::ecdsa_sig sig = ECDSA_SIG_new();
   CFErrorRef error = NULL;

   CFDataRef digestData = CFDataCreateWithBytesNoCopy(NULL, (UInt8*)digest.data(), digest.data_size(), kCFAllocatorNull);
   CFDataRef signature = SecKeyCreateSignature(my.key_ref, kSecKeyAlgorithmECDSASignatureDigestX962SHA256, digestData, &error);

   auto cleanup = fc::make_scoped_exit([&digestData, &signature]() {
      CFRelease(digestData);
      if(signature)
         CFRelease(signature);
   });

   if(error) {
      auto release_error = fc::make_scoped_exit([&error](){CFRelease(error);});
      std::string error_string = string_for_cferror(error);
      FC_ASSERT(false, "Failed to sign digest in Secure Enclave: ${m}", ("m", error_string));
   }

   const UInt8* der_bytes = CFDataGetBytePtr(signature);
   long derSize = CFDataGetLength(signature);
   d2i_ECDSA_SIG(&sig.obj, &der_bytes, derSize);

   char serialized_signature[sizeof(fc::crypto::r1::compact_signature) + 1] = {fc::get_index<fc::crypto::signature::storage_type, fc::crypto::r1::signature_shim>()};

   fc::crypto::r1::compact_signature* compact_sig = (fc::crypto::r1::compact_signature *)(serialized_signature + 1);
   fc::ec_key key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
   *compact_sig = fc::crypto::r1::signature_from_ecdsa(key, std::get<fc::crypto::r1::public_key_shim>(my.pub_key._storage)._data, sig, digest);

   fc::crypto::signature final_signature;
   fc::datastream<const char*> ds(serialized_signature, sizeof(serialized_signature));
   fc::raw::unpack(ds, final_signature);
   return final_signature;
}

secure_enclave_key create_key() {
   SecAccessControlRef accessControlRef = SecAccessControlCreateWithFlags(NULL, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, kSecAccessControlPrivateKeyUsage, NULL);

   int keySizeValue = 256;
   CFNumberRef keySizeNumber = CFNumberCreate(NULL, kCFNumberIntType, &keySizeValue);

   const void* keyAttrKeys[] = {
      kSecAttrIsPermanent,
      kSecAttrAccessControl
   };
   const void* keyAttrValues[] = {
      kCFBooleanTrue,
      accessControlRef
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

   auto cleanup = fc::make_scoped_exit([&attributesDic, &keyAttrDic, &keySizeNumber, &accessControlRef]() {
      CFRelease(attributesDic);
      CFRelease(keyAttrDic);
      CFRelease(keySizeNumber);
      CFRelease(accessControlRef);
   });

   CFErrorRef error = NULL;
   SecKeyRef privateKey = SecKeyCreateRandomKey(attributesDic, &error);
   if(error) {
      auto release_error = fc::make_scoped_exit([&error](){CFRelease(error);});
      FC_ASSERT(false, "Failed to create key in Secure Enclave: ${m}", ("m", string_for_cferror(error)));
   }

   return secure_enclave_key(privateKey);
}

std::set<secure_enclave_key> get_all_keys() {
   const void* keyAttrKeys[] = {
      kSecClass,
      kSecAttrKeyClass,
      kSecMatchLimit,
      kSecReturnRef,
      kSecAttrTokenID
   };
   const void* keyAttrValues[] = {
      kSecClassKey,
      kSecAttrKeyClassPrivate,
      kSecMatchLimitAll,
      kCFBooleanTrue,
      kSecAttrTokenIDSecureEnclave,
   };
   CFDictionaryRef keyAttrDic = CFDictionaryCreate(NULL, keyAttrKeys, keyAttrValues, sizeof(keyAttrValues)/sizeof(keyAttrValues[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
   auto cleanup_keyAttrDic = fc::make_scoped_exit([&keyAttrDic](){CFRelease(keyAttrDic);});

   std::set<secure_enclave_key> ret;

   CFArrayRef keyRefs = NULL;
   if(SecItemCopyMatching(keyAttrDic, (CFTypeRef*)&keyRefs) || !keyRefs)
      return ret;
   auto cleanup_keyRefs = fc::make_scoped_exit([&keyRefs](){CFRelease(keyRefs);});

   CFIndex count = CFArrayGetCount(keyRefs);
   for(long i = 0; i < count; ++i)
      ret.emplace((void*)CFArrayGetValueAtIndex(keyRefs, i));

   return ret;
}

void delete_key(secure_enclave_key&& key) {
   CFDictionaryRef deleteDic = CFDictionaryCreate(NULL, (const void**)&kSecValueRef, (const void**)&key.my.key_ref, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

   OSStatus ret = SecItemDelete(deleteDic);
   CFRelease(deleteDic);

   FC_ASSERT(ret == 0, "Failed to remove key from Secure Enclave");
}

bool hardware_supports_secure_enclave() {
   //How to figure out if SE is available?!
   char model[256];
   size_t model_size = sizeof(model);
   if(sysctlbyname("hw.model", model, &model_size, nullptr, 0) == 0) {
      if(strncmp(model, "iMacPro", strlen("iMacPro")) == 0)
         return true;
      unsigned int major, minor;
      if(sscanf(model, "MacBookPro%u,%u", &major, &minor) == 2) {
         if((major >= 15) || (major >= 13 && minor >= 2)) {
            return true;
         }
      }
      if(sscanf(model, "Macmini%u", &major) == 1 && major >= 8)
         return true;
      if(sscanf(model, "MacBookAir%u", &major) == 1 && major >= 8)
         return true;
      if(sscanf(model, "MacPro%u", &major) == 1 && major >= 7)
         return true;
   }

   return false;
}

bool application_signed() {
   OSStatus is_valid{-1};
   pid_t pid = getpid();
   SecCodeRef code = NULL;
   CFNumberRef pidnumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
   CFDictionaryRef piddict = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&kSecGuestAttributePid, (const void**)&pidnumber, 1, NULL, NULL);
   if(!SecCodeCopyGuestWithAttributes(NULL, piddict, kSecCSDefaultFlags, &code)) {
      is_valid = SecCodeCheckValidity(code, kSecCSDefaultFlags, 0);
      CFRelease(code);
   }
   CFRelease(piddict);
   CFRelease(pidnumber);

   return is_valid == errSecSuccess;
}

}
