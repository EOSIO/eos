#include <boost/test/unit_test.hpp>

#include "eosio/chain/block.hpp"

BOOST_AUTO_TEST_SUITE(block_test)

BOOST_AUTO_TEST_CASE(block_header_default_digest)
{
    eosio::chain::block_header header;
    std::string result = header.digest().str();
    BOOST_CHECK_EQUAL("dc2021c180e2d8367d094b4c07d11bd556d64b33d1fe8bf58e208e8da8f5dd55", result);
}

BOOST_AUTO_TEST_SUITE_END()



