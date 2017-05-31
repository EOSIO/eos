#include <fc/fwd_impl.hpp>
#include <boost/config.hpp>

#include "_elliptic_impl_pub.hpp"

/* used by mixed + openssl */

namespace fc { namespace ecc {
    namespace detail {

        public_key_impl::public_key_impl() BOOST_NOEXCEPT
        {
            _init_lib();
        }

        public_key_impl::public_key_impl( const public_key_impl& cpy ) BOOST_NOEXCEPT
        {
            _init_lib();
            *this = cpy;
        }

        public_key_impl::public_key_impl( public_key_impl&& cpy ) BOOST_NOEXCEPT
        {
            _init_lib();
            *this = cpy;
        }

        public_key_impl::~public_key_impl() BOOST_NOEXCEPT
        {
            free_key();
        }

        public_key_impl& public_key_impl::operator=( const public_key_impl& pk ) BOOST_NOEXCEPT
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

        public_key_impl& public_key_impl::operator=( public_key_impl&& pk ) BOOST_NOEXCEPT
        {
            if ( this != &pk ) {
                free_key();
                _key = pk._key;
                pk._key = nullptr;
            }
            return *this;
        }

        void public_key_impl::free_key() BOOST_NOEXCEPT
        {
            if( _key != nullptr )
            {
                EC_KEY_free(_key);
                _key = nullptr;
            }
        }

        // Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
        // recid selects which key is recovered
        // if check is non-zero, additional checks are performed
        int public_key_impl::ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig,
                                                       const unsigned char *msg,
                                                       int msglen, int recid, int check)
        {
            if (!eckey) FC_THROW_EXCEPTION( exception, "null key" );

            int ret = 0;
            BN_CTX *ctx = NULL;

            BIGNUM *x = NULL;
            BIGNUM *e = NULL;
            BIGNUM *order = NULL;
            BIGNUM *sor = NULL;
            BIGNUM *eor = NULL;
            BIGNUM *field = NULL;
            EC_POINT *R = NULL;
            EC_POINT *O = NULL;
            EC_POINT *Q = NULL;
            BIGNUM *rr = NULL;
            BIGNUM *zero = NULL;
            int n = 0;
            int i = recid / 2;

            const EC_GROUP *group = EC_KEY_get0_group(eckey);
            if ((ctx = BN_CTX_new()) == NULL) { ret = -1; goto err; }
            BN_CTX_start(ctx);
            order = BN_CTX_get(ctx);
            if (!EC_GROUP_get_order(group, order, ctx)) { ret = -2; goto err; }
            x = BN_CTX_get(ctx);
            if (!BN_copy(x, order)) { ret=-1; goto err; }
            if (!BN_mul_word(x, i)) { ret=-1; goto err; }
            if (!BN_add(x, x, ecsig->r)) { ret=-1; goto err; }
            field = BN_CTX_get(ctx);
            if (!EC_GROUP_get_curve_GFp(group, field, NULL, NULL, ctx)) { ret=-2; goto err; }
            if (BN_cmp(x, field) >= 0) { ret=0; goto err; }
            if ((R = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
            if (!EC_POINT_set_compressed_coordinates_GFp(group, R, x, recid % 2, ctx)) { ret=0; goto err; }
            if (check)
            {
                if ((O = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
                if (!EC_POINT_mul(group, O, NULL, R, order, ctx)) { ret=-2; goto err; }
                if (!EC_POINT_is_at_infinity(group, O)) { ret = 0; goto err; }
            }
            if ((Q = EC_POINT_new(group)) == NULL) { ret = -2; goto err; }
            n = EC_GROUP_get_degree(group);
            e = BN_CTX_get(ctx);
            if (!BN_bin2bn(msg, msglen, e)) { ret=-1; goto err; }
            if (8*msglen > n) BN_rshift(e, e, 8-(n & 7));
            zero = BN_CTX_get(ctx);
            if (!BN_zero(zero)) { ret=-1; goto err; }
            if (!BN_mod_sub(e, zero, e, order, ctx)) { ret=-1; goto err; }
            rr = BN_CTX_get(ctx);
            if (!BN_mod_inverse(rr, ecsig->r, order, ctx)) { ret=-1; goto err; }
            sor = BN_CTX_get(ctx);
            if (!BN_mod_mul(sor, ecsig->s, rr, order, ctx)) { ret=-1; goto err; }
            eor = BN_CTX_get(ctx);
            if (!BN_mod_mul(eor, e, rr, order, ctx)) { ret=-1; goto err; }
            if (!EC_POINT_mul(group, Q, eor, R, sor, ctx)) { ret=-2; goto err; }
            if (!EC_KEY_set_public_key(eckey, Q)) { ret=-2; goto err; }

            ret = 1;

        err:
            if (ctx) {
                BN_CTX_end(ctx);
                BN_CTX_free(ctx);
            }
            if (R != NULL) EC_POINT_free(R);
            if (O != NULL) EC_POINT_free(O);
            if (Q != NULL) EC_POINT_free(Q);
            return ret;
        }
    }

    public_key::public_key() {}

    public_key::public_key( const public_key& pk ) : my( pk.my ) {}

    public_key::public_key( public_key&& pk ) : my( std::move( pk.my ) ) {}

    public_key::~public_key() {}

    public_key& public_key::operator=( public_key&& pk )
    {
        my = std::move(pk.my);
        return *this;
    }

    public_key& public_key::operator=( const public_key& pk )
    {
        my = pk.my;
        return *this;
    }

    bool public_key::valid()const
    {
      return my->_key != nullptr;
    }

    /* WARNING! This implementation is broken, it is actually equivalent to
     * public_key::add()!
     */
//    public_key public_key::mult( const fc::sha256& digest ) const
//    {
//        // get point from this public key
//        const EC_POINT* master_pub   = EC_KEY_get0_public_key( my->_key );
//        ec_group group(EC_GROUP_new_by_curve_name(NID_secp256k1));
//
//        ssl_bignum z;
//        BN_bin2bn((unsigned char*)&digest, sizeof(digest), z);
//
//        // multiply by digest
//        ssl_bignum one;
//        BN_one(one);
//        bn_ctx ctx(BN_CTX_new());
//
//        ec_point result(EC_POINT_new(group));
//        EC_POINT_mul(group, result, z, master_pub, one, ctx);
//
//        public_key rtn;
//        rtn.my->_key = EC_KEY_new_by_curve_name( NID_secp256k1 );
//        EC_KEY_set_public_key(rtn.my->_key,result);
//
//        return rtn;
//    }
    public_key public_key::add( const fc::sha256& digest )const
    {
      try {
        ec_group group(EC_GROUP_new_by_curve_name(NID_secp256k1));
        bn_ctx ctx(BN_CTX_new());

        fc::bigint digest_bi( (char*)&digest, sizeof(digest) );

        ssl_bignum order;
        EC_GROUP_get_order(group, order, ctx);
        if( digest_bi > fc::bigint(order) )
        {
          FC_THROW_EXCEPTION( exception, "digest > group order" );
        }


        public_key digest_key = private_key::regenerate(digest).get_public_key();
        const EC_POINT* digest_point   = EC_KEY_get0_public_key( digest_key.my->_key );

        // get point from this public key
        const EC_POINT* master_pub   = EC_KEY_get0_public_key( my->_key );

//        ssl_bignum z;
//        BN_bin2bn((unsigned char*)&digest, sizeof(digest), z);

        // multiply by digest
//        ssl_bignum one;
//        BN_one(one);

        ec_point result(EC_POINT_new(group));
        EC_POINT_add(group, result, digest_point, master_pub, ctx);

        if (EC_POINT_is_at_infinity(group, result))
        {
          FC_THROW_EXCEPTION( exception, "point at  infinity" );
        }


        public_key rtn;
        rtn.my->_key = EC_KEY_new_by_curve_name( NID_secp256k1 );
        EC_KEY_set_public_key(rtn.my->_key,result);
        return rtn;
      } FC_RETHROW_EXCEPTIONS( debug, "digest: ${digest}", ("digest",digest) );
    }

    std::string public_key::to_base58() const
    {
      public_key_data key = serialize();
      return to_base58( key );
    }

//    signature private_key::sign( const fc::sha256& digest )const
//    {
//        unsigned int buf_len = ECDSA_size(my->_key);
////        fprintf( stderr, "%d  %d\n", buf_len, sizeof(sha256) );
//        signature sig;
//        assert( buf_len == sizeof(sig) );
//
//        if( !ECDSA_sign( 0,
//                    (const unsigned char*)&digest, sizeof(digest),
//                    (unsigned char*)&sig, &buf_len, my->_key ) )
//        {
//            FC_THROW_EXCEPTION( exception, "signing error" );
//        }
//
//
//        return sig;
//    }
//    bool       public_key::verify( const fc::sha256& digest, const fc::ecc::signature& sig )
//    {
//      return 1 == ECDSA_verify( 0, (unsigned char*)&digest, sizeof(digest), (unsigned char*)&sig, sizeof(sig), my->_key );
//    }

    public_key_data public_key::serialize()const
    {
      public_key_data dat;
      if( !my->_key ) return dat;
      EC_KEY_set_conv_form( my->_key, POINT_CONVERSION_COMPRESSED );
      /*size_t nbytes = i2o_ECPublicKey( my->_key, nullptr ); */
      /*assert( nbytes == 33 )*/
      char* front = &dat.data[0];
      i2o_ECPublicKey( my->_key, (unsigned char**)&front ); // FIXME: questionable memory handling
      return dat;
      /*
       EC_POINT* pub   = EC_KEY_get0_public_key( my->_key );
       EC_GROUP* group = EC_KEY_get0_group( my->_key );
       EC_POINT_get_affine_coordinates_GFp( group, pub, self.my->_pub_x.get(), self.my->_pub_y.get(), nullptr );
       */
    }
    public_key_point_data public_key::serialize_ecc_point()const
    {
      public_key_point_data dat;
      if( !my->_key ) return dat;
      EC_KEY_set_conv_form( my->_key, POINT_CONVERSION_UNCOMPRESSED );
      char* front = &dat.data[0];
      i2o_ECPublicKey( my->_key, (unsigned char**)&front ); // FIXME: questionable memory handling
      return dat;
    }

    public_key::public_key( const public_key_point_data& dat )
    {
      const char* front = &dat.data[0];
      if( *front == 0 ){}
      else
      {
         my->_key = EC_KEY_new_by_curve_name( NID_secp256k1 );
         my->_key = o2i_ECPublicKey( &my->_key, (const unsigned char**)&front, sizeof(dat)  );
         if( !my->_key )
         {
           FC_THROW_EXCEPTION( exception, "error decoding public key", ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
         }
      }
    }
    public_key::public_key( const public_key_data& dat )
    {
      const char* front = &dat.data[0];
      if( *front == 0 ){}
      else
      {
         my->_key = EC_KEY_new_by_curve_name( NID_secp256k1 );
         my->_key = o2i_ECPublicKey( &my->_key, (const unsigned char**)&front, sizeof(public_key_data) );
         if( !my->_key )
         {
           FC_THROW_EXCEPTION( exception, "error decoding public key", ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
         }
      }
    }

//    bool       private_key::verify( const fc::sha256& digest, const fc::ecc::signature& sig )
//    {
//      return 1 == ECDSA_verify( 0, (unsigned char*)&digest, sizeof(digest), (unsigned char*)&sig, sizeof(sig), my->_key );
//    }

    public_key::public_key( const compact_signature& c, const fc::sha256& digest, bool check_canonical )
    {
        int nV = c.data[0];
        if (nV<27 || nV>=35)
            FC_THROW_EXCEPTION( exception, "unable to reconstruct public key from signature" );

        ECDSA_SIG *sig = ECDSA_SIG_new();
        BN_bin2bn(&c.data[1],32,sig->r);
        BN_bin2bn(&c.data[33],32,sig->s);

        if( check_canonical )
        {
            FC_ASSERT( is_canonical( c ), "signature is not canonical" );
        }

        my->_key = EC_KEY_new_by_curve_name(NID_secp256k1);

        if (nV >= 31)
        {
            EC_KEY_set_conv_form( my->_key, POINT_CONVERSION_COMPRESSED );
            nV -= 4;
//            fprintf( stderr, "compressed\n" );
        }

        if (detail::public_key_impl::ECDSA_SIG_recover_key_GFp(my->_key, sig, (unsigned char*)&digest, sizeof(digest), nV - 27, 0) == 1)
        {
            ECDSA_SIG_free(sig);
            return;
        }
        ECDSA_SIG_free(sig);
        FC_THROW_EXCEPTION( exception, "unable to reconstruct public key from signature" );
    }
}}
