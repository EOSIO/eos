#include <boost/test/unit_test.hpp>

#include "runner.h"

using namespace eosio;

BOOST_AUTO_TEST_SUITE(consumer_test)

BOOST_AUTO_TEST_CASE(instantiate)
{
    struct foo : public consumer<int>
    {
    public:
        void consume(const std::vector<int> &blocks) override
        {
            for (int i : blocks)
                std::cout << i << std::endl;
        }
    };

    runner<int> c(std::make_unique<foo>());
    c.push(1);
    c.push(10);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

BOOST_AUTO_TEST_SUITE_END()
