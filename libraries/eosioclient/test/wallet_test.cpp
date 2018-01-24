#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <string>

#include "wallet.hpp"

BOOST_AUTO_TEST_SUITE(wallet_test)

BOOST_AUTO_TEST_CASE(instance)
{
    BOOST_CHECK_NO_THROW(eosio::client::Wallet());
}

BOOST_AUTO_TEST_SUITE_END()


