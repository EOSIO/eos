#include <eosiolib/real.hpp>
#include <eosiolib/eosio.hpp>

#include "test_api.hpp"
using namespace eosio;
void test_real::create_instances() {
    real lhs1(5);
    eosio_assert(lhs1.value() == 5, "real instance value is wrong");
}

void test_real::test_division() {
    real lhs1(5);
    real rhs1(10);
    real result1 = lhs1 / rhs1;

    uint64_t a = double_div(i64_to_double(5), i64_to_double(10));
    eosio_assert(a == result1.value(), "real division result is wrong");
}

void test_real::test_division_by_0() {
    real lhs1(5);
    real rhs1(0);
    real result1 = lhs1 / rhs1;

    eosio_assert(false, "should've thrown an error");
}

void test_real::test_multiplication() {
    real lhs1(5);
    real rhs1(10);
    real result1 = lhs1 * rhs1;
    uint64_t res = double_mult( 5, 10 );
    eosio_assert(res == result1.value(), "real multiplication result is wrong");
}

void test_real::test_addition()
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
    eosio_assert(sum.value() == c, "real addition operation result is wrong");
}


