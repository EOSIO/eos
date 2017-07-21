#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

using namespace std;


fc::variant call( const std::string& server, uint16_t port, 
                  const std::string& path,
                  const fc::variant& postdata = fc::variant() );

/**
 *   Usage:
 *
 *   eocs create wallet walletname  ***PASS1*** ***PASS2*** 
 *   eosc unlock walletname  ***PASSWORD*** 
 *   eosc wallets -> prints list of wallets with * next to currently unlocked
 *   eosc keys -> prints list of private keys
 *   eosc importkey privatekey -> loads keys
 *   eosc accounts -> prints list of accounts that reference key
 *   eosc lock
 *   eosc do contract action { from to amount }
 *   eosc transfer from to amount memo  => aliaze for eosc 
 *   eosc create account creator 
 */
int main( int argc, char** argv ) {
   try {
   vector<string> args(argc);
   for( uint32_t i = 0; i < argc; ++i ) {
      args[i] = string(argv[i]);
   }
   const auto& command = args[1];
   if( command == "info" ) {
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/get_info" ) );
   }
   else if( command == "block" ) {
      FC_ASSERT( args.size() == 3 );
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/get_block", fc::mutable_variant_object( "block_num_or_id", args[2]) ) );
   }
   if( command == "unlock" ) {

   } else if( command == "lock" ) {

   } else if( command == "do" ) {

   }
   } catch ( const fc::exception& e ) {
      std::cerr << e.to_detail_string() <<"\n";
   }
   std::cout << "\n";

   return 0;
}
