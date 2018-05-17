#include <boost/test/unit_test.hpp>

#include "fifo.h"

using namespace eosio;

BOOST_AUTO_TEST_SUITE(fifo_test)

BOOST_AUTO_TEST_CASE(pop_empty_fifo_not_blocking)
{
    fifo<int> f(fifo<int>::behavior::not_blocking);
    auto v = f.pop_all();
    BOOST_TEST(v.size() == 0);
}

BOOST_AUTO_TEST_CASE(change_to_not_blocking)
{
    fifo<int> f(fifo<int>::behavior::blocking);
    f.push(1);
    f.push(2);
    f.push(3);
    auto v = f.pop_all();
    BOOST_TEST(v.size() == 3);
    f.set_behavior(fifo<int>::behavior::not_blocking);
    v = f.pop_all();
    BOOST_TEST(v.size() == 0);
}

BOOST_AUTO_TEST_CASE(pushing_2_int_pop_2_int)
{
    fifo<int> f(fifo<int>::behavior::not_blocking);
    f.push(1);
    f.push(2);
    auto v = f.pop_all();
    BOOST_TEST(1 == v.at(0));
    BOOST_TEST(2 == v.at(1));
}

BOOST_AUTO_TEST_SUITE_END()
