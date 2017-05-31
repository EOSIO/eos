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

    extended_public_key::extended_public_key( const public_key& k, const fc::sha256& c,
                                              int child, int parent, uint8_t depth )
        : public_key(k), c(c), child_num(child), parent_fp(parent), depth(depth) { }

    extended_public_key extended_public_key::derive_normal_child(int i) const
    {
        hmac_sha512 mac;
        public_key_data key = serialize();
        const detail::chr37 data = detail::_derive_message( key, i );
        fc::sha512 l = mac.digest( c.data(), c.data_size(), data.begin(), data.size() );
        fc::sha256 left = detail::_left(l);
        FC_ASSERT( left < detail::get_curve_order() );
        FC_ASSERT( secp256k1_ec_pubkey_tweak_add( detail::_get_context(), (unsigned char*) key.begin(), key.size(), (unsigned char*) left.data() ) > 0 );
        // FIXME: check validity - if left + key == infinity then invalid
        extended_public_key result( key, detail::_right(l), i, fingerprint(), depth + 1 );
        return result;
    }

    static void to_bignum( const unsigned char* in, ssl_bignum& out, unsigned int len )
    {
        if ( *in & 0x80 )
        {
            unsigned char *buffer = (unsigned char*)alloca(len + 1);
            *buffer = 0;
            memcpy( buffer + 1, in, len );
            BN_bin2bn( buffer, sizeof(buffer), out );
        }
        else
        {
            BN_bin2bn( in, len, out );
        }
    }

    static void to_bignum( const private_key_secret& in, ssl_bignum& out )
    {
        to_bignum( (unsigned char*) in.data(), out, in.data_size() );
    }

    static void from_bignum( const ssl_bignum& in, unsigned char* out, unsigned int len )
    {
        unsigned int l = BN_num_bytes( in );
        if ( l > len )
        {
            unsigned char *buffer = (unsigned char*)alloca(l);
            BN_bn2bin( in, buffer );
            memcpy( out, buffer + l - len, len );
        }
        else
        {
            memset( out, 0, len - l );
            BN_bn2bin( in, out + len - l );
        }
    }

    static void from_bignum( const ssl_bignum& in, private_key_secret& out )
    {
        from_bignum( in, (unsigned char*) out.data(), out.data_size() );
    }

    static void invert( const private_key_secret& in, private_key_secret& out )
    {
        ssl_bignum bn_in;
        to_bignum( in, bn_in );
        ssl_bignum bn_n;
        to_bignum( detail::get_curve_order(), bn_n );
        ssl_bignum bn_inv;
        bn_ctx ctx( BN_CTX_new() );
        FC_ASSERT( BN_mod_inverse( bn_inv, bn_in, bn_n, ctx ) );
        from_bignum( bn_inv, out );
    }

    static void to_point( const public_key_data& in, ec_point& out )
    {
        bn_ctx ctx( BN_CTX_new() );
        const ec_group& curve = detail::get_curve();
        private_key_secret x;
        memcpy( x.data(), in.begin() + 1, x.data_size() );
        ssl_bignum bn_x;
        to_bignum( x, bn_x );
        FC_ASSERT( EC_POINT_set_compressed_coordinates_GFp( curve, out, bn_x, *in.begin() & 1, ctx ) > 0 );
    }

    static void from_point( const ec_point& in, public_key_data& out )
    {
        bn_ctx ctx( BN_CTX_new() );
        const ec_group& curve = detail::get_curve();
        ssl_bignum bn_x;
        ssl_bignum bn_y;
        FC_ASSERT( EC_POINT_get_affine_coordinates_GFp( curve, in, bn_x, bn_y, ctx ) > 0 );
        private_key_secret x;
        from_bignum( bn_x, x );
        memcpy( out.begin() + 1, x.data(), out.size() - 1 );
        *out.begin() = BN_is_bit_set( bn_y, 0 ) ? 3 : 2;
    }

