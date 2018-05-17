#include <enumivolib/fixedpoint.hpp>
#include <enumivolib/enumivo.hpp>

#include "test_api.hpp"

void test_fixedpoint::create_instances()
{
    {
        // Various ways to create fixed_point128
       enumivo::fixed_point128<18> a(12345667);
       enumivo::fixed_point128<18> b(12345667);
       enumivo::fixed_point128<16> c(12345667);
       enumivo_assert(b == a, "fixed_point128 instances comparison with same number of decimals");
       enumivo_assert(c == a, "fixed_point128 instances with different number of decimals");
    }

    {
        // Various ways to create fixed_point64
       enumivo::fixed_point64<5> a(12345667);
       enumivo::fixed_point64<5> b(12345667);
       enumivo::fixed_point64<5> c(12345667);
       enumivo_assert(b == a, "fixed_point64 instances comparison with same number of decimals");
       enumivo_assert(c == a, "fixed_point64 instances with different number of decimals");
    }

    {
        // Various ways to create fixed_point32
       enumivo::fixed_point32<18> a(12345667);
       enumivo::fixed_point32<18> b(12345667);
       enumivo::fixed_point32<16> c(12345667);
       enumivo_assert(b == a, "fixed_point32 instances comparison with same number of decimals");
       enumivo_assert(c == a, "fixed_point32 instances with different number of decimals");
    }
}

void test_fixedpoint::test_addition()
{
    {
       // Various ways to create fixed_point32
       enumivo::fixed_point32<0> a(100);
       enumivo::fixed_point32<0> b(100);
       enumivo::fixed_point32<0> c = a + b;
       enumivo::fixed_point32<0> d = 200;
       enumivo_assert(c == d, "fixed_point32 instances addition with zero decmimals");
    }
    {
       // Various ways to create fixed_point64
       enumivo::fixed_point64<0> a(100);
       enumivo::fixed_point64<0> b(100);
       enumivo::fixed_point64<0> c = a + b;
       enumivo::fixed_point64<0> d = 200;
       enumivo_assert(c == d, "fixed_point64 instances addition with zero decmimals");
    }
};

void test_fixedpoint::test_subtraction()
{
    {
       // Various ways to create fixed_point64
       enumivo::fixed_point64<0> a(100);
       enumivo::fixed_point64<0> b(100);
       enumivo::fixed_point64<0> c = a - b;
       enumivo::fixed_point64<0> d = 0;
       enumivo_assert(c == d, "fixed_point64 instances subtraction with zero decmimals");

       enumivo::fixed_point64<0> a1(0);
       enumivo::fixed_point64<0> c1 = a1 - b;
       enumivo::fixed_point64<0> d1 = -100;
       enumivo_assert(c1 == d1, "fixed_point64 instances subtraction with zero decmimals");
    }
    {
       // Various ways to create fixed_point32
       enumivo::fixed_point32<0> a(100);
       enumivo::fixed_point32<0> b(100);
       enumivo::fixed_point32<0> c = a - b;
       enumivo::fixed_point32<0> d = 0;
       enumivo_assert(c == d, "fixed_point32 instances subtraction with zero decmimals");

       // Various ways to create fixed_point32
       enumivo::fixed_point32<0> a1(0);
       enumivo::fixed_point32<0> c1 = a1 - b;
       enumivo::fixed_point32<0> d1 = -100;
       enumivo_assert(c1 == d1, "fixed_point32 instances subtraction with zero decmimals");

    }
};

void test_fixedpoint::test_multiplication()
{
    {
       // Various ways to create fixed_point64
       enumivo::fixed_point64<0> a(100);
       enumivo::fixed_point64<0> b(200);
       enumivo::fixed_point128<0> c = a * b;
       enumivo::fixed_point128<0> d(200*100);
       enumivo_assert(c == d, "fixed_point64 instances multiplication result in fixed_point128");
    }

    {
       // Various ways to create fixed_point32
       enumivo::fixed_point32<0> a(100);
       enumivo::fixed_point32<0> b(200);
       enumivo::fixed_point64<0> c = a * b;
       enumivo::fixed_point64<0> d(200*100);
       enumivo_assert(c == d, "fixed_point32 instances multiplication result in fixed_point64");
    }
}

void test_fixedpoint::test_division()
{
    {
        uint64_t lhs = 10000000;
        uint64_t rhs = 333;

        enumivo::fixed_point64<0> a((int64_t)lhs);
        enumivo::fixed_point64<0> b((int64_t)rhs);
        enumivo::fixed_point128<5> c = a / b;

        enumivo::fixed_point128<5> e = enumivo::fixed_divide<5>(lhs, rhs);
        print(e);
        enumivo_assert(c == e, "fixed_point64 instances division result from operator and function and compare in fixed_point128");

    }

    {
        uint32_t lhs = 100000;
        uint32_t rhs = 33;

        enumivo::fixed_point32<0> a((int32_t)lhs);
        enumivo::fixed_point32<0> b((int32_t)rhs);
        enumivo::fixed_point64<5> c = a / b;

        enumivo::fixed_point64<5> e = enumivo::fixed_divide<5>(lhs, rhs);
        enumivo_assert(c == e, "fixed_point64 instances division result from operator and function and compare in fixed_point128");

    }
}

void test_fixedpoint::test_division_by_0()
{
    {
        uint64_t lhs = 10000000;
        uint64_t rhs = 0;

        enumivo::fixed_point64<0> a((int64_t)lhs);
        enumivo::fixed_point64<0> b((int64_t)rhs);

        enumivo::fixed_point128<5> e = enumivo::fixed_divide<5>(lhs, rhs);
        // in order to get rid of unused parameter warning
        e = 0;
        enumivo_assert(false, "should've thrown an error");

    }

 }
