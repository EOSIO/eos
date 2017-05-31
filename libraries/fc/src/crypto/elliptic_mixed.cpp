#include <fc/crypto/elliptic.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/openssl.hpp>

#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

#include <assert.h>
#include <secp256k1.h>

#include "_elliptic_impl_priv.hpp"
#include "_elliptic_impl_pub.hpp"

namespace fc { namespace ecc {
    namespace detail
    {
        const secp256k1_context_t* _get_context() {
            static secp256k1_context_t* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN);
            return ctx;
        }

        void _init_lib() {
            static const secp256k1_context_t* ctx = _get_context();
            static int init_o = init_openssl();
        }
    }

    static const private_key_secret empty_priv;
    fc::sha512 private_key::get_shared_secret( const public_key& other )const
    {
      FC_ASSERT( my->_key != empty_priv );
      FC_ASSERT( other.my->_key != nullptr );
      public_key_data pub(other.serialize());
      FC_ASSERT( secp256k1_ec_pubkey_tweak_mul( detail::_get_context(), (unsigned char*) pub.begin(), pub.size(), (unsigned char*) my->_key.data() ) );
      return fc::sha512::hash( pub.begin() + 1, pub.size() - 1 );
    }

} }
