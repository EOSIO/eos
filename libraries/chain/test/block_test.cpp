#include <boost/test/unit_test.hpp>

#include "eos/chain/block.hpp"

BOOST_AUTO_TEST_SUITE(block_test)

BOOST_AUTO_TEST_CASE(block_header_default_digest)
{
    eosio::chain::block_header header;
    std::string result = header.digest().str();
    BOOST_CHECK_EQUAL("075561eff2cd3ad586776fa904f0040282c5f6a261f6a8fd6a0a524d14cd2d2c", result);
}

BOOST_AUTO_TEST_SUITE_END()



