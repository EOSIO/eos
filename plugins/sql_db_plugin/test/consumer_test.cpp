#include <boost/test/unit_test.hpp>

#include "consumer.h"

using namespace eosio;

BOOST_AUTO_TEST_SUITE(consumer_test)

BOOST_AUTO_TEST_CASE(instantiate)
{
    consumer<int>::consume_function f = [](const std::vector<int>& elements){
        for ( auto element : elements)
            std::cout << element << std::endl;
    };
    consumer<int> c(f);

    c.push(1);
    c.push(2);
    c.push(3);

    std::this_thread::sleep_for(std::chrono::seconds(3));
}

BOOST_AUTO_TEST_SUITE_END()
