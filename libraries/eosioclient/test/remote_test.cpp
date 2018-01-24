#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <string>

#include "remote.hpp"

BOOST_AUTO_TEST_SUITE(remote_test)

BOOST_AUTO_TEST_CASE(instance)
{
    BOOST_CHECK_NO_THROW(eosio::client::Remote());
}

BOOST_AUTO_TEST_CASE(default_host)
{
    eosio::client::Remote remote;
    BOOST_CHECK_EQUAL("localhost", remote.host());
}

BOOST_AUTO_TEST_CASE(default_port)
{
    eosio::client::Remote remote;
    BOOST_CHECK_EQUAL(8888, remote.port());
}

BOOST_AUTO_TEST_CASE(set_host)
{
    eosio::client::Remote remote;
    std::string host("192.168.0.1");
    remote.set_host(host);
    BOOST_CHECK_EQUAL(host, remote.host());
}

BOOST_AUTO_TEST_CASE(set_port)
{
    eosio::client::Remote remote;
    uint32_t port(1111);
    remote.set_port(port);
    BOOST_CHECK_EQUAL(port, remote.port());
}

BOOST_AUTO_TEST_CASE(call)
{
    eosio::client::Remote remote;
    remote.set_host("localhost");
    remote.set_port(1111);
    remote.call("ciao", "");
    //BOOST_CHECK_EQUAL(port, remote.port());
}

BOOST_AUTO_TEST_SUITE_END()


