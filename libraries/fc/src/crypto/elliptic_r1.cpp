#include <fc/crypto/elliptic_r1.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/openssl.hpp>

#include <fc/fwd_impl.hpp>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>

#include <assert.h>

namespace fc { namespace crypto { namespace r1 {
    namespace detail
    {
      class public_key_impl
      {
        public:
          public_key_impl()
          :_key(nullptr)
          {
          static int init = init_openssl();
          }

          ~public_key_impl()
          {
            if( _key != nullptr )
            {
              EC_KEY_free(_key);
            }
          }
          public_key_impl( const public_key_impl& cpy )
          {
            _key = cpy._key ? EC_KEY_dup( cpy._key ) : nullptr;
          }
          EC_KEY* _key;
      };
      class private_key_impl
      {
        public:
          private_key_impl()
          :_key(nullptr)
          {
          static int init = init_openssl();
          }
          ~private_key_impl()
          {
            if( _key != nullptr )
            {
              EC_KEY_free(_key);
            }
          }
          private_key_impl( const private_key_impl& cpy )
          {
            _key = cpy._key ? EC_KEY_dup( cpy._key ) : nullptr;
          }
          EC_KEY* _key;
      };
    }
    static void * ecies_key_derivation(const void *input, size_t ilen, void *output, size_t *olen)
    {
        if (*olen < SHA512_DIGEST_LENGTH) {
          return NULL;
        }
        *olen = SHA512_DIGEST_LENGTH;
        return (void*)SHA512((const unsigned char*)input, ilen, (unsigned char*)output);
    }

    // Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
    // recid selects which key is recovered
    // if check is non-zero, additional checks are performed
    static int ECDSA_SIG_recover_key_GFp(EC_KEY *eckey, ECDSA_SIG *ecsig, const unsigned char *msg, int msglen, int recid, int check)
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

        const BIGNUM *r, *s;
        ECDSA_SIG_get0(ecsig, &r, &s);

        const EC_GROUP *group = EC_KEY_get0_group(eckey);
        if ((ctx = BN_CTX_new()) == NULL) { ret = -1; goto err; }
        BN_CTX_start(ctx);
        order = BN_CTX_get(ctx);
        if (!EC_GROUP_get_order(group, order, ctx)) { ret = -2; goto err; }
        x = BN_CTX_get(ctx);
        if (!BN_copy(x, order)) { ret=-1; goto err; }
        if (!BN_mul_word(x, i)) { ret=-1; goto err; }
        if (!BN_add(x, x, r)) { ret=-1; goto err; }
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
        if (!BN_mod_inverse(rr, r, order, ctx)) { ret=-1; goto err; }
        sor = BN_CTX_get(ctx);
        if (!BN_mod_mul(sor, s, rr, order, ctx)) { ret=-1; goto err; }
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

/*
    public_key::public_key()
    :my( new detail::public_key_impl() )
    {
    }

    public_key::public_key( fc::bigint pub_x, fc::bigint pub_y )
    :my( new detail::public_key_impl() )
    {
    }

    public_key::~public_key()
    {
    }
    */

    public_key public_key::mult( const fc::sha256& digest )
    {
        // get point from this public key
        const EC_POINT* master_pub   = EC_KEY_get0_public_key( my->_key );
        ec_group group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));

        ssl_bignum z;
        BN_bin2bn((unsigned char*)&digest, sizeof(digest), z);

        // multiply by digest
        ssl_bignum one;
        BN_one(one);
        bn_ctx ctx(BN_CTX_new());

        ec_point result(EC_POINT_new(group));
        EC_POINT_mul(group, result, z, master_pub, one, ctx);

        public_key rtn;
        rtn.my->_key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
        EC_KEY_set_public_key(rtn.my->_key,result);

