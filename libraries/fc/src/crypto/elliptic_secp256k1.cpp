#include <fc/crypto/elliptic.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/hmac.hpp>
#include <fc/crypto/openssl.hpp>
#include <fc/crypto/sha512.hpp>

#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

#include <assert.h>
#include <secp256k1.h>

#if _WIN32
# include <malloc.h>
#else
# include <alloca.h>
#endif

#include "_elliptic_impl_priv.hpp"

namespace fc { namespace ecc {
    namespace detail
    {
        const secp256k1_context_t* _get_context() {
            static secp256k1_context_t* ctx = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY | SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_RANGEPROOF | SECP256K1_CONTEXT_COMMIT );
            return ctx;
        }

        void _init_lib() {
            static const secp256k1_context_t* ctx = _get_context();
            static int init_o = init_openssl();
            (void)ctx;
            (void)init_o;
        }

        class public_key_impl
        {
            public:
                public_key_impl() BOOST_NOEXCEPT
                {
                    _init_lib();
                }

                public_key_impl( const public_key_impl& cpy ) BOOST_NOEXCEPT
                    : _key( cpy._key )
                {
                    _init_lib();
                }

                public_key_data _key;
        };

        typedef fc::array<char,37> chr37;
        chr37 _derive_message( const public_key_data& key, int i );
        fc::sha256 _left( const fc::sha512& v );
        fc::sha256 _right( const fc::sha512& v );
        const ec_group& get_curve();
        const private_key_secret& get_curve_order();
        const private_key_secret& get_half_curve_order();
    }

    static const public_key_data empty_pub;
    static const private_key_secret empty_priv;

    fc::sha512 private_key::get_shared_secret( const public_key& other )const
    {
      FC_ASSERT( my->_key != empty_priv );
      FC_ASSERT( other.my->_key != empty_pub );
      public_key_data pub(other.my->_key);
      FC_ASSERT( secp256k1_ec_pubkey_tweak_mul( detail::_get_context(), (unsigned char*) pub.begin(), pub.size(), (unsigned char*) my->_key.data() ) );
      return fc::sha512::hash( pub.begin() + 1, pub.size() - 1 );
    }


    public_key::public_key() {}

    public_key::public_key( const public_key &pk ) : my( pk.my ) {}

    public_key::public_key( public_key &&pk ) : my( std::move( pk.my ) ) {}

    public_key::~public_key() {}

    public_key& public_key::operator=( const public_key& pk )
    {
        my = pk.my;
        return *this;
    }

    public_key& public_key::operator=( public_key&& pk )
    {
        my = pk.my;
        return *this;
    }

    bool public_key::valid()const
    {
      return my->_key != empty_pub;
    }

    public_key public_key::add( const fc::sha256& digest )const
    {
        FC_ASSERT( my->_key != empty_pub );
        public_key_data new_key;
        memcpy( new_key.begin(), my->_key.begin(), new_key.size() );
        FC_ASSERT( secp256k1_ec_pubkey_tweak_add( detail::_get_context(), (unsigned char*) new_key.begin(), new_key.size(), (unsigned char*) digest.data() ) );
        return public_key( new_key );
    }

    std::string public_key::to_base58() const
    {
        FC_ASSERT( my->_key != empty_pub );
        return to_base58( my->_key );
    }

    public_key_data public_key::serialize()const
    {
        FC_ASSERT( my->_key != empty_pub );
        return my->_key;
    }

    public_key_point_data public_key::serialize_ecc_point()const
    {
        FC_ASSERT( my->_key != empty_pub );
        public_key_point_data dat;
        unsigned int pk_len = my->_key.size();
        memcpy( dat.begin(), my->_key.begin(), pk_len );
        FC_ASSERT( secp256k1_ec_pubkey_decompress( detail::_get_context(), (unsigned char *) dat.begin(), (int*) &pk_len ) );
        FC_ASSERT( pk_len == dat.size() );
        return dat;
    }

    public_key::public_key( const public_key_point_data& dat )
    {
        const char* front = &dat.data[0];
        if( *front == 0 ){}
        else
        {
            EC_KEY *key = EC_KEY_new_by_curve_name( NID_secp256k1 );
            key = o2i_ECPublicKey( &key, (const unsigned char**)&front, sizeof(dat) );
            FC_ASSERT( key );
            EC_KEY_set_conv_form( key, POINT_CONVERSION_COMPRESSED );
            unsigned char* buffer = (unsigned char*) my->_key.begin();
            i2o_ECPublicKey( key, &buffer ); // FIXME: questionable memory handling
            EC_KEY_free( key );
        }
    }

    public_key::public_key( const public_key_data& dat )
    {
        my->_key = dat;
    }

