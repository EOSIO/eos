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

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <fc/io/fstream.hpp>


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

vector<uint8_t> assemble_wast( const std::string& wast ) {
   //   std::cout << "\n" << wast << "\n";
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
   {
      // Print any parse errors;
      std::cerr << "Error parsing WebAssembly text file:" << std::endl;
      for(auto& error : parseErrors)
      {
         std::cerr << ":" << error.locus.describe() << ": " << error.message.c_str() << std::endl;
         std::cerr << error.locus.sourceLine << std::endl;
         std::cerr << std::setw(error.locus.column(8)) << "^" << std::endl;
      }
      FC_ASSERT( !"error parsing wast" );
   }

   try
   {
      // Serialize the WebAssembly module.
      Serialization::ArrayOutputStream stream;
      WASM::serialize(stream,module);
      return stream.getBytes();
   }
   catch(Serialization::FatalSerializationException exception)
   {
      std::cerr << "Error serializing WebAssembly binary file:" << std::endl;
      std::cerr << exception.message << std::endl;
      throw;
   }
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

fc::variant push_transaction( SignedTransaction& trx ) {
    auto info = get_info();
    trx.expiration = info.head_block_time + 100; //chain.head_block_time() + 100; 
    trx.set_reference_block(info.head_block_id);
    boost::sort( trx.scope );

    return call( "localhost", 8888, "/v1/chain/push_transaction", trx );
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
                         vector<types::AccountPermission>{{creator, "active"}},
                         "newaccount", types::newaccount{creator, newaccount, owner_auth, 
                                                         active_auth, recovery_auth, deposit}); 

      std::cout << fc::json::to_pretty_string( push_transaction(trx)  );
}






/**
 *   Usage:
 *
 *   eosc create wallet walletname  ***PASS1*** ***PASS2*** 
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
   string command = "help";

   if( args.size() > 1 ) {
      command = args[1];
   }                                                                                                                               
   if( command == "help" ) {
      std::cout << "Command list: info, block, exec, account, push-trx, setcode, transfer, create, import, unlock, lock, and do\n";
      return -1;                                                                                                                   
   }                                                                                                                               
   if( command == "info" ) {
      std::cout << fc::json::to_pretty_string( get_info() );
   }
   else if( command == "block" ) {
      FC_ASSERT( args.size() == 3 );
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/get_block", fc::mutable_variant_object( "block_num_or_id", args[2]) ) );
   }
   else if( command == "exec" ) {
      FC_ASSERT( args.size() >= 7 );
      Name code   = args[2];
      Name action = args[3];
      auto& json   = args[4];

      auto result = call( "localhost", 8888, "/v1/chain/abi_json_to_bin", fc::mutable_variant_object( "code", code )("action",action)("args", fc::json::from_string(json)) );

      SignedTransaction trx;
      trx.messages.resize(1);
      auto& msg = trx.messages.back();
      msg.code = code;
      msg.type = action;
      msg.authorization = fc::json::from_string( args[6] ).as<vector<types::AccountPermission>>();
      msg.data = result.get_object()["binargs"].as<Bytes>();
      trx.scope = fc::json::from_string( args[5] ).as<vector<Name>>();

      auto trx_result = push_transaction( trx );
      std::cout << fc::json::to_pretty_string( trx_result ) <<"\n";

   } else if( command == "account" ) {
      FC_ASSERT( args.size() == 3 );
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/get_account", fc::mutable_variant_object( "name", args[2] ) ) );
   }
   else if( command == "push-trx" ) {
      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/push_transaction", 
                                                     fc::json::from_string(args[2])) );
   } else if ( command == "setcode" ) {
      if( args.size() == 2 ) {
         std::cout << "usage: "<<args[0] << " " << args[1] <<" ACCOUNT FILE.WAST FILE.ABI\n";
         return -1;
      }
      Name account(args[2]);
      const auto& wast_file = args[3];
      std::string wast;

      FC_ASSERT( fc::exists(wast_file) );
      fc::read_file_contents( wast_file, wast );
      auto wasm = assemble_wast( wast );


      
      types::setcode handler;
      handler.account = account;
      handler.code.resize(wasm.size());
      memcpy( handler.code.data(), wasm.data(), wasm.size() );

      if( args.size() == 5 ) {
         handler.abi = fc::json::from_file( args[4] ).as<types::Abi>();
      }

      SignedTransaction trx;
      trx.scope = { config::EosContractName, account };
      trx.emplaceMessage( config::EosContractName,
                          vector<types::AccountPermission>{ {account,"active"} },
                          "setcode", handler );

      std::cout << fc::json::to_pretty_string( push_transaction(trx)  );

   }else if( command == "transfer" ) {
      FC_ASSERT( args.size() == 5 );

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

      std::cout << fc::json::to_pretty_string( call( "localhost", 8888, "/v1/chain/push_transaction", trx ) );
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
