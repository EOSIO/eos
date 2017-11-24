#include <eoslib/real.hpp>
#include <eoslib/eos.hpp>

#include "test_api.hpp"
using namespace eosio;
unsigned int test_real::create_instances() {
    real lhs1(5);
    WASM_ASSERT(lhs1.value() == 5, "real instance value is wrong");
    return WASM_TEST_PASS;
}

unsigned int test_real::test_division() {
    real lhs1(5);
    real rhs1(10);
    real result1 = lhs1 / rhs1;

    uint64_t a = double_div(i64_to_double(5), i64_to_double(10));
    WASM_ASSERT(a == result1.value(), "real division result is wrong");
    return WASM_TEST_PASS;
}

unsigned int test_real::test_multiplication() {
    real lhs1(5);
    real rhs1(10);
    real result1 = lhs1 * rhs1;
    uint64_t res = double_mult( 5, 10 );
    WASM_ASSERT(res == result1.value(), "real multiplication result is wrong");
    return WASM_TEST_PASS;
}

unsigned int test_real::test_addition()
{
    real lhs1(5);
    real rhs1(10);
    real result1 = lhs1 / rhs1;
    uint64_t a = double_div(i64_to_double(5), i64_to_double(10));

    real lhs2(5);
    real rhs2(2);
    real result2 = lhs2 / rhs2;
    uint64_t b = double_div(i64_to_double(5), i64_to_double(2));


    real sum = result1+result2;
    uint64_t c = double_add( a, b );
    WASM_ASSERT(sum.value() == c, "real addition operation result is wrong");

    return WASM_TEST_PASS;
}


