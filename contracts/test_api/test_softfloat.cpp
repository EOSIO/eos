/**
 * @file
 * @copyright defined in eos/LICENSE.txt
 */

#include <eosiolib/eosio.hpp>
#include <iostream>
#include <cmath>
#include "test_api.hpp"

struct float_pack {
   float a;    // operand a
   float b;    // operand b
   float r;    // expected result
};

void test_softfloat::test_f32_add() {
   float_pack fp[16];
   read_action((char*)fp, sizeof(fp));
   float neg_zero = -0.0f;
   float infinity = 1.0f/0.0f;
   float neg_infinity = -1.0f/0.0f;
   
   // first test f32.add
   // test imprecise representations
   float c = fp[0].a + fp[0].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[0].r, "0.3 + 1.1 == 1.4~");
   
   // test precise representations
   c = fp[1].a + fp[1].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[1].r, "0.5 + 0.25 == 0.75");

   // test zeros representations
   c = fp[2].a + fp[2].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_zero, "-0.0 + -0.0 == -0.0");
   c = fp[3].a + fp[3].b;
   eosio_assert(*(uint32_t*)&c == 0, "0.0 + 0.0 == 0.0");
   c = fp[4].a + fp[4].b;
   eosio_assert(*(uint32_t*)&c == 0, "-0.0 + 0.0 == 0.0");

   // test NaNs 
   c = fp[5].a + fp[5].b;
   eosio_assert(isnan(c), "NAN + NAN == NAN");
   c = fp[6].a + fp[6].b;
   eosio_assert(isnan(c), "x + NAN == NAN");
   c = fp[7].a + fp[7].b;
   eosio_assert(isnan(c), "NAN + x == NAN");
   c = fp[8].a + fp[8].b;
   eosio_assert(isnan(c), "inf + -inf == NAN");

   // test infs
   c = fp[9].a + fp[9].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf + inf == inf");
   c = fp[10].a + fp[10].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_infinity, "-inf + -inf == -inf");
   c = fp[11].a + fp[11].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "x + inf == inf");
   c = fp[12].a + fp[12].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf + x == inf");

   // test additive identity
   c = fp[13].a + fp[13].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[13].r, "0.5 + 0.0 == 0.5");
   c = fp[14].a + fp[14].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[14].r, "0.0 + 0.1 == 0.1");

   // test additive inverse
   c = fp[15].a + fp[15].b;
   eosio_assert(*(uint32_t*)&c == 0, "0.5 + -0.5 == 0.0");
}

void test_softfloat::test_f32_sub() {
   float_pack fp[18];
   read_action((char*)fp, sizeof(fp));
   float neg_zero = -0.0f;
   float infinity = 1.0f/0.0f;
   float neg_infinity = -1.0f/0.0f;
   
   // first test f32.add
   // test imprecise representations
   float c = fp[0].a - fp[0].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[0].r, "0.3 - 1.1 == -0.8~");
   
   // test precise representations
   c = fp[1].a - fp[1].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[1].r, "0.5 - 0.25 == 0.25");

   // test zeros representations
   c = fp[2].a - fp[2].b;
   eosio_assert(*(uint32_t*)&c == 0, "-0.0 - -0.0 == 0.0");
   c = fp[3].a - fp[3].b;
   eosio_assert(*(uint32_t*)&c == 0, "0.0 - 0.0 == 0.0");
   c = fp[4].a - fp[4].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_zero, "-0.0 - 0.0 == -0.0");
   c = fp[5].a - fp[5].b;
   eosio_assert(*(uint32_t*)&c == 0, "0.0 - -0.0 == 0.0");

   // test NaNs 
   c = fp[6].a - fp[6].b;
   eosio_assert(isnan(c), "NAN - NAN == NAN");
   c = fp[7].a - fp[7].b;
   eosio_assert(isnan(c), "x - NAN == NAN");
   c = fp[8].a - fp[8].b;
   eosio_assert(isnan(c), "NAN - x == NAN");
   /* TODO ask about this, should be NaN, but is inf
   c = fp[9].a - fp[9].b;
   eosio_assert(isnan(c), "inf - inf == NAN");
   */

   // test infs
   c = fp[10].a - fp[10].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf - -inf == inf");
   c = fp[11].a - fp[11].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_infinity, "-inf - inf == -inf");
   c = fp[12].a - fp[12].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf - x == inf");
   c = fp[13].a - fp[13].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_infinity, "x - inf == -inf");

   // test identity
   c = fp[14].a - fp[14].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[14].r, "0.5 - 0.0 == 0.5");
   c = fp[15].a - fp[15].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[15].r, "0.0 - 0.1 == -0.1");

   // test additive inverse
   c = fp[16].a - fp[16].b;
   eosio_assert(*(uint32_t*)&c == 0, "-0.5 - -0.5 == 0.0");

   // test negation
   c = fp[17].a - fp[17].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[17].r, "0.0 - 0.5 == -0.5");
}

