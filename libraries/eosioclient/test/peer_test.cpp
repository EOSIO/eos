#include <boost/test/unit_test.hpp>

#include "peer.hpp"

BOOST_AUTO_TEST_SUITE(peer_test)

BOOST_AUTO_TEST_CASE(instance)
{
    BOOST_CHECK_NO_THROW(eosio::client::Peer());
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
