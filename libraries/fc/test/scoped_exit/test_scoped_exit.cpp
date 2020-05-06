#include <fc/scoped_exit.hpp>

#define BOOST_TEST_MODULE scoped_exit
#include <boost/test/included/unit_test.hpp>

using namespace fc;

BOOST_AUTO_TEST_SUITE(scoped_exit_test_suite)

BOOST_AUTO_TEST_CASE(scoped_exit_test)
{
   bool result = false;
   {
      auto g1 = make_scoped_exit([&]{ result = true; });
      BOOST_TEST(result == false);
   }
   BOOST_TEST(result == true);
}

BOOST_AUTO_TEST_CASE(cancel) {
   bool result = false;
   {
      auto g1 = make_scoped_exit([&]{ result = true; });
      BOOST_TEST(result == false);
      g1.cancel();
   }
   BOOST_TEST(result == false);
}

BOOST_AUTO_TEST_CASE(test_move) {
   bool result = false;
   {
      auto g1 = make_scoped_exit([&]{ result = true; });
      BOOST_TEST(result == false);
      {
         auto g2 = std::move(g1);
         BOOST_TEST(result == false);
      }
      BOOST_TEST(result == true);
      result = false;
   }
   BOOST_TEST(result == false);
}

struct move_only {
   move_only() = default;
   move_only(move_only&&) = default;
   void operator()() {}
};

BOOST_AUTO_TEST_CASE(test_forward) {
   auto g = make_scoped_exit(move_only{});
   auto g2 = std::move(g);
}

BOOST_AUTO_TEST_SUITE_END()
