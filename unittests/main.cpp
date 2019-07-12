/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <cstdlib>

#include <iostream>

#include <eosio/chain/exceptions.hpp>

#include <fc/log/logger.hpp>

#include <boost/test/included/unit_test.hpp>

//extern uint32_t EOS_TESTING_GENESIS_TIMESTAMP;

void translate_fc_exception(const fc::exception &e) {
   std::cerr << "\033[33m" <<  e.to_detail_string() << "\033[0m" << std::endl;
   BOOST_TEST_FAIL("Caught Unexpected Exception");
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
   fc::logger::get(DEFAULT_LOGGER).set_log_level(fc::log_level::debug);

   // Register fc::exception translator
   boost::unit_test::unit_test_monitor.register_exception_translator<fc::exception>(&translate_fc_exception);

   std::srand(time(NULL));
   std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
   /*
   const char* genesis_timestamp_str = getenv("EOS_TESTING_GENESIS_TIMESTAMP");
   if( genesis_timestamp_str != nullptr )
   {
      EOS_TESTING_GENESIS_TIMESTAMP = std::stoul( genesis_timestamp_str );
   }
   std::cout << "EOS_TESTING_GENESIS_TIMESTAMP is " << EOS_TESTING_GENESIS_TIMESTAMP << std::endl;
   */
   return nullptr;
}
