#include <boost/test/unit_test.hpp>

#include "wallet.hpp"

using namespace eosio::client;

BOOST_AUTO_TEST_SUITE(wallet_test)

BOOST_AUTO_TEST_CASE(instance)
{
    BOOST_CHECK_NO_THROW(eosio::client::Wallet());
}

BOOST_AUTO_TEST_CASE(defaults)
{
    Wallet wallet;
    BOOST_CHECK_EQUAL("localhost", wallet.host());
    BOOST_CHECK_EQUAL(8888, wallet.port());
}

BOOST_AUTO_TEST_CASE(set_host)
{
    Wallet wallet;
    std::string host("192.168.1.1");
    wallet.set_host(host);
    BOOST_CHECK_EQUAL(host, wallet.host());
}

BOOST_AUTO_TEST_CASE(set_port)
{
    Wallet wallet;
    int port(1111);
    wallet.set_port(port);
    BOOST_CHECK_EQUAL(port, wallet.port());
}

BOOST_AUTO_TEST_SUITE_END()


