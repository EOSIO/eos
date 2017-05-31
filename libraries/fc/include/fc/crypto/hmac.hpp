/* 
 * File:   hmac.hpp
 * Author: Peter Conrad
 *
 * Created on 1. Juli 2015, 21:48
 */

#ifndef HMAC_HPP
#define	HMAC_HPP

#include <fc/crypto/sha224.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha512.hpp>

namespace fc {

    template<typename H>
    class hmac
    {
        public:
            hmac() {}

            H digest( const char* c, uint32_t c_len, const char* d, uint32_t d_len )
            {
                encoder.reset();
                add_key(c, c_len, 0x36);
                encoder.write( d, d_len );
                H intermediate = encoder.result();

                encoder.reset();
                add_key(c, c_len, 0x5c);
                encoder.write( intermediate.data(), intermediate.data_size() );
                return encoder.result();
            }

        private:
            void add_key( const char* c, const uint32_t c_len, char pad )
            {
                if ( c_len > internal_block_size() )
                {
                    H hash = H::hash( c, c_len );
                    add_key( hash.data(), hash.data_size(), pad );
                }
                else
                    for (unsigned int i = 0; i < internal_block_size(); i++ )
                    {
                        encoder.put( pad ^ ((i < c_len) ? *c++ : 0) );
                    }
            }

            unsigned int internal_block_size() const;

            H dummy;
            typename H::encoder encoder;
    };

    typedef hmac<fc::sha224> hmac_sha224;
    typedef hmac<fc::sha256> hmac_sha256;
    typedef hmac<fc::sha512> hmac_sha512;
}

#endif	/* HMAC_HPP */

