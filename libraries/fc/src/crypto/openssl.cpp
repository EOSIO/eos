#include <fc/crypto/openssl.hpp>

#include <fc/filesystem.hpp>

#include <boost/filesystem/path.hpp>

#include <cstdlib>
#include <string>
#include <stdlib.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L
void ECDSA_SIG_get0(const ECDSA_SIG *sig, const BIGNUM **pr, const BIGNUM **ps) {
   if (pr != NULL)
      *pr = sig->r;
   if (ps != NULL)
      *ps = sig->s;
 }

 int ECDSA_SIG_set0(ECDSA_SIG *sig, BIGNUM *r, BIGNUM *s) {
   if (r == NULL || s == NULL)
      return 0;
   BN_clear_free(sig->r);
   BN_clear_free(sig->s);
   sig->r = r;
   sig->s = s;
   return 1;
}
#endif

namespace  fc 
{
    struct openssl_scope
    {
       static path& config_path() {
          static path cfg;
          return cfg;
       }
       openssl_scope()
       {
          ERR_load_crypto_strings(); 
          OpenSSL_add_all_algorithms();

          const boost::filesystem::path& boostPath = config_path();
          if(boostPath.empty() == false)
          {
            std::string varSetting("OPENSSL_CONF=");
            varSetting += config_path().to_native_ansi_path();
#if defined(WIN32)
            _putenv((char*)varSetting.c_str());
#else
            putenv((char*)varSetting.c_str());
#endif
          }

          OPENSSL_config(nullptr);
       }

       ~openssl_scope()
       {
          EVP_cleanup();
          ERR_free_strings();
       }
    };


    void store_configuration_path(const path& filePath)
    {
      openssl_scope::config_path() = filePath;
    }
   
    int init_openssl()
    {
      static openssl_scope ossl;
      return 0;
    }
}