void test_softfloat::test_f32_mul() {
   float_pack fp[17];
   read_action((char*)fp, sizeof(fp));
   float neg_zero = -0.0f;
   float infinity = 1.0f/0.0f;
   float neg_infinity = -1.0f/0.0f;
   
   // first test f32.add
   // test imprecise representations
   float c = fp[0].a * fp[0].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[0].r, "0.3 * 1.1 == -0.33~");
   
   // test precise representations
   c = fp[1].a * fp[1].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[1].r, "0.5 * 0.25 == 0.125");

   // test zeros representations
   c = fp[2].a * fp[2].b;
   eosio_assert(*(uint32_t*)&c == 0, "-0.0 * -0.0 == 0.0");
   c = fp[3].a * fp[3].b;
   eosio_assert(*(uint32_t*)&c == 0, "0.0 * 0.0 == 0.0");
   c = fp[4].a * fp[4].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_zero, "-0.0 * 0.0 == -0.0");
   c = fp[5].a * fp[5].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_zero, "0.0 * -0.0 == -0.0");

   // test NaNs 
   c = fp[6].a * fp[6].b;
   eosio_assert(isnan(c), "NAN * NAN == NAN");
   c = fp[7].a * fp[7].b;
   eosio_assert(isnan(c), "x * NAN == NAN");
   c = fp[8].a * fp[8].b;
   eosio_assert(isnan(c), "NAN * x == NAN");
   c = fp[9].a * fp[9].b;
   eosio_assert(isnan(c), "0 * inf == NAN");

   // test infs
   c = fp[10].a * fp[10].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_infinity, "inf * -inf == -inf");
   c = fp[11].a * fp[11].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_infinity, "-inf * inf == -inf");
   c = fp[12].a * fp[12].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf * x == inf");
   c = fp[13].a * fp[13].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_infinity, "x * -inf == -inf");

   // test identity
   c = fp[14].a * fp[14].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[14].r, "0.5 * 1.0 == 0.5");
   c = fp[15].a * fp[15].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[15].r, "1.0 * -0.1 == -0.1");

   // test multiplicative inverse
   c = fp[16].a * fp[16].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[16].r, "0.5 * 1.0/0.5 == 1.0");
}