//    static void print(const unsigned char* data) {
//        for (int i = 0; i < 32; i++) {
//            printf("%02x", *data++);
//        }
//    }
//
//    static void print(private_key_secret key) {
//        print((unsigned char*) key.data());
//    }
//
//    static void print(public_key_data key) {
//        print((unsigned char*) key.begin() + 1);
//    }

    static void canonicalize( unsigned char *int256 )
    {
        fc::sha256 biggi( (char*) int256, 32 );
        if ( detail::get_half_curve_order() >= biggi )
        {
            return; // nothing to do
        }
        ssl_bignum bn_k;
        to_bignum( int256, bn_k, 32 );
        ssl_bignum bn_n;
        to_bignum( detail::get_curve_order(), bn_n );
        FC_ASSERT( BN_sub( bn_k, bn_n, bn_k ) );
        from_bignum( bn_k, int256, 32 );
    }

    static public_key compute_k( const private_key_secret& a, const private_key_secret& c,
                                 const public_key& p )
    {
        private_key_secret prod = a;
        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) prod.data(), (unsigned char*) c.data() ) > 0 );
        invert( prod, prod );
        public_key_data P = p.serialize();
        FC_ASSERT( secp256k1_ec_pubkey_tweak_mul( detail::_get_context(), (unsigned char*) P.begin(), P.size(), (unsigned char*) prod.data() ) );
//        printf("K: "); print(P); printf("\n");
        return public_key( P );
    }

    static public_key compute_t( const private_key_secret& a, const private_key_secret& b,
                                 const private_key_secret& c, const private_key_secret& d,
                                 const public_key_data& p, const public_key_data& q )
    {
        private_key_secret prod;
        invert( c, prod ); // prod == c^-1
        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) prod.data(), (unsigned char*) d.data() ) > 0 );
        // prod == c^-1 * d

        public_key_data accu = p;
        FC_ASSERT( secp256k1_ec_pubkey_tweak_mul( detail::_get_context(), (unsigned char*) accu.begin(), accu.size(), (unsigned char*) prod.data() ) );
        // accu == prod * P == c^-1 * d * P

        ec_point point_accu( EC_POINT_new( detail::get_curve() ) );
        to_point( accu, point_accu );
        ec_point point_q( EC_POINT_new( detail::get_curve() ) );
        to_point( q, point_q );
        bn_ctx ctx(BN_CTX_new());
        FC_ASSERT( EC_POINT_add( detail::get_curve(), point_accu, point_accu, point_q, ctx ) > 0 );
        from_point( point_accu, accu );
        // accu == c^-1 * a * P + Q

        FC_ASSERT( secp256k1_ec_pubkey_tweak_add( detail::_get_context(), (unsigned char*) accu.begin(), accu.size(), (unsigned char*) b.data() ) );
        // accu == c^-1 * a * P + Q + b*G

        public_key_data k = compute_k( a, c, p ).serialize();
        memcpy( prod.data(), k.begin() + 1, prod.data_size() );
        // prod == Kx
        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) prod.data(), (unsigned char*) a.data() ) > 0 );
        // prod == Kx * a
        invert( prod, prod );
        // prod == (Kx * a)^-1

        FC_ASSERT( secp256k1_ec_pubkey_tweak_mul( detail::_get_context(), (unsigned char*) accu.begin(), accu.size(), (unsigned char*) prod.data() ) );
        // accu == (c^-1 * a * P + Q + b*G) * (Kx * a)^-1

//        printf("T: "); print(accu); printf("\n");
        return public_key( accu );
    }

    extended_private_key::extended_private_key( const private_key& k, const sha256& c,
                                                int child, int parent, uint8_t depth )
        : private_key(k), c(c), child_num(child), parent_fp(parent), depth(depth) { }

    extended_private_key extended_private_key::private_derive_rest( const fc::sha512& hash,
                                                                    int i) const
    {
        fc::sha256 left = detail::_left(hash);
        FC_ASSERT( left < detail::get_curve_order() );
        FC_ASSERT( secp256k1_ec_privkey_tweak_add( detail::_get_context(), (unsigned char*) left.data(), (unsigned char*) get_secret().data() ) > 0 );
        extended_private_key result( private_key::regenerate( left ), detail::_right(hash),
                                     i, fingerprint(), depth + 1 );
        return result;
    }

    public_key extended_private_key::blind_public_key( const extended_public_key& bob, int i ) const
    {
        private_key_secret a = generate_a(i).get_secret();
        private_key_secret b = generate_b(i).get_secret();
        private_key_secret c = generate_c(i).get_secret();
        private_key_secret d = generate_d(i).get_secret();
        public_key_data p = bob.generate_p(i).serialize();
        public_key_data q = bob.generate_q(i).serialize();
//        printf("a: "); print(a); printf("\n");
//        printf("b: "); print(b); printf("\n");
//        printf("c: "); print(c); printf("\n");
//        printf("d: "); print(d); printf("\n");
//        printf("P: "); print(p); printf("\n");
//        printf("Q: "); print(q); printf("\n");
        return compute_t( a, b, c, d, p, q );
    }

    blinded_hash extended_private_key::blind_hash( const fc::sha256& hash, int i ) const
    {
        private_key_secret a = generate_a(i).get_secret();
        private_key_secret b = generate_b(i).get_secret();
        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) a.data(), (unsigned char*) hash.data() ) > 0 );
        FC_ASSERT( secp256k1_ec_privkey_tweak_add( detail::_get_context(), (unsigned char*) a.data(), (unsigned char*) b.data() ) > 0 );
