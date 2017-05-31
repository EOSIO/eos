#include <fc/crypto/aes.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/fwd_impl.hpp>

//#include <fc/io/fstream.hpp>
#include <fc/io/raw.hpp>

#include <fc/log/logger.hpp>

#include <boost/thread/mutex.hpp>
#include <openssl/opensslconf.h>
#ifndef OPENSSL_THREADS
# error "OpenSSL must be configured to support threads"
#endif
#include <openssl/crypto.h>

#if defined(_WIN32)
# include <windows.h>
#endif

#include <thread>
#include <fstream>
#include <functional>

namespace fc {

struct aes_encoder::impl 
{
   evp_cipher_ctx ctx;
};

aes_encoder::aes_encoder()
{
  static int init = init_openssl();
  (void)init;
}

aes_encoder::~aes_encoder()
{
}

void aes_encoder::init( const fc::sha256& key, const fc::uint128& init_value )
{
    my->ctx.obj = EVP_CIPHER_CTX_new();
    /* Create and initialise the context */
    if(!my->ctx)
    {
        FC_THROW_EXCEPTION( aes_exception, "error allocating evp cipher context", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    *    and IV size appropriate for your cipher
    *    In this example we are using 256 bit AES (i.e. a 256 bit key). The
    *    IV size for *most* modes is the same as the block size. For AES this
    *    is 128 bits */
    if(1 != EVP_EncryptInit_ex(my->ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)&key, (unsigned char*)&init_value))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc encryption init", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    EVP_CIPHER_CTX_set_padding( my->ctx, 0 );
}

uint32_t aes_encoder::encode( const char* plaintxt, uint32_t plaintext_len, char* ciphertxt )
{
    int ciphertext_len = 0;
    /* Provide the message to be encrypted, and obtain the encrypted output.
    *    * EVP_EncryptUpdate can be called multiple times if necessary
    *       */
    if(1 != EVP_EncryptUpdate(my->ctx, (unsigned char*)ciphertxt, &ciphertext_len, (const unsigned char*)plaintxt, plaintext_len))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc encryption update", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    FC_ASSERT( ciphertext_len == plaintext_len, "", ("ciphertext_len",ciphertext_len)("plaintext_len",plaintext_len) );
    return ciphertext_len;
}
#if 0
uint32_t aes_encoder::final_encode( char* ciphertxt )
{
    int ciphertext_len = 0;
    /* Finalise the encryption. Further ciphertext bytes may be written at
    *    * this stage.
    *       */
    if(1 != EVP_EncryptFinal_ex(my->ctx, (unsigned char*)ciphertxt, &ciphertext_len)) 
    {
        FC_THROW_EXCEPTION( exception, "error during aes 256 cbc encryption final", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    return ciphertext_len;
}
#endif


struct aes_decoder::impl 
{
   evp_cipher_ctx ctx;
};

aes_decoder::aes_decoder()
{
  static int init = init_openssl();
  (void)init;
}

void aes_decoder::init( const fc::sha256& key, const fc::uint128& init_value )
{
    my->ctx.obj = EVP_CIPHER_CTX_new();
    /* Create and initialise the context */
    if(!my->ctx)
    {
        FC_THROW_EXCEPTION( aes_exception, "error allocating evp cipher context", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    *    and IV size appropriate for your cipher
    *    In this example we are using 256 bit AES (i.e. a 256 bit key). The
    *    IV size for *most* modes is the same as the block size. For AES this
    *    is 128 bits */
    if(1 != EVP_DecryptInit_ex(my->ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)&key, (unsigned char*)&init_value))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc encryption init", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    EVP_CIPHER_CTX_set_padding( my->ctx, 0 );
}
aes_decoder::~aes_decoder()
{
}

uint32_t aes_decoder::decode( const char* ciphertxt, uint32_t ciphertxt_len, char* plaintext )
{
    int plaintext_len = 0;
    /* Provide the message to be decrypted, and obtain the decrypted output.
    *    * EVP_DecryptUpdate can be called multiple times if necessary
    *       */
	if (1 != EVP_DecryptUpdate(my->ctx, (unsigned char*)plaintext, &plaintext_len, (const unsigned char*)ciphertxt, ciphertxt_len))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decryption update", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    FC_ASSERT( ciphertxt_len == plaintext_len, "", ("ciphertxt_len",ciphertxt_len)("plaintext_len",plaintext_len) );
	return plaintext_len;
}
#if 0
uint32_t aes_decoder::final_decode( char* plaintext )
{
    return 0;
    int ciphertext_len = 0;
    /* Finalise the encryption. Further ciphertext bytes may be written at
    *    * this stage.
    *       */
    if(1 != EVP_DecryptFinal_ex(my->ctx, (unsigned char*)plaintext, &ciphertext_len)) 
    {
        FC_THROW_EXCEPTION( exception, "error during aes 256 cbc encryption final", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    return ciphertext_len;
}
#endif












/** example method from wiki.opensslfoundation.com */
unsigned aes_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                     unsigned char *iv, unsigned char *ciphertext)
{
    evp_cipher_ctx ctx( EVP_CIPHER_CTX_new() );

    int len = 0;
    unsigned ciphertext_len = 0;

    /* Create and initialise the context */
    if(!ctx)
    {
        FC_THROW_EXCEPTION( aes_exception, "error allocating evp cipher context", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Initialise the encryption operation. IMPORTANT - ensure you use a key
    *    and IV size appropriate for your cipher
    *    In this example we are using 256 bit AES (i.e. a 256 bit key). The
    *    IV size for *most* modes is the same as the block size. For AES this
    *    is 128 bits */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc encryption init", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Provide the message to be encrypted, and obtain the encrypted output.
    *    * EVP_EncryptUpdate can be called multiple times if necessary
    *       */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc encryption update", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    ciphertext_len = len;

    /* Finalise the encryption. Further ciphertext bytes may be written at
    *    * this stage.
    *       */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) 
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc encryption final", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    ciphertext_len += len;

    return ciphertext_len;
}

unsigned aes_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                     unsigned char *iv, unsigned char *plaintext)
{
    evp_cipher_ctx ctx( EVP_CIPHER_CTX_new() );
    int len = 0;
    unsigned plaintext_len = 0;

    /* Create and initialise the context */
    if(!ctx) 
    {
        FC_THROW_EXCEPTION( aes_exception, "error allocating evp cipher context", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    *    * and IV size appropriate for your cipher
    *       * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    *          * IV size for *most* modes is the same as the block size. For AES this
    *             * is 128 bits */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decrypt init", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    *    * EVP_DecryptUpdate can be called multiple times if necessary
    *       */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decrypt update", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    *    * this stage.
    *       */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) 
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decrypt final", 
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    plaintext_len += len;

    return plaintext_len;
}

unsigned aes_cfb_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                         unsigned char *iv, unsigned char *plaintext)
{
    evp_cipher_ctx ctx( EVP_CIPHER_CTX_new() );
    int len = 0;
    unsigned plaintext_len = 0;

    /* Create and initialise the context */
    if(!ctx)
    {
        FC_THROW_EXCEPTION( aes_exception, "error allocating evp cipher context",
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Initialise the decryption operation. IMPORTANT - ensure you use a key
    *    * and IV size appropriate for your cipher
    *       * In this example we are using 256 bit AES (i.e. a 256 bit key). The
    *          * IV size for *most* modes is the same as the block size. For AES this
    *             * is 128 bits */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cfb128(), NULL, key, iv))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decrypt init",
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    /* Provide the message to be decrypted, and obtain the plaintext output.
    *    * EVP_DecryptUpdate can be called multiple times if necessary
    *       */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decrypt update",
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }

    plaintext_len = len;

    /* Finalise the decryption. Further plaintext bytes may be written at
    *    * this stage.
    *       */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
    {
        FC_THROW_EXCEPTION( aes_exception, "error during aes 256 cbc decrypt final",
                           ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
    }
    plaintext_len += len;

    return plaintext_len;
}

std::vector<char> aes_encrypt( const fc::sha512& key, const std::vector<char>& plain_text  )
{
    std::vector<char> cipher_text(plain_text.size()+16);
    auto cipher_len = aes_encrypt( (unsigned char*)plain_text.data(), (int)plain_text.size(),  
                                   (unsigned char*)&key, ((unsigned char*)&key)+32,
                                   (unsigned char*)cipher_text.data() );
    FC_ASSERT( cipher_len <= cipher_text.size() );
    cipher_text.resize(cipher_len);
    return cipher_text;

}
std::vector<char> aes_decrypt( const fc::sha512& key, const std::vector<char>& cipher_text )
{
    std::vector<char> plain_text( cipher_text.size() );
    auto plain_len = aes_decrypt( (unsigned char*)cipher_text.data(), (int)cipher_text.size(),  
                                 (unsigned char*)&key, ((unsigned char*)&key)+32,
                                 (unsigned char*)plain_text.data() );
    plain_text.resize(plain_len);
    return plain_text;
}


/** encrypts plain_text and then includes a checksum that enables us to verify the integrety of
 * the file / key prior to decryption. 
 */
void              aes_save( const fc::path& file, const fc::sha512& key, std::vector<char> plain_text )
{ try {
   auto cipher = aes_encrypt( key, plain_text );
   fc::sha512::encoder check_enc;
   fc::raw::pack( check_enc, key );
   fc::raw::pack( check_enc, cipher );
   auto check = check_enc.result();

   std::ofstream out(file.generic_string().c_str());
   fc::raw::pack( out, check );
   fc::raw::pack( out, cipher );
} FC_RETHROW_EXCEPTIONS( warn, "", ("file",file) ) }

/**
 *  recovers the plain_text saved via aes_save()
 */
std::vector<char> aes_load( const fc::path& file, const fc::sha512& key )
{ try {
   FC_ASSERT( fc::exists( file ) );

   std::ifstream in( file.generic_string().c_str(), std::ifstream::binary );
   fc::sha512 check;
   std::vector<char> cipher;

   fc::raw::unpack( in, check );
   fc::raw::unpack( in, cipher );

   fc::sha512::encoder check_enc;
   fc::raw::pack( check_enc, key );
   fc::raw::pack( check_enc, cipher );

   FC_ASSERT( check_enc.result() == check );

   return aes_decrypt( key, cipher );
} FC_RETHROW_EXCEPTIONS( warn, "", ("file",file) ) }

/* This stuff has to go somewhere, I guess this is as good a place as any...
  OpenSSL isn't thread-safe unless you give it access to some mutexes,
  so the CRYPTO_set_id_callback() function needs to be called before there's any
  chance of OpenSSL being accessed from multiple threads.
*/
struct openssl_thread_config
{
  static boost::mutex* openssl_mutexes;
  static unsigned long get_thread_id();
  static void locking_callback(int mode, int type, const char *file, int line);
  openssl_thread_config();
  ~openssl_thread_config();
};
openssl_thread_config openssl_thread_config_manager;

boost::mutex*         openssl_thread_config::openssl_mutexes = nullptr;

unsigned long openssl_thread_config::get_thread_id()
{
#ifdef _WIN32
  return (unsigned long)::GetCurrentThreadId();
#else
  return std::hash<std::thread::id>()(std::this_thread::get_id());
//  return (unsigned long)(&fc::thread::current());    // TODO: should expose boost thread id
#endif
}

void openssl_thread_config::locking_callback(int mode, int type, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    openssl_mutexes[type].lock();
  else
    openssl_mutexes[type].unlock();
}

// Warning: Things get complicated if third-party libraries also try to install their their own 
// OpenSSL thread functions.  Right now, we don't install our own handlers if another library has
// installed them before us which is a partial solution, but you'd really need to evaluate
// each library that does this to make sure they will play nice.
openssl_thread_config::openssl_thread_config()
{
  if (CRYPTO_get_id_callback() == NULL &&
      CRYPTO_get_locking_callback() == NULL)
  {
    openssl_mutexes = new boost::mutex[CRYPTO_num_locks()];
    CRYPTO_set_id_callback(&get_thread_id);
    CRYPTO_set_locking_callback(&locking_callback);
  }
}
openssl_thread_config::~openssl_thread_config()
{
  if (CRYPTO_get_id_callback() == &get_thread_id)
  {
    CRYPTO_set_id_callback(NULL);
    CRYPTO_set_locking_callback(NULL);
    delete[] openssl_mutexes;
    openssl_mutexes = nullptr;
  }
}

}  // namespace fc
