#include <boost/test/unit_test.hpp>

#include "fifo.h"

using namespace eosio;

BOOST_AUTO_TEST_SUITE(fifo_test)

BOOST_AUTO_TEST_CASE(pushing_2_int_pop_2_int)
{
    fifo<int> f;
    f.push(1);
    f.push(2);
    BOOST_TEST(1 == f.pop());
    BOOST_TEST(2 == f.pop());
}

BOOST_AUTO_TEST_SUITE_END()
