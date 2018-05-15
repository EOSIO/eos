#include "r1_signature_compactor.hpp"

using namespace fc;

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

fc::crypto::r1::compact_signature compact_r1(fc::crypto::r1::public_key_data& pubkey, fc::ecdsa_sig& sig, fc::sha256& digest) {
   fc::crypto::r1::compact_signature csig;
   ec_key key = EC_KEY_new_by_curve_name( NID_X9_62_prime256v1 );

   //We can't use ssl_bignum here; _get0() does not transfer ownership to us; _set0() does transfer ownership to fc::ecdsa_sig
   const BIGNUM *sig_r, *sig_s;
   BIGNUM *r = BN_new(), *s = BN_new();
   ECDSA_SIG_get0(sig, &sig_r, &sig_s);
   BN_copy(r, sig_r);
   BN_copy(s, sig_s);

   //want to always use the low S value
   const EC_GROUP* group = EC_KEY_get0_group(key);
   ssl_bignum order, halforder;
   EC_GROUP_get_order(group, order, nullptr);
   BN_rshift1(halforder, order);
   if(BN_cmp(s, halforder) > 0)
       BN_sub(s, order, s);

   int nBitsR = BN_num_bits(r);
   int nBitsS = BN_num_bits(s);
   if(nBitsR > 256 || nBitsS > 256)
      FC_THROW_EXCEPTION( exception, "Unable to sign" );

   ECDSA_SIG_set0(sig, r, s);

   int nRecId = -1;
   for (int i=0; i<4; i++)
   {
      if (ECDSA_SIG_recover_key_GFp(key, sig, (unsigned char*)digest.data(), digest.data_size(), i, 1) == 1)
      {
         EC_KEY_set_conv_form(key, POINT_CONVERSION_COMPRESSED );
         unsigned char* pubcheck = nullptr;
         int s = i2o_ECPublicKey(key, &pubcheck);
         if (memcmp(pubcheck, pubkey.data, pubkey.size()) == 0)
         {
            nRecId = i;
            free(pubcheck);
            break;
         }
         if(s > 0)
            free(pubcheck);
      }
   }
   if (nRecId == -1)
      FC_THROW_EXCEPTION( exception, "unable to construct recoverable key");

   BN_bn2bin(r,&csig.data[33-(nBitsR+7)/8]);
   BN_bn2bin(s,&csig.data[65-(nBitsS+7)/8]);
   csig.data[0] = nRecId+27+4;

   return csig;
}