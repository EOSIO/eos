#include <fc/crypto/elliptic_r1.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/crypto/base58.hpp>
#include <fc/crypto/signature.hpp>
#include <fc/crypto/public_key.hpp>

#include <boost/program_options.hpp>

#include <Security/Security.h>

#include <array>
#include "r1_signature_compactor.hpp"

namespace po = boost::program_options;
using namespace std;
using namespace fc::crypto::r1;

//get a copy of our key info, should it exist in the keychain
CFDictionaryRef CopyOurKeyInfo() {
   const void* keyAttrKeys[] = {
      kSecClass,
      kSecAttrKeyClass,
      kSecAttrApplicationTag,
      kSecReturnAttributes,
      kSecReturnRef
   };
   const void* keyAttrValues[] = {
      kSecClassKey,
      kSecAttrKeyClassPrivate,
      CFSTR("one.block.applesedemo"),
      kCFBooleanTrue,
      kCFBooleanTrue
   };
   CFDictionaryRef keyAttrDic = CFDictionaryCreate(nullptr, keyAttrKeys, keyAttrValues, sizeof(keyAttrKeys)/sizeof(keyAttrKeys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

   CFDictionaryRef attributes = nullptr;
   SecItemCopyMatching(keyAttrDic, (CFTypeRef*)&attributes);

   CFRelease(keyAttrDic);
   return attributes;
}

SecKeyRef CopyOurKey() {
   CFDictionaryRef attributes = CopyOurKeyInfo();
   if(!attributes)
      return nullptr;
   SecKeyRef key = (SecKeyRef)CFDictionaryGetValue(attributes, kSecValueRef);
   if(key)
      CFRetain(key);
   CFRelease(attributes);
   return key;
}

public_key_data get_compressed_pub_for_key(SecKeyRef key) {
   SecKeyRef pubkey = SecKeyCopyPublicKey(key);
   
   CFErrorRef error = nullptr;
   CFDataRef keyrep = SecKeyCopyExternalRepresentation(pubkey, &error);
   assert(CFDataGetLength(keyrep) == 65);

   public_key_data pub_key_data;
   const UInt8* cfdata = CFDataGetBytePtr(keyrep);

   memcpy(pub_key_data.data+1, cfdata+1, 32);
   pub_key_data.data[0] = 0x02 + (cfdata[64]&1);

   CFRelease(keyrep);
   CFRelease(pubkey);

   return pub_key_data;
}

void print_pub_for_key(SecKeyRef key) {
   fc::crypto::checksummed_data<fc::crypto::r1::public_key_data> pub_wrapper;
   pub_wrapper.data = get_compressed_pub_for_key(key);
   pub_wrapper.check = fc::crypto::checksummed_data<fc::crypto::r1::public_key_data>::calculate_checksum(pub_wrapper.data, "R1");
   std::vector<char> checksummed = fc::raw::pack(pub_wrapper);
   
   cout << "public_key(PUB_R1_" << fc::to_base58(checksummed.data(), checksummed.size()) << ")" << endl;
}

void print_attributes() {
   CFDictionaryRef key_info = CopyOurKeyInfo();
   if(!key_info)
      cout << "No key currently in SE" << endl;
   else {
      CFShow(key_info);
      CFRelease(key_info);
   }

   SecKeyRef key = CopyOurKey();
   print_pub_for_key(key);
   CFRelease(key);
}

void remove() {
   SecKeyRef key = CopyOurKey();
   if(!key) {
      cout << "No key current in SE" << endl;
      return;
   }
   CFRelease(key);
   //hmmm. previously I was trying to delete based on the key reference; no such luck

   const void* deleteKeys[] = {
      kSecClass,
      kSecAttrApplicationTag,
   };
   const void* deleteValues[] = {
      kSecClassKey,
      CFSTR("one.block.applesedemo"),
   };
   CFDictionaryRef deleteDic = CFDictionaryCreate(nullptr, deleteKeys, deleteValues, sizeof(deleteKeys)/sizeof(deleteKeys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

   OSStatus ret = SecItemDelete(deleteDic);
   if(!ret)
      cout << "Successfully removed key" << endl;
   else
      cout << "Error removing key " << ret << endl;

   CFRelease(deleteDic);
}

void create(SecAccessControlCreateFlags flags) {
   SecKeyRef key = CopyOurKey();
   if(key) {
      cout << "Already have a key in SE; not creating more" << endl;
      CFRelease(key);
      return;
   }

   SecAccessControlRef accessControlRef = SecAccessControlCreateWithFlags(nullptr, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, flags | kSecAccessControlPrivateKeyUsage, nullptr);

   int keySizeValue = 256;
   CFNumberRef keySizeNumber = CFNumberCreate(NULL, kCFNumberIntType, &keySizeValue);

   const void* keyAttrKeys[] = {
      kSecAttrIsPermanent,
      kSecAttrApplicationTag,
      kSecAttrAccessControl
   };
   const void* keyAttrValues[] = {
      kCFBooleanTrue,
      CFSTR("one.block.applesedemo"),
      accessControlRef
   };
   CFDictionaryRef keyAttrDic = CFDictionaryCreate(NULL, keyAttrKeys, keyAttrValues, sizeof(keyAttrKeys)/sizeof(keyAttrKeys[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

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
   CFDictionaryRef attributesDic = CFDictionaryCreate(NULL, attrKeys, atrrValues, sizeof(attrKeys)/sizeof(atrrValues[0]), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

   CFErrorRef error = NULL;
   SecKeyRef privateKey = SecKeyCreateRandomKey(attributesDic, &error);
   if(error) {
      cout << "Failed" << endl;
      CFShow(error);
   }
   else {
      cout << "Successfully created" << endl;
      print_pub_for_key(privateKey);
      CFRelease(privateKey);
   }

   
   CFRelease(attributesDic);
   CFRelease(keyAttrDic);
   CFRelease(keySizeNumber);
   CFRelease(accessControlRef);
}

void sign(const string& hex) {
   SecKeyRef key = CopyOurKey();
   if(!key) {
      cout << "No key currently in SE" << endl;
      return;
   }

   fc::ecdsa_sig sig = ECDSA_SIG_new();
   BIGNUM *r = BN_new(), *s = BN_new();
   const UInt8* der_bytes = nullptr;
   fc::sha256 digest(hex);
   CFErrorRef error = nullptr;
   fc::crypto::r1::compact_signature compacted;
   public_key_data pub;
   fc::crypto::checksummed_data<fc::crypto::r1::compact_signature> signature_wrapper;
   std::vector<char> checksummed;
   std::string blah;

   CFDataRef digestData = CFDataCreateWithBytesNoCopy(nullptr, (UInt8*)digest.data(), digest.data_size(), kCFAllocatorNull);

   CFDataRef signature = SecKeyCreateSignature(key, kSecKeyAlgorithmECDSASignatureDigestX962SHA256, digestData, &error);
   if(error) {
      cout << "Failed to sign" << endl;
      CFShow(error);
      goto err;
   }
   
   der_bytes = CFDataGetBytePtr(signature);
   assert(der_bytes[0] == 0x30);
   assert(der_bytes[2] == 0x02);
   assert(der_bytes[4+der_bytes[3]] == 0x02);
   
   BN_bin2bn(der_bytes+4, der_bytes[3], r);
   BN_bin2bn(der_bytes+6+der_bytes[3], der_bytes[4+der_bytes[3]+1], s);
   ECDSA_SIG_set0(sig, r, s);

   pub = get_compressed_pub_for_key(key);

   signature_wrapper.data = compact_r1(pub, sig, digest);
   signature_wrapper.check = fc::crypto::checksummed_data<fc::crypto::r1::compact_signature>::calculate_checksum(signature_wrapper.data, "R1");
   checksummed = fc::raw::pack(signature_wrapper);
   
   cout << "signature(SIG_R1_" << fc::to_base58(checksummed.data(), checksummed.size()) << ")" << endl;

err:
   CFRelease(key);
   CFRelease(digestData);
}

void recover(vector<string> params) {
   fc::sha256 digest(params[0]);
   fc::crypto::signature sig(params[1]);

   cout << fc::crypto::public_key(sig, digest) << endl;
}

int main(int argc, char* argv[]) {
   pid_t pid = getpid();
   SecCodeRef code = nullptr;
   CFNumberRef pidnumber = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
   CFDictionaryRef piddict = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&kSecGuestAttributePid, (const void**)&pidnumber, 1, nullptr, nullptr);
   if(!SecCodeCopyGuestWithAttributes(nullptr, piddict, kSecCSDefaultFlags, &code)) {
      if(SecCodeCheckValidity(code, kSecCSDefaultFlags, 0)) {
         cout << "This application is NOT signed. For access to the secure enclave the application must be signed. Exiting." << endl;
         return 1;
      }
      CFRelease(code);
   }
   CFRelease(piddict);
   CFRelease(pidnumber);

   po::options_description desc_help;
   desc_help.add_options()("help", "halp");

   po::options_description desc_create("Create and store a key; This application only allows a single key to be stored at a time");
   desc_create.add_options()
      ("create-se", "Create a key to be stored in Secure Enclave; no TouchID requirements")
      ("create-se-interactive", "Create a key to be stored in Secure Enclave; fingerprint or password required on use")
      ("create-se-touch-only", "Create a key to be stored in Secure Enclave; fingerprint required on use");

   po::options_description desc_key("Other key operations");
   desc_key.add_options()
      ("print-attributes", "Print the attributes and public key of the stored key")
      ("remove", "Remove the key");

   po::options_description sign_key("Signing & Key Recovery");
   sign_key.add_options()
      ("sign", po::value<string>()->value_name("DIGEST"), "Sign sha256 via SE")
      ("recover", po::value<vector<string>>()->multitoken()->value_name("DIGEST COMPACT_SIG"), "Recover and print pubkey");

   po::options_description all_desc;
   all_desc.add(desc_create).add(desc_key).add(sign_key);
   po::options_description all_desc_and_help;
   all_desc_and_help.add(all_desc).add(desc_help);

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, all_desc_and_help), vm);
   po::notify(vm);
   
   if(vm.count("create-se"))
      create(0);
   else if(vm.count("create-se-touch-only"))
      create(kSecAccessControlTouchIDAny);
   else if(vm.count("create-se-interactive"))
      create(kSecAccessControlUserPresence);
   else if(vm.count("print-attributes"))
      print_attributes();
   else if(vm.count("remove"))
      remove();
   else if(vm.count("sign"))
      sign(vm["sign"].as<string>());
   else if (!vm["recover"].empty() && vm["recover"].as<vector<string>>().size() == 2)
      recover(vm["recover"].as<vector<string>>());
   else
      cout << all_desc << endl;
   
   return 0;
}
