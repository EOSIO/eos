#include <eosiolib/fixedpoint.hpp>
#include <eosiolib/eos.hpp>

#include "test_api.hpp"
using namespace eosio;
unsigned int test_fixedpoint::create_instances()
{
    {
        // Various ways to create fixed_point128
        fixed_point128<18> a(12345667);
        fixed_point128<18> b(12345667);
        fixed_point128<16> c(12345667);
        WASM_ASSERT(b == a, "fixed_point128 instances comparison with same number of decimals");
        WASM_ASSERT(c == a, "fixed_point128 instances with different number of decimals");
    }

    {
        // Various ways to create fixed_point64
        fixed_point64<5> a(12345667);
        fixed_point64<5> b(12345667);
        fixed_point64<5> c(12345667);
        WASM_ASSERT(b == a, "fixed_point64 instances comparison with same number of decimals");
        WASM_ASSERT(c == a, "fixed_point64 instances with different number of decimals");
    }

    {
        // Various ways to create fixed_point32
        fixed_point32<18> a(12345667);
        fixed_point32<18> b(12345667);
        fixed_point32<16> c(12345667);
        WASM_ASSERT(b == a, "fixed_point32 instances comparison with same number of decimals");
        WASM_ASSERT(c == a, "fixed_point32 instances with different number of decimals");
    }


  return WASM_TEST_PASS;
}

unsigned int test_fixedpoint::test_addition()
{
    {
        // Various ways to create fixed_point32
        fixed_point32<0> a(100);
        fixed_point32<0> b(100);
        fixed_point32<0> c = a + b;
        fixed_point32<0> d = 200;
        WASM_ASSERT(c == d, "fixed_point32 instances addition with zero decmimals");
    }
    {
        // Various ways to create fixed_point64
        fixed_point64<0> a(100);
        fixed_point64<0> b(100);
        fixed_point64<0> c = a + b;
        fixed_point64<0> d = 200;
        WASM_ASSERT(c == d, "fixed_point64 instances addition with zero decmimals");
    }

    return WASM_TEST_PASS;
};

unsigned int test_fixedpoint::test_subtraction()
{
    {
        // Various ways to create fixed_point64
        fixed_point64<0> a(100);
        fixed_point64<0> b(100);
        fixed_point64<0> c = a - b;
        fixed_point64<0> d = 0;
        WASM_ASSERT(c == d, "fixed_point64 instances subtraction with zero decmimals");
    }
    {
        // Various ways to create fixed_point32
        fixed_point32<0> a(100);
        fixed_point32<0> b(100);
        fixed_point32<0> c = a - b;
        fixed_point32<0> d = 0;
        WASM_ASSERT(c == d, "fixed_point32 instances subtraction with zero decmimals");
    }

    return WASM_TEST_PASS;
};

unsigned int test_fixedpoint::test_multiplication()
{
    {
        // Various ways to create fixed_point64
        fixed_point64<0> a(100);
        fixed_point64<0> b(200);
        fixed_point128<0> c = a * b;
        fixed_point128<0> d(200*100);
        WASM_ASSERT(c == d, "fixed_point64 instances multiplication result in fixed_point128");
    }

    {
        // Various ways to create fixed_point32
        fixed_point32<0> a(100);
        fixed_point32<0> b(200);
        fixed_point64<0> c = a * b;
        fixed_point64<0> d(200*100);
        WASM_ASSERT(c == d, "fixed_point32 instances multiplication result in fixed_point64");
    }
    return WASM_TEST_PASS;

}

unsigned int test_fixedpoint::test_division()
{
    {
        uint64_t lhs = 10000000;
        uint64_t rhs = 333;

        fixed_point64<0> a(lhs);
        fixed_point64<0> b(rhs);
        fixed_point128<5> c = a / b;

        fixed_point128<5> e = fixed_divide<5>(lhs, rhs);
        WASM_ASSERT(c == e, "fixed_point64 instances division result from operator and function and compare in fixed_point128");

    }

    {
        uint32_t lhs = 100000;
        uint32_t rhs = 33;

        fixed_point32<0> a(lhs);
        fixed_point32<0> b(rhs);
        fixed_point64<5> c = a / b;

        fixed_point64<5> e = fixed_divide<5>(lhs, rhs);
        WASM_ASSERT(c == e, "fixed_point64 instances division result from operator and function and compare in fixed_point128");

    }
    return WASM_TEST_PASS;

}


