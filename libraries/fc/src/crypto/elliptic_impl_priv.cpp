#include <fc/fwd_impl.hpp>

#include <secp256k1.h>

#include "_elliptic_impl_priv.hpp"

/* used by mixed + secp256k1 */

namespace fc { namespace ecc {
    namespace detail {

        private_key_impl::private_key_impl() BOOST_NOEXCEPT
        {
            _init_lib();
        }

        private_key_impl::private_key_impl( const private_key_impl& cpy ) BOOST_NOEXCEPT
        {
            _init_lib();
            this->_key = cpy._key;
        }

        private_key_impl& private_key_impl::operator=( const private_key_impl& pk ) BOOST_NOEXCEPT
        {
            _key = pk._key;
            return *this;
        }
    }

    static const private_key_secret empty_priv;

    private_key::private_key() {}

    private_key::private_key( const private_key& pk ) : my( pk.my ) {}

    private_key::private_key( private_key&& pk ) : my( std::move( pk.my ) ) {}

    private_key::~private_key() {}

    private_key& private_key::operator=( private_key&& pk )
    {
        my = std::move( pk.my );
        return *this;
    }

    private_key& private_key::operator=( const private_key& pk )
    {
        my = pk.my;
        return *this;
    }

    private_key private_key::regenerate( const fc::sha256& secret )
    {
       private_key self;
       self.my->_key = secret;
       return self;
    }

    fc::sha256 private_key::get_secret()const
    {
        return my->_key;
    }

    private_key::private_key( EC_KEY* k )
    {
       my->_key = get_secret( k );
       EC_KEY_free(k);
    }

    public_key private_key::get_public_key()const
    {
       FC_ASSERT( my->_key != empty_priv );
       public_key_data pub;
       unsigned int pk_len;
       FC_ASSERT( secp256k1_ec_pubkey_create( detail::_get_context(), (unsigned char*) pub.begin(), (int*) &pk_len, (unsigned char*) my->_key.data(), 1 ) );
       FC_ASSERT( pk_len == pub.size() );
       return public_key(pub);
    }

    static int extended_nonce_function( unsigned char *nonce32, const unsigned char *msg32,
                                        const unsigned char *key32, unsigned int attempt,
                                        const void *data ) {
        unsigned int* extra = (unsigned int*) data;
        (*extra)++;
        return secp256k1_nonce_function_default( nonce32, msg32, key32, *extra, nullptr );
    }

    compact_signature private_key::sign_compact( const fc::sha256& digest, bool require_canonical )const
    {
        FC_ASSERT( my->_key != empty_priv );
        compact_signature result;
        int recid;
        unsigned int counter = 0;
        do
        {
            FC_ASSERT( secp256k1_ecdsa_sign_compact( detail::_get_context(), (unsigned char*) digest.data(), (unsigned char*) result.begin() + 1, (unsigned char*) my->_key.data(), extended_nonce_function, &counter, &recid ));
        } while( require_canonical && !public_key::is_canonical( result ) );
        result.begin()[0] = 27 + 4 + recid;
        return result;
    }

}}
