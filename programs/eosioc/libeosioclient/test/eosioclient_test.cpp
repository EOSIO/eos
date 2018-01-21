#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <string>

#include "eosioclient.hpp"

BOOST_AUTO_TEST_SUITE(eosioclient_test)

BOOST_AUTO_TEST_CASE(instance)
{
    BOOST_CHECK_NO_THROW(eosio::client::Eosioclient());
}

//BOOST_AUTO_TEST_CASE(get_info)
//{
//    eosio::client::Eosioclient client;
//    BOOST_CHECK_NO_THROW(client.get_info());
//}

BOOST_AUTO_TEST_SUITE_END()