        return rtn;
    }
    bool       public_key::valid()const
    {
      return my->_key != nullptr;
    }
    public_key public_key::add( const fc::sha256& digest )const
    {
      try {
        ec_group group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
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

        ssl_bignum z;
        BN_bin2bn((unsigned char*)&digest, sizeof(digest), z);

        // multiply by digest
        ssl_bignum one;
        BN_one(one);

        ec_point result(EC_POINT_new(group));
        EC_POINT_add(group, result, digest_point, master_pub, ctx);

        if (EC_POINT_is_at_infinity(group, result))
        {
          FC_THROW_EXCEPTION( exception, "point at  infinity" );
        }


        public_key rtn;
        rtn.my->_key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
        EC_KEY_set_public_key(rtn.my->_key,result);
        return rtn;
      } FC_RETHROW_EXCEPTIONS( debug, "digest: ${digest}", ("digest",digest) );
    }

    std::string public_key::to_base58() const
    {
      public_key_data key = serialize();
      uint32_t check = (uint32_t)sha256::hash(key.data, sizeof(key))._hash[0];
      assert(key.size() + sizeof(check) == 37);
      array<char, 37> data;
      memcpy(data.data, key.begin(), key.size());
      memcpy(data.begin() + key.size(), (const char*)&check, sizeof(check));
      return fc::to_base58(data.begin(), data.size());
    }

    public_key public_key::from_base58( const std::string& b58 )
    {
        array<char, 37> data;
        size_t s = fc::from_base58(b58, (char*)&data, sizeof(data) );
        FC_ASSERT( s == sizeof(data) );

        public_key_data key;
        uint32_t check = (uint32_t)sha256::hash(data.data, sizeof(key))._hash[0];
        FC_ASSERT( memcmp( (char*)&check, data.data + sizeof(key), sizeof(check) ) == 0 );
        memcpy( (char*)key.data, data.data, sizeof(key) );
        return public_key(key);
    }

    private_key::private_key()
    {}

    private_key private_key::generate_from_seed( const fc::sha256& seed, const fc::sha256& offset )
    {
        ssl_bignum z;
        BN_bin2bn((unsigned char*)&offset, sizeof(offset), z);

        ec_group group(EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1));
        bn_ctx ctx(BN_CTX_new());
        ssl_bignum order;
        EC_GROUP_get_order(group, order, ctx);

        // secexp = (seed + z) % order
        ssl_bignum secexp;
        BN_bin2bn((unsigned char*)&seed, sizeof(seed), secexp);
        BN_add(secexp, secexp, z);
        BN_mod(secexp, secexp, order, ctx);

