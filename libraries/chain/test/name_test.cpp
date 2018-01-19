#include <boost/test/unit_test.hpp>

#include "eosio/chain/name.hpp"

BOOST_AUTO_TEST_SUITE(name_test)

BOOST_AUTO_TEST_CASE(default_to_string)
{
    eosio::chain::name name;
    std::string result = name.to_string();
    BOOST_CHECK_EQUAL("", result);
}

BOOST_AUTO_TEST_SUITE_END()