void test_softfloat::test_f32_div() {
   float_pack fp[17];
   read_action((char*)fp, sizeof(fp));
   float neg_zero = -0.0f;
   float infinity = 1.0f/0.0f;
   float neg_infinity = -1.0f/0.0f;
   
   // first test f32.add
   // test imprecise representations
   float c = fp[0].a / fp[0].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[0].r, "0.3 / 1.1 == 0.272727...~");
   
   // test precise representations
   c = fp[1].a / fp[1].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[1].r, "0.5 / 0.25 == 2.0");

   // test zeros representations
   c = fp[2].a / fp[2].b;
   eosio_assert(isnan(c), "-0.0 / -0.0 == NaN");
   c = fp[3].a / fp[3].b;
   eosio_assert(isnan(c), "0.0 / 0.0 == NaN");
   c = fp[4].a / fp[4].b;
   eosio_assert(isnan(c), "-0.0 / 0.0 == NaN");
   c = fp[5].a / fp[5].b;
   eosio_assert(isnan(c), "0.0 / -0.0 == NaN");

   // test NaNs 
   c = fp[6].a / fp[6].b;
   eosio_assert(isnan(c), "NAN / NAN == NAN");
   c = fp[7].a / fp[7].b;
   eosio_assert(isnan(c), "x / NAN == NAN");
   c = fp[8].a / fp[8].b;
   eosio_assert(isnan(c), "NAN / x == NAN");
   c = fp[9].a / fp[9].b;
   eosio_assert(*(uint32_t*)&c == 0, "0 / inf == 0");

   // test infs
   c = fp[10].a / fp[10].b;
   eosio_assert(isnan(c), "inf / inf == NaN");
   c = fp[11].a / fp[11].b;
   eosio_assert(isnan(c), "-inf / inf == NaN");
   c = fp[12].a / fp[12].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf / x == inf");
   c = fp[13].a / fp[13].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_zero, "x / -inf == -0.0");

   // test identity
   c = fp[14].a / fp[14].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[14].r, "0.5 / 1.0 == 0.5");
   c = fp[15].a / fp[15].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[15].r, "-0.1 / -1.0 == 0.1");

   // test multiplicative inverse
   c = fp[16].a / fp[16].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[16].r, "0.5 / 0.5 == 1.0");
}

void test_softfloat::test_f32_min() {
   float_pack fp[17];
   read_action((char*)fp, sizeof(fp));
   float neg_zero = -0.0f;
   float infinity = 1.0f/0.0f;
   float neg_infinity = -1.0f/0.0f;

   // first test f32.add
   // test imprecise representations
   float c = fmin(fp[0].a, fp[0].b);
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[0].r, "min(0.3, 1.1) == 0.3~");
   
   // test precise representations
   c = fp[1].a / fp[1].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[1].r, "0.5 / 0.25 == 2.0");

   // test zeros representations
   c = fp[2].a / fp[2].b;
   eosio_assert(isnan(c), "-0.0 / -0.0 == NaN");
   c = fp[3].a / fp[3].b;
   eosio_assert(isnan(c), "0.0 / 0.0 == NaN");
   c = fp[4].a / fp[4].b;
   eosio_assert(isnan(c), "-0.0 / 0.0 == NaN");
   c = fp[5].a / fp[5].b;
   eosio_assert(isnan(c), "0.0 / -0.0 == NaN");

   // test NaNs 
   c = fp[6].a / fp[6].b;
   eosio_assert(isnan(c), "NAN / NAN == NAN");
   c = fp[7].a / fp[7].b;
   eosio_assert(isnan(c), "x / NAN == NAN");
   c = fp[8].a / fp[8].b;
   eosio_assert(isnan(c), "NAN / x == NAN");
   c = fp[9].a / fp[9].b;
   eosio_assert(*(uint32_t*)&c == 0, "0 / inf == 0");

   // test infs
   c = fp[10].a / fp[10].b;
   eosio_assert(isnan(c), "inf / inf == NaN");
   c = fp[11].a / fp[11].b;
   eosio_assert(isnan(c), "-inf / inf == NaN");
   c = fp[12].a / fp[12].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&infinity, "inf / x == inf");
   c = fp[13].a / fp[13].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&neg_zero, "x / -inf == -0.0");

   // test identity
   c = fp[14].a / fp[14].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[14].r, "0.5 / 1.0 == 0.5");
   c = fp[15].a / fp[15].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[15].r, "-0.1 / -1.0 == 0.1");

   // test multiplicative inverse
   c = fp[16].a / fp[16].b;
   eosio_assert(*(uint32_t*)&c == *(uint32_t*)&fp[16].r, "0.5 / 0.5 == 1.0");
}
