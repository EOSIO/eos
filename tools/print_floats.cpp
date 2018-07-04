/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <iostream>
#include <iomanip>
#include <limits>
#include <random>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, const char **argv) {

   unsigned int seed = std::mt19937::default_seed;

   unsigned int num_cases = 10;


   po::options_description desc("Options");
   desc.add_options()
      ("help,h", "Print this help message and exit")
      ("seed,s", po::value<unsigned int>(&seed)->default_value(seed), "random seed")
      ("num-cases,n", po::value<unsigned int>(&num_cases)->default_value(num_cases), "number of test cases to generate" )
      ("avoid-specials", po::bool_switch()->default_value(false), "Avoid printing test cases for special values of double" )
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);

   if( vm.count("help") ) {
      std::cout << desc << std::endl;
      return 1;
   }

   std::cout << "Seed: " << seed << std::endl;
   std::mt19937 gen(seed);

   double lbound = std::numeric_limits<double>::lowest();
   double ubound = std::numeric_limits<double>::max();

   std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());

   std::cout.precision(std::numeric_limits<double>::digits10 + 1);

   auto print_triplet = [&]( double d, int left_padding = 0 ) {
      std::cout << std::setw(left_padding) << "";
      std::cout << "[\"0x" << std::hex << *(uint64_t*)(&d) << "\", \""
                << std::hexfloat << d << "\", \"" << std::fixed << d << "\"]";
   };

   if( num_cases > 0 ) {
      std::cout << std::endl;
      std::cout <<    "Randomly generated double test cases (raw hex dump, hexidecimal float, decimal fixed) "
                      "within the interval [" << std::fixed << lbound << ", " << ubound << "]:" << std::endl
                <<    " [" << std::endl;
      for( int n = 1, end = num_cases; n <= end; )
      {

         uint64_t x = dis(gen);
         double v = *(double*)(&x);

         if( !(lbound <= v && v <= ubound) ) continue;

         print_triplet(v, 2);
         if( n != end )
            std::cout << ",";

         std::cout << std::endl;

         ++n;
      }
      std::cout << "]" << std::endl;
   }

   if( !vm["avoid-specials"].as<bool>() ) {
      std::cout << std::endl;
      std::cout << "Special double value test cases (raw hex dump, hexidecimal float, decimal fixed):" << std::endl;

      using nld = std::numeric_limits<double>;

      std::cout << std::endl << " lowest():        "; print_triplet(nld::lowest());
      std::cout << std::endl << " min():           "; print_triplet(nld::min());
      std::cout << std::endl << " max():           "; print_triplet(nld::max());
      std::cout << std::endl << " epsilon():       "; print_triplet(nld::epsilon());
      std::cout << std::endl << " round_error():   "; print_triplet(nld::round_error());
      std::cout << std::endl << " +0:              "; print_triplet(0.0);
      std::cout << std::endl << " -0:              "; print_triplet(-0.0);
      std::cout << std::endl << " infinity():      "; print_triplet(nld::infinity());
      std::cout << std::endl << " -infinity():     "; print_triplet(-nld::infinity());
      std::cout << std::endl << " quiet_NaN():     "; print_triplet(nld::quiet_NaN());
      std::cout << std::endl << " signaling_NaN(): "; print_triplet(nld::signaling_NaN());
      std::cout << std::endl << " denorm_min():    "; print_triplet(nld::denorm_min());
      std::cout << std::endl;

   }

   return 0;
}