        fc::sha256 secret;
        assert(BN_num_bytes(secexp) <= int64_t(sizeof(secret)));
        auto shift = sizeof(secret) - BN_num_bytes(secexp);
        BN_bn2bin(secexp, ((unsigned char*)&secret)+shift);
        return regenerate( secret );
    }

    private_key private_key::regenerate( const fc::sha256& secret )
    {
       private_key self;
       self.my->_key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
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
       if( !my->_key )
       {
          return fc::sha256();
       }

       fc::sha256 sec;
       const BIGNUM* bn = EC_KEY_get0_private_key(my->_key);
       if( bn == NULL )
       {
         FC_THROW_EXCEPTION( exception, "get private key failed" );
       }
       int nbytes = BN_num_bytes(bn);
       BN_bn2bin(bn, &((unsigned char*)&sec)[32-nbytes] );
       return sec;
    }

    private_key private_key::generate()
    {
       private_key self;
       EC_KEY* k = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
       if( !k ) FC_THROW_EXCEPTION( exception, "Unable to generate EC key" );
       self.my->_key = k;
       if( !EC_KEY_generate_key( self.my->_key ) )
       {
          FC_THROW_EXCEPTION( exception, "ecc key generation error" );

       }

#if 0
          = bigint( EC_KEY_get0_private_key( k );
       EC_POINT* pub   = EC_KEY_get0_public_key( k );
       EC_GROUP* group = EC_KEY_get0_group( k );

       EC_POINT_get_affine_coordinates_GFp( group, pub, self.my->_pub_x.get(), self.my->_pub_y.get(), nullptr/*ctx*/ );

       EC_KEY_free(k);
#endif

       return self;
    }

    signature private_key::sign( const fc::sha256& digest )const
    {
        unsigned int buf_len = ECDSA_size(my->_key);
//        fprintf( stderr, "%d  %d\n", buf_len, sizeof(sha256) );
        signature sig;
        assert( buf_len == sizeof(sig) );

        if( !ECDSA_sign( 0,
                    (const unsigned char*)&digest, sizeof(digest),
                    (unsigned char*)&sig, &buf_len, my->_key ) )
        {
            FC_THROW_EXCEPTION( exception, "signing error" );
        }


        return sig;
    }
    bool       public_key::verify( const fc::sha256& digest, const fc::crypto::r1::signature& sig )
    {
      return 1 == ECDSA_verify( 0, (unsigned char*)&digest, sizeof(digest), (unsigned char*)&sig, sizeof(sig), my->_key );
    }

    public_key_data public_key::serialize()const
    {
      public_key_data dat;
      if( !my->_key ) return dat;
      EC_KEY_set_conv_form( my->_key, POINT_CONVERSION_COMPRESSED );
      /*size_t nbytes = i2o_ECPublicKey( my->_key, nullptr ); */
      /*assert( nbytes == 33 )*/
      char* front = &dat.data[0];
      i2o_ECPublicKey( my->_key, (unsigned char**)&front  );
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
      i2o_ECPublicKey( my->_key, (unsigned char**)&front  );
      return dat;
    }

    public_key::public_key()
    {
    }
    public_key::~public_key()
    {
    }
    public_key::public_key( const public_key_point_data& dat )
    {
      const char* front = &dat.data[0];
      if( *front == 0 ){}
      else
      {
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
         my->_key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
         my->_key = o2i_ECPublicKey( &my->_key, (const unsigned char**)&front, sizeof(public_key_data) );
         if( !my->_key )
         {
           FC_THROW_EXCEPTION( exception, "error decoding public key", ("s", ERR_error_string( ERR_get_error(), nullptr) ) );
         }
      }
    }

    bool       private_key::verify( const fc::sha256& digest, const fc::crypto::r1::signature& sig )
    {
      return 1 == ECDSA_verify( 0, (unsigned char*)&digest, sizeof(digest), (unsigned char*)&sig, sizeof(sig), my->_key );
    }

    public_key private_key::get_public_key()const
    {
       public_key pub;
       pub.my->_key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
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

    private_key::~private_key()
    {
    }

    public_key::public_key( const compact_signature& c, const fc::sha256& digest, bool check_canonical )
    {
        int nV = c.data[0];
        if (nV<27 || nV>=35)
            FC_THROW_EXCEPTION( exception, "unable to reconstruct public key from signature" );

        ecdsa_sig sig = ECDSA_SIG_new();
        BIGNUM *r = BN_new(), *s = BN_new();
        BN_bin2bn(&c.data[1],32,r);
        BN_bin2bn(&c.data[33],32,s);
        ECDSA_SIG_set0(sig, r, s);

        my->_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);

        const EC_GROUP* group = EC_KEY_get0_group(my->_key);
        ssl_bignum order, halforder;
        EC_GROUP_get_order(group, order, nullptr);
        BN_rshift1(halforder, order);
        if(BN_cmp(s, halforder) > 0)
           FC_THROW_EXCEPTION( exception, "invalid high s-value encountered in r1 signature" );

        if (nV >= 31)
        {
            EC_KEY_set_conv_form( my->_key, POINT_CONVERSION_COMPRESSED );
            nV -= 4;
//            fprintf( stderr, "compressed\n" );
        }

        if (ECDSA_SIG_recover_key_GFp(my->_key, sig, (unsigned char*)&digest, sizeof(digest), nV - 27, 0) == 1)
            return;
        FC_THROW_EXCEPTION( exception, "unable to reconstruct public key from signature" );
    }

    compact_signature private_key::sign_compact( const fc::sha256& digest )const
    {
      try {
        FC_ASSERT( my->_key != nullptr );
        auto my_pub_key = get_public_key().serialize(); // just for good measure
        ecdsa_sig sig = ECDSA_do_sign((unsigned char*)&digest, sizeof(digest), my->_key);

        if (sig==nullptr)
          FC_THROW_EXCEPTION( exception, "Unable to sign" );

        //We can't use ssl_bignum here; _get0() does not transfer ownership to us; _set0() does transfer ownership to fc::ecdsa_sig
        const BIGNUM *sig_r, *sig_s;
        BIGNUM *r = BN_new(), *s = BN_new();
        ECDSA_SIG_get0(sig, &sig_r, &sig_s);
        BN_copy(r, sig_r);
        BN_copy(s, sig_s);

        //want to always use the low S value
        const EC_GROUP* group = EC_KEY_get0_group(my->_key);
        ssl_bignum order, halforder;
        EC_GROUP_get_order(group, order, nullptr);
        BN_rshift1(halforder, order);
        if(BN_cmp(s, halforder) > 0)
           BN_sub(s, order, s);

        compact_signature csig;

        int nBitsR = BN_num_bits(r);
        int nBitsS = BN_num_bits(s);
        if(nBitsR > 256 || nBitsS > 256)
          FC_THROW_EXCEPTION( exception, "Unable to sign" );

        ECDSA_SIG_set0(sig, r, s);
        
        int nRecId = -1;
        for (int i=0; i<4; i++)
        {
          public_key keyRec;
          keyRec.my->_key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );
          if (ECDSA_SIG_recover_key_GFp(keyRec.my->_key, sig, (unsigned char*)&digest, sizeof(digest), i, 1) == 1)
          {
            if (keyRec.serialize() == my_pub_key )
            {
              nRecId = i;
              break;
            }
          }
        }
        if (nRecId == -1)
          FC_THROW_EXCEPTION( exception, "unable to construct recoverable key");

        csig.data[0] = nRecId+27+4;
        BN_bn2bin(r,&csig.data[33-(nBitsR+7)/8]);
        BN_bn2bin(s,&csig.data[65-(nBitsS+7)/8]);
        
        return csig;
      } FC_RETHROW_EXCEPTIONS( warn, "sign ${digest}", ("digest", digest)("private_key",*this) );
    }

   private_key& private_key::operator=( private_key&& pk )
   {
     if( my->_key )
     {
       EC_KEY_free(my->_key);
     }
     my->_key = pk.my->_key;
     pk.my->_key = nullptr;
     return *this;
   }
   public_key::public_key( const public_key& pk )
   :my(pk.my)
   {
   }
   public_key::public_key( public_key&& pk )
   :my( fc::move( pk.my) )
   {
   }
   private_key::private_key( const private_key& pk )
   :my(pk.my)
   {
   }
   private_key::private_key( private_key&& pk )
   :my( fc::move( pk.my) )
   {
   }

   public_key& public_key::operator=( public_key&& pk )
   {
     if( my->_key )
     {
       EC_KEY_free(my->_key);
     }
     my->_key = pk.my->_key;
     pk.my->_key = nullptr;
     return *this;
   }
   public_key& public_key::operator=( const public_key& pk )
   {
     if( my->_key )
     {
       EC_KEY_free(my->_key);
     }
     my->_key = EC_KEY_dup(pk.my->_key);
     return *this;
   }
   private_key& private_key::operator=( const private_key& pk )
   {
     if( my->_key )
     {
       EC_KEY_free(my->_key);
     }
     my->_key = EC_KEY_dup(pk.my->_key);
     return *this;
   }

}
}
  void to_variant( const crypto::r1::private_key& var,  variant& vo )
  {
    vo = var.get_secret();
  }
  void from_variant( const variant& var,  crypto::r1::private_key& vo )
  {
    fc::sha256 sec;
    from_variant( var, sec );
    vo = crypto::r1::private_key::regenerate(sec);
  }

  void to_variant( const crypto::r1::public_key& var,  variant& vo )
  {
    vo = var.serialize();
  }
  void from_variant( const variant& var,  crypto::r1::public_key& vo )
  {
    crypto::r1::public_key_data dat;
    from_variant( var, dat );
    vo = crypto::r1::public_key(dat);
  }


}
