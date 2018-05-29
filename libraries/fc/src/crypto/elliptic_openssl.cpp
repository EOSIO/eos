#include <fc/crypto/elliptic.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/openssl.hpp>

#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

#include "_elliptic_impl_pub.hpp"

namespace fc { namespace ecc {
    namespace detail
    {
        void _init_lib() {
            static int init_o = init_openssl();
        }

        class private_key_impl
        {
            public:
                private_key_impl() BOOST_NOEXCEPT
                {
                    _init_lib();
                }

                private_key_impl( const private_key_impl& cpy ) BOOST_NOEXCEPT
                {
                    _init_lib();
                    *this = cpy;
                }

                private_key_impl( private_key_impl&& cpy ) BOOST_NOEXCEPT
                {
                    _init_lib();
                    *this = cpy;
                }

                ~private_key_impl() BOOST_NOEXCEPT
                {
                    free_key();
                }

                private_key_impl& operator=( const private_key_impl& pk ) BOOST_NOEXCEPT
                {
                    if (pk._key == nullptr)
                    {
                        free_key();
                    } else if ( _key == nullptr ) {
                        _key = EC_KEY_dup( pk._key );
                    } else {
                        EC_KEY_copy( _key, pk._key );
                    }
                    return *this;
                }

                private_key_impl& operator=( private_key_impl&& pk ) BOOST_NOEXCEPT
                {
                    if ( this != &pk ) {
                        free_key();
                        _key = pk._key;
                        pk._key = nullptr;
                    }
                    return *this;
                }

                EC_KEY* _key = nullptr;

            private:
                void free_key() BOOST_NOEXCEPT
                {
                    if( _key != nullptr )
                    {
                        EC_KEY_free(_key);
                        _key = nullptr;
                    }
                }
        };
    }

    private_key::private_key() {}

    private_key::private_key( const private_key& pk ) : my( pk.my ) {}

    private_key::private_key( private_key&& pk ) : my( std::move( pk.my ) ) {}

    private_key::~private_key() {}

    private_key& private_key::operator=( private_key&& pk )
    {
        my = std::move(pk.my);
        return *this;
    }

    private_key& private_key::operator=( const private_key& pk )
    {
        my = pk.my;
        return *this;
    }
    static void * ecies_key_derivation(const void *input, size_t ilen, void *output, size_t *olen)
    {
        if (*olen < SHA512_DIGEST_LENGTH) {
            return NULL;
        }
        *olen = SHA512_DIGEST_LENGTH;
        return (void*)SHA512((const unsigned char*)input, ilen, (unsigned char*)output);
    }

    int static inline EC_KEY_regenerate_key(EC_KEY *eckey, const BIGNUM *priv_key)
    {
        int ok = 0;
        BN_CTX *ctx = NULL;
        EC_POINT *pub_key = NULL;

        if (!eckey) return 0;

        const EC_GROUP *group = EC_KEY_get0_group(eckey);

        if ((ctx = BN_CTX_new()) == NULL)
        goto err;

        pub_key = EC_POINT_new(group);

        if (pub_key == NULL)
        goto err;

        if (!EC_POINT_mul(group, pub_key, priv_key, NULL, NULL, ctx))
        goto err;

        EC_KEY_set_private_key(eckey,priv_key);
        EC_KEY_set_public_key(eckey,pub_key);

        ok = 1;

        err:

        if (pub_key) EC_POINT_free(pub_key);
        if (ctx != NULL) BN_CTX_free(ctx);

        return(ok);
    }

    private_key private_key::regenerate( const fc::sha256& secret )
    {
        private_key self;
        self.my->_key = EC_KEY_new_by_curve_name( NID_secp256k1 );
        if( !self.my->_key ) FC_THROW_EXCEPTION( exception, "Unable to generate EC key" );

        ssl_bignum bn;
        BN_bin2bn( (const unsigned char*)&secret, 32, bn );

        if( !EC_KEY_regenerate_key(self.my->_key,bn) )
        {
            FC_THROW_EXCEPTION( exception, "unable to regenerate key" );
        }
        return self;
    }

    fc::sha256 private_key::get_secret()const
    {
        return get_secret( my->_key );
    }

