#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>

#include <eos/chain/config.hpp>
#include <eos/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>
#include <boost/range/algorithm/sort.hpp>


using namespace std;
using namespace eos;
using namespace eos::chain;
using namespace eos::utilities;

inline std::vector<Name> sort_names( std::vector<Name>&& names ) {
   std::sort( names.begin(), names.end() );
   auto itr = std::unique( names.begin(), names.end() );
   names.erase( itr, names.end() );
   return names;
}

fc::variant call( const std::string& server, uint16_t port, 
                  const std::string& path,
                  const fc::variant& postdata = fc::variant() );

template<typename T>
fc::variant call( const std::string& server, uint16_t port, 
                  const std::string& path,
                  const T& v ) { return call( server, port, path, fc::variant(v) ); }

eos::chain_apis::read_only::get_info_results get_info() {
  return call( "localhost", 8888, "/v1/chain/get_info" ).as<eos::chain_apis::read_only::get_info_results>();
}

std::pair<fc::variant,fc::variant> push_transaction( SignedTransaction& trx ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100; 
    trx.set_reference_block(info.head_block_id);
    boost::sort( trx.scope );

    return std::make_pair( call( "localhost", 8888, "/v1/chain/push_transaction", trx ), fc::variant(trx) );
}


void create_account( const vector<string>& args ) {
      Name creator(args[3]);
      Name newaccount(args[4]);
      Name eosaccnt(config::EosContractName);
      Name staked("staked");

      public_key_type owner_key(args[5]);
      public_key_type active_key(args[6]);

      auto owner_auth   = eos::chain::Authority{1, {{owner_key, 1}}, {}};
      auto active_auth  = eos::chain::Authority{1, {{active_key, 1}}, {}};
      auto recovery_auth = eos::chain::Authority{1, {}, {{{creator, "active"}, 1}}};

      uint64_t deposit = 1;

      SignedTransaction trx;
      trx.scope = sort_names({creator,eosaccnt}); 
      trx.emplaceMessage(config::EosContractName, 
                         vector<types::AccountPermission>{ {creator,"active"} }, 
                         "newaccount", types::newaccount{creator, newaccount, owner_auth, 
                                                         active_auth, recovery_auth, deposit}); 

      std::cout << fc::json::to_pretty_string( push_transaction(trx)  );
}






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
      std::cout << fc::json::to_pretty_string( get_info() );
   }
   else if( command == "block" ) {
      FC_ASSERT( args.size() == 3 );
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/get_block", fc::mutable_variant_object( "block_num_or_id", args[2]) ) );
   }
   else if( command == "push-trx" ) {
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/push_transaction", 
                                                     fc::json::from_string(args[2])) );
   } else if( command == "transfer" ) {
      Name sender(args[2]);
      Name recipient(args[3]);
      uint64_t amount = fc::variant(argv[4]).as_uint64();

      SignedTransaction trx;
      trx.scope = sort_names({sender,recipient}); 
      trx.emplaceMessage(config::EosContractName, 
                         vector<types::AccountPermission>{ {sender,"active"} }, 
                         "transfer", types::transfer{sender, recipient, amount/*, memo*/}); 
      auto info = get_info();
      trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100; 
      trx.set_reference_block(info.head_block_id);

      std::cout << fc::json::to_pretty_string( fc::variant( std::make_pair( trx, call( "localhost", 8888, "/v1/chain/push_transaction", trx ) ) ) );
   } else if (command == "create" ) {
      if( args[2] == "account" ) {
         if( args.size() < 7 ) {
            std::cerr << "usage: " << args[0] << " create account CREATOR NEWACCOUNT OWNERKEY ACTIVEKEY\n";
            return -1;
         }
         create_account( args );
      }
      else if( args[2] == "key" ) {
         auto priv = fc::ecc::private_key::generate();
         auto pub = public_key_type( priv.get_public_key() );

         std::cout << "public: " << string(pub) <<"\n";
         std::cout << "private: " << key_to_wif(priv.get_secret()) <<"\n";
      }
   } else if( command == "import" ) {
      if( args[2] == "key" ) {
        auto secret = wif_to_key( args[3] ); //fc::variant( args[3] ).as<fc::ecc::private_key_secret>();
        if( !secret ) {
           std::cerr << "invalid WIF private key\n";
           return -1;
        }
        auto priv = fc::ecc::private_key::regenerate(*secret);
        auto pub = public_key_type( priv.get_public_key() );
        std::cout << "public: " << string(pub) <<"\n";
      }
   }else if( command == "unlock" ) {

   } else if( command == "lock" ) {

   } else if( command == "do" ) {

   }
   } catch ( const fc::exception& e ) {
      std::cerr << e.to_detail_string() <<"\n";
   }
   std::cout << "\n";

   return 0;
}
