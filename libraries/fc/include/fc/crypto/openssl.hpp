#pragma once
#include <openssl/ec.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/ecdh.h>
#include <openssl/sha.h>
#include <openssl/obj_mac.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps);
int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s);
#endif

/** 
 * @file openssl.hpp
 * Provides common utility calls for wrapping openssl c api.
 */
namespace fc 
{
  class path;

    template <typename ssl_type>
    struct ssl_wrapper
    {
        ssl_wrapper(ssl_type* obj):obj(obj) {}

        operator ssl_type*() { return obj; }
        operator const ssl_type*() const { return obj; }
        ssl_type* operator->() { return obj; }
        const ssl_type* operator->() const { return obj; }

        ssl_type* obj;
    };

    #define SSL_TYPE(name, ssl_type, free_func) \
        struct name  : public ssl_wrapper<ssl_type> \
        { \
            name(ssl_type* obj=nullptr) \
              : ssl_wrapper(obj) {} \
            ~name() \
            { \
                if( obj != nullptr ) \
                free_func(obj); \
            } \
        };

    SSL_TYPE(ec_group,       EC_GROUP,       EC_GROUP_free)
    SSL_TYPE(ec_point,       EC_POINT,       EC_POINT_free)
    SSL_TYPE(ecdsa_sig,      ECDSA_SIG,      ECDSA_SIG_free)
    SSL_TYPE(bn_ctx,         BN_CTX,         BN_CTX_free)
    SSL_TYPE(evp_cipher_ctx, EVP_CIPHER_CTX, EVP_CIPHER_CTX_free )
    SSL_TYPE(ec_key,         EC_KEY,         EC_KEY_free)

    /** allocates a bignum by default.. */
    struct ssl_bignum : public ssl_wrapper<BIGNUM>
    {
        ssl_bignum() : ssl_wrapper(BN_new()) {}
        ~ssl_bignum() { BN_free(obj); }
    };

    /** Allows to explicitly specify OpenSSL configuration file path to be loaded at OpenSSL library init.
        If not set OpenSSL will try to load the conf. file (openssl.cnf) from the path it was
        configured with what caused serious Keyhotee startup bugs on some Win7, Win8 machines.
        \warning to be effective this method should be used before any part using OpenSSL, especially
        before init_openssl call
    */
    void store_configuration_path(const path& filePath);
    int init_openssl();

} // namespace fc
