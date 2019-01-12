/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <iostream>

#include <fc/filesystem.hpp>
#include <fc/stacktrace.hpp>


int show_dump(const std::string& name, const fc::path& dump) {
   if (fc::has_btrace(dump)) {
      std::cout
         << "Last " << name << " run crashed:" << std::endl
         << fc::get_btrace(dump) << std::endl << std::endl;
      return 1;
   }
   return 0;
}

int main() {
   auto root = fc::app_path();
   int result = 0;

   result += show_dump("nodeos", root / "eosio/nodeos/backtrace.dmp");
   result += show_dump("keosd",  root / "eosio/keosd/backtrace.dmp" );
   result += show_dump("cleos",  root / "eosio/cleos/backtrace.dmp" );

   if (!result) {
       std::cout << "No backtrace dumps" << std::endl;
   }

   return 0;
}