    public_key::public_key( const compact_signature& c, const fc::sha256& digest, bool check_canonical )
    {
        int nV = c.data[0];
        if (nV<27 || nV>=35)
            FC_THROW_EXCEPTION( exception, "unable to reconstruct public key from signature" );

        if( check_canonical )
        {
            FC_ASSERT( is_canonical( c ), "signature is not canonical" );
        }

        unsigned int pk_len;
        FC_ASSERT( secp256k1_ecdsa_recover_compact( detail::_get_context(), (unsigned char*) digest.data(), (unsigned char*) c.begin() + 1, (unsigned char*) my->_key.begin(), (int*) &pk_len, 1, (*c.begin() - 27) & 3 ) );
        FC_ASSERT( pk_len == my->_key.size() );
    }


     commitment_type blind( const blind_factor_type& blind, uint64_t value )
     {
        commitment_type result;
        FC_ASSERT( secp256k1_pedersen_commit( detail::_get_context(), (unsigned char*)&result, (unsigned char*)&blind, value ) );
        return result;
     }

     blind_factor_type blind_sum( const std::vector<blind_factor_type>& blinds_in, uint32_t non_neg )
     {
        blind_factor_type result;
        std::vector<const unsigned char*> blinds(blinds_in.size());
        for( uint32_t i = 0; i < blinds_in.size(); ++i ) blinds[i] = (const unsigned char*)&blinds_in[i];
        FC_ASSERT( secp256k1_pedersen_blind_sum( detail::_get_context(), (unsigned char*)&result, blinds.data(), blinds_in.size(), non_neg ) );
        return result;
     }

     /**  verifies taht commnits + neg_commits + excess == 0 */
     bool            verify_sum( const std::vector<commitment_type>& commits_in, const std::vector<commitment_type>& neg_commits_in, int64_t excess )
     {
        std::vector<const unsigned char*> commits(commits_in.size());
        for( uint32_t i = 0; i < commits_in.size(); ++i ) commits[i] = (const unsigned char*)&commits_in[i];
        std::vector<const unsigned char*> neg_commits(neg_commits_in.size());
        for( uint32_t i = 0; i < neg_commits_in.size(); ++i ) neg_commits[i] = (const unsigned char*)&neg_commits_in[i];

        return secp256k1_pedersen_verify_tally( detail::_get_context(), commits.data(), commits.size(), neg_commits.data(), neg_commits.size(), excess  );
     }

     bool            verify_range( uint64_t& min_val, uint64_t& max_val, const commitment_type& commit, const std::vector<char>& proof )
     {
        return secp256k1_rangeproof_verify( detail::_get_context(), &min_val, &max_val, (const unsigned char*)&commit, (const unsigned char*)proof.data(), proof.size() );
     }

     std::vector<char>    range_proof_sign( uint64_t min_value, 
                                       const commitment_type& commit, 
                                       const blind_factor_type& commit_blind, 
                                       const blind_factor_type& nonce,
                                       int8_t base10_exp,
                                       uint8_t min_bits,
                                       uint64_t actual_value
                                     )
     {
        int proof_len = 5134; 
        std::vector<char> proof(proof_len);

        FC_ASSERT( secp256k1_rangeproof_sign( detail::_get_context(), 
                                              (unsigned char*)proof.data(), 
                                              &proof_len, min_value, 
                                              (const unsigned char*)&commit, 
                                              (const unsigned char*)&commit_blind, 
                                              (const unsigned char*)&nonce, 
                                              base10_exp, min_bits, actual_value ) );
        proof.resize(proof_len);
        return proof;
     }


     bool            verify_range_proof_rewind( blind_factor_type& blind_out,
                                                uint64_t& value_out,
                                                string& message_out, 
                                                const blind_factor_type& nonce,
                                                uint64_t& min_val, 
                                                uint64_t& max_val, 
                                                commitment_type commit, 
                                                const std::vector<char>& proof )
     {
        char msg[4096];
        int  mlen = 0;
        FC_ASSERT( secp256k1_rangeproof_rewind( detail::_get_context(), 
                                                (unsigned char*)&blind_out,
                                                &value_out,
                                                (unsigned char*)msg,
                                                &mlen,
                                                (const unsigned char*)&nonce,
                                                &min_val,
                                                &max_val,
                                                (const unsigned char*)&commit,
                                                (const unsigned char*)proof.data(),
                                                proof.size() ) );

        message_out = std::string( msg, mlen );
        return true;
     }

     range_proof_info range_get_info( const std::vector<char>& proof )
     {
        range_proof_info result;
        FC_ASSERT( secp256k1_rangeproof_info( detail::_get_context(), 
                                              (int*)&result.exp, 
                                              (int*)&result.mantissa, 
                                              (uint64_t*)&result.min_value, 
                                              (uint64_t*)&result.max_value, 
                                              (const unsigned char*)proof.data(), 
                                              (int)proof.size() ) );

        return result;
     }


} }
