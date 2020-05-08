#pragma once
#include <fc/crypto/elliptic.hpp>
#include <boost/config.hpp>

/* public_key_impl implementation based on openssl
 * used by mixed + openssl
 */

namespace fc { namespace ecc { namespace detail {

void _init_lib();

class public_key_impl
{
    public:
        public_key_impl() BOOST_NOEXCEPT;
        public_key_impl( const public_key_impl& cpy ) BOOST_NOEXCEPT;
        public_key_impl( public_key_impl&& cpy ) BOOST_NOEXCEPT;
        ~public_key_impl() BOOST_NOEXCEPT;

        public_key_impl& operator=( const public_key_impl& pk ) BOOST_NOEXCEPT;

        public_key_impl& operator=( public_key_impl&& pk ) BOOST_NOEXCEPT;

        static int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const unsigned char *msg, int msglen, int recid, int check);

        EC_KEY* _key = nullptr;

    private:
        void free_key() BOOST_NOEXCEPT;
};

}}}
