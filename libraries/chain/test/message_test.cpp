#include <boost/test/unit_test.hpp>

#include "eos/chain/message.hpp"

BOOST_AUTO_TEST_SUITE(message_test)

BOOST_AUTO_TEST_CASE(default_message)
{
    eosio::chain::message message;
    std::string result = message.as<std::string>();
    BOOST_CHECK_EQUAL("", result);
}

BOOST_AUTO_TEST_SUITE_END()




