#include <boost/test/unit_test.hpp>
#include <b1/session/boost_memory_allocator.hpp>

using namespace eosio::session;

BOOST_AUTO_TEST_SUITE(boost_memory_allocator_tests)

BOOST_AUTO_TEST_CASE(malloc_free_test)  {
    auto allocator = boost_memory_allocator::make();

    auto size = size_t{512};
    auto* chunk = allocator->malloc(size);
    BOOST_REQUIRE(chunk);
    allocator->free(chunk, size);

    chunk = allocator->malloc(size);
    BOOST_REQUIRE(chunk);
    allocator->free_function()(chunk, size);
}

BOOST_AUTO_TEST_CASE(equality_test) {
  auto allocator_1 = boost_memory_allocator::make();
  auto allocator_2 = allocator_1;
  auto allocator_3 = boost_memory_allocator::make();
  auto allocator_4 = allocator_3;

  BOOST_REQUIRE(allocator_1->equals(*allocator_1));
  BOOST_REQUIRE(allocator_1->equals(*allocator_2));
  BOOST_REQUIRE(allocator_3->equals(*allocator_3));
  BOOST_REQUIRE(allocator_3->equals(*allocator_4));
  BOOST_REQUIRE(!allocator_1->equals(*allocator_3));
  BOOST_REQUIRE(!allocator_1->equals(*allocator_4));
}

BOOST_AUTO_TEST_CASE(multiple_allocations) {
  auto allocator = boost_memory_allocator::make();
  for (size_t i = 0; i < 256; ++i) {
    static auto size = size_t{40};
    auto* chunk = allocator->malloc(size);
    BOOST_REQUIRE(chunk);
  }
}

BOOST_AUTO_TEST_SUITE_END();