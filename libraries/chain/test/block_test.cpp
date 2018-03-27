#include <boost/test/unit_test.hpp>

#include "eosio/chain/block.hpp"

BOOST_AUTO_TEST_SUITE(block_test)

BOOST_AUTO_TEST_CASE(block_header_default_digest)
{
    eosio::chain::block_header header;
    std::string result = header.digest().str();
    BOOST_CHECK_EQUAL("c38f1294a259a7e943728e76d1a9d2e0992d22f4cebf6de1fb42204e7126d19a", result);
}

BOOST_AUTO_TEST_SUITE_END()



