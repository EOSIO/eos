/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosiolib/crypto.h>

#include "vm_api.h"

extern "C" {

void assert_sha256( const char* data, uint32_t length, const struct checksum256* hash ) {
   get_vm_api()->assert_sha256(data, length, hash );
}

void assert_sha1( const char* data, uint32_t length, const struct checksum160* hash ) {
   get_vm_api()->assert_sha1(data, length, hash );
}

void assert_sha512( const char* data, uint32_t length, const struct checksum512* hash ) {
   get_vm_api()->assert_sha512(data, length, hash );
}

void assert_ripemd160( const char* data, uint32_t length, const struct checksum160* hash ) {
   get_vm_api()->assert_ripemd160(data, length, hash );
}

void sha256( const char* data, uint32_t length, struct checksum256* hash ) {
   get_vm_api()->sha256(data, length, hash );
}

void sha1( const char* data, uint32_t length, struct checksum160* hash ) {
   get_vm_api()->sha1(data, length, hash );
}

void sha512( const char* data, uint32_t length, struct checksum512* hash ) {
   get_vm_api()->sha512(data, length, hash );
}

void ripemd160( const char* data, uint32_t length, struct checksum160* hash ) {
   get_vm_api()->ripemd160(data, length, hash );
}

int recover_key( const struct checksum256* digest, const char* sig, size_t siglen, char* pub, size_t publen ) {
   return get_vm_api()->recover_key(digest, sig, siglen, pub, publen);
}

void assert_recover_key( const struct checksum256* digest, const char* sig, size_t siglen, const char* pub, size_t publen ) {
   get_vm_api()->assert_recover_key(digest, sig, siglen, pub, publen);
}

}
