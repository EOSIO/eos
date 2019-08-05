/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <cstdlib>

#include <iostream>

#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/wasm_interface.hpp>
#include <eosio/testing/tester.hpp>

#include <fc/log/logger.hpp>

#include <boost/test/unit_test_monitor.hpp>

void translate_fc_exception(const fc::exception &e) {
   std::cerr << "\033[33m" <<  e.to_detail_string() << "\033[0m" << std::endl;
   BOOST_TEST_FAIL("Caught Unexpected Exception");
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
   // Turn off blockchain logging if no --verbose parameter is not added
   // To have verbose enabled, call "tests/chain_test -- --verbose"
   bool is_verbose = false;
   const std::string verbose_arg = "--verbose";
   for (int i = 1; i < argc; i++) {
      const std::string this_arg(argv[i]);
      if (verbose_arg == this_arg) {
         is_verbose = true;
         continue;
      }
      if(this_arg.length() > 2) {
         std::istringstream ss(this_arg.substr(2));
         eosio::chain::wasm_interface::vm_type vm;
         ss >> vm;
         if(!ss.fail()) {
            eosio::testing::base_tester::tester_runtime = vm;
            continue;
         }
      }

      std::stringstream ss;
      ss << "Unknown option: " << argv[i];
      throw boost::unit_test::framework::setup_error(ss.str());
   }
   if(!is_verbose) fc::logger::get(DEFAULT_LOGGER).set_log_level(fc::log_level::off);

   // Register fc::exception translator
   boost::unit_test::unit_test_monitor.register_exception_translator<fc::exception>(&translate_fc_exception);

   std::srand(time(NULL));
   std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
   return nullptr;
}