    private_key::private_key( EC_KEY* k )
    {
        my->_key = k;
    }

    public_key private_key::get_public_key()const
    {
       public_key pub;
       pub.my->_key = EC_KEY_new_by_curve_name( NID_secp256k1 );
       EC_KEY_set_public_key( pub.my->_key, EC_KEY_get0_public_key( my->_key ) );
       return pub;
    }


    fc::sha512 private_key::get_shared_secret( const public_key& other )const
    {
      FC_ASSERT( my->_key != nullptr );
      FC_ASSERT( other.my->_key != nullptr );
      fc::sha512 buf;
      ECDH_compute_key( (unsigned char*)&buf, sizeof(buf), EC_KEY_get0_public_key(other.my->_key), my->_key, ecies_key_derivation );
      return buf;
    }

    compact_signature private_key::sign_compact( const fc::sha256& digest )const
    {
        try {
            FC_ASSERT( my->_key != nullptr );
            auto my_pub_key = get_public_key().serialize(); // just for good measure
            //ECDSA_SIG *sig = ECDSA_do_sign((unsigned char*)&digest, sizeof(digest), my->_key);
            public_key_data key_data;
            while( true )
            {
                ecdsa_sig sig = ECDSA_do_sign((unsigned char*)&digest, sizeof(digest), my->_key);

                if (sig==nullptr)
                    FC_THROW_EXCEPTION( exception, "Unable to sign" );

                compact_signature csig;
                // memset( csig.data, 0, sizeof(csig) );

                int nBitsR = BN_num_bits(sig->r);
                int nBitsS = BN_num_bits(sig->s);
                if (nBitsR <= 256 && nBitsS <= 256)
                {
                    int nRecId = -1;
                    EC_KEY* key = EC_KEY_new_by_curve_name( NID_secp256k1 );
                    FC_ASSERT( key );
                    EC_KEY_set_conv_form( key, POINT_CONVERSION_COMPRESSED );
                    for (int i=0; i<4; i++)
                    {
                        if (detail::public_key_impl::ECDSA_SIG_recover_key_GFp(key, sig, (unsigned char*)&digest, sizeof(digest), i, 1) == 1)
                        {
                            unsigned char* buffer = (unsigned char*) key_data.begin();
                            i2o_ECPublicKey( key, &buffer ); // FIXME: questionable memory handling
                            if ( key_data == my_pub_key )
                            {
                                nRecId = i;
                                break;
                            }
                        }
                    }
                    EC_KEY_free( key );

                    if (nRecId == -1)
                    {
                        FC_THROW_EXCEPTION( exception, "unable to construct recoverable key");
                    }
                    unsigned char* result = nullptr;
                    auto bytes = i2d_ECDSA_SIG( sig, &result );
                    auto lenR = result[3];
                    auto lenS = result[5+lenR];
                    //idump( (result[0])(result[1])(result[2])(result[3])(result[3+lenR])(result[4+lenR])(bytes)(lenR)(lenS) );
                    if( lenR != 32 ) { free(result); continue; }
                    if( lenS != 32 ) { free(result); continue; }
                    //idump( (33-(nBitsR+7)/8) );
                    //idump( (65-(nBitsS+7)/8) );
                    //idump( (sizeof(csig) ) );
                    memcpy( &csig.data[1], &result[4], lenR );
                    memcpy( &csig.data[33], &result[6+lenR], lenS );
                    //idump( (csig.data[33]) );
                    //idump( (csig.data[1]) );
                    free(result);
                    //idump( (nRecId) );
                    csig.data[0] = nRecId+27+4;//(fCompressedPubKey ? 4 : 0);
                    /*
                    idump( (csig) );
                    auto rlen = BN_bn2bin(sig->r,&csig.data[33-(nBitsR+7)/8]);
                    auto slen = BN_bn2bin(sig->s,&csig.data[65-(nBitsS+7)/8]);
                    idump( (rlen)(slen) );
                    */
                }
                return csig;
            } // while true
        } FC_RETHROW_EXCEPTIONS( warn, "sign ${digest}", ("digest", digest)("private_key",*this) );
    }
} }
