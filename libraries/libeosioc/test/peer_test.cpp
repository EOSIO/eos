#include <boost/test/unit_test.hpp>

#include <eosio/libeosioc/peer.hpp>

using namespace eosio::client;

BOOST_AUTO_TEST_SUITE(peer_test)

BOOST_AUTO_TEST_CASE(instance)
{
    BOOST_CHECK_NO_THROW(eosio::client::Peer());
}

BOOST_AUTO_TEST_CASE(defaults)
{
    Peer peer;
    BOOST_CHECK_EQUAL("localhost", peer.host());
    BOOST_CHECK_EQUAL(8888, peer.port());
}

BOOST_AUTO_TEST_CASE(set_host)
{
    Peer peer;
    std::string host("192.168.1.1");
    peer.set_host(host);
    BOOST_CHECK_EQUAL(host, peer.host());
}

BOOST_AUTO_TEST_CASE(set_port)
{
    Peer peer;
    int port(1111);
    peer.set_port(port);
    BOOST_CHECK_EQUAL(port, peer.port());
}

//BOOST_AUTO_TEST_CASE(get_block)
//{
//    eosio::client::Peer peer;
//    peer.set_port(8888);
//    peer.set_host("localhost");
//    auto result0 = peer.get_block("5");
//    auto result1 = peer.get_block("5");
//    BOOST_CHECK_EQUAL(fc::json::to_string(result0), fc::json::to_string(result1));
//}

BOOST_AUTO_TEST_SUITE_END()
