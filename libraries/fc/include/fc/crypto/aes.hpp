#pragma once
#include <fc/crypto/sha512.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/uint128.hpp>
#include <fc/fwd.hpp>
#include <vector>

namespace fc {
    class path;

    class aes_encoder
    {
       public:
         aes_encoder();
         ~aes_encoder();
     
         void init( const fc::sha256& key, const fc::uint128& init_value );
         uint32_t encode( const char* plaintxt, uint32_t len, char* ciphertxt );
 //        uint32_t final_encode( char* ciphertxt );

       private:
         struct      impl;
         fc::fwd<impl,96> my;
    };
    class aes_decoder
    {
       public:
         aes_decoder();
         ~aes_decoder();
     
         void     init( const fc::sha256& key, const fc::uint128& init_value );
         uint32_t decode( const char* ciphertxt, uint32_t len, char* plaintext );
//         uint32_t final_decode( char* plaintext );

       private:
         struct      impl;
         fc::fwd<impl,96> my;
    };

    unsigned aes_encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                         unsigned char *iv, unsigned char *ciphertext);
    unsigned aes_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                         unsigned char *iv, unsigned char *plaintext);
    unsigned aes_cfb_decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                             unsigned char *iv, unsigned char *plaintext);

    std::vector<char> aes_encrypt( const fc::sha512& key, const std::vector<char>& plain_text  );
    std::vector<char> aes_decrypt( const fc::sha512& key, const std::vector<char>& cipher_text );

    /** encrypts plain_text and then includes a checksum that enables us to verify the integrety of
     * the file / key prior to decryption. 
     */
    void              aes_save( const fc::path& file, const fc::sha512& key, std::vector<char> plain_text );

    /**
     *  recovers the plain_text saved via aes_save()
     */
    std::vector<char> aes_load( const fc::path& file, const fc::sha512& key );

} // namespace fc 