//        printf("hash: "); print(hash); printf("\n");
//        printf("blinded: "); print(a); printf("\n");
        return a;
    }

    private_key_secret extended_private_key::compute_p( int i ) const
    {
        private_key_secret p_inv = derive_normal_child( 2*i ).get_secret();
        invert( p_inv, p_inv );
//        printf("p: "); print(p_inv); printf("\n");
        return p_inv;
    }

    private_key_secret extended_private_key::compute_q( int i, const private_key_secret& p ) const
    {
        private_key_secret q = derive_normal_child( 2*i + 1 ).get_secret();
        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) q.data(), (unsigned char*) p.data() ) > 0 );
//        printf("q: "); print(q); printf("\n");
        return q;
    }

    blind_signature extended_private_key::blind_sign( const blinded_hash& hash, int i ) const
    {
        private_key_secret p = compute_p( i );
        private_key_secret q = compute_q( i, p );
        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) p.data(), (unsigned char*) hash.data() ) > 0 );
        FC_ASSERT( secp256k1_ec_privkey_tweak_add( detail::_get_context(), (unsigned char*) p.data(), (unsigned char*) q.data() ) > 0 );
//        printf("blind_sig: "); print(p); printf("\n");
        return p;
    }

    compact_signature extended_private_key::unblind_signature( const extended_public_key& bob,
                                                               const blind_signature& sig,
                                                               const fc::sha256& hash,
                                                               int i ) const
    {
        private_key_secret a = generate_a(i).get_secret();
        private_key_secret b = generate_b(i).get_secret();
        private_key_secret c = generate_c(i).get_secret();
        private_key_secret d = generate_d(i).get_secret();
        public_key p = bob.generate_p(i);
        public_key q = bob.generate_q(i);
        public_key_data k = compute_k( a, c, p );
        public_key_data t = compute_t( a, b, c, d, p, q ).serialize();

        FC_ASSERT( secp256k1_ec_privkey_tweak_mul( detail::_get_context(), (unsigned char*) c.data(), (unsigned char*) sig.data() ) > 0 );
        FC_ASSERT( secp256k1_ec_privkey_tweak_add( detail::_get_context(), (unsigned char*) c.data(), (unsigned char*) d.data() ) > 0 );

        compact_signature result;
        memcpy( result.begin() + 1, k.begin() + 1, 32 );
        memcpy( result.begin() + 33, c.data(), 32 );
        canonicalize( result.begin() + 33 );
//        printf("unblinded: "); print(result.begin() + 33); printf("\n");
        for ( int i = 0; i < 4; i++ )
        {
            unsigned char pubkey[33];
            int pklen = 33;
            if ( secp256k1_ecdsa_recover_compact( detail::_get_context(), (unsigned char*) hash.data(),
                                                  (unsigned char*) result.begin() + 1,
                                                  pubkey, &pklen, 1, i ) )
            {
                if ( !memcmp( t.begin(), pubkey, sizeof(pubkey) ) )
                {
                    *result.begin() = 27 + 4 + i;
                    return result;
//                } else {
//                    printf("Candidate: "); print( pubkey ); printf("\n");
                }
            }
        }
        FC_ASSERT( 0, "Failed to unblind - use different i" );
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
