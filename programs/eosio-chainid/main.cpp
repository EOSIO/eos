#include <iostream>
#include <string>

#include <fc/io/json.hpp>
#include <eosio/chain/genesis_state.hpp>

int main(int argc, char** argv) {
   bool is_help = argc == 2 && std::string(argv[1]) == "--help";
   if(argc != 2 || is_help) {
      std::ostream& outs = is_help ? std::cout : std::cerr;
      outs << "Given a genesis JSON outputs the chain id" << std::endl;
      outs << "  Usage: eosio-chainid genesis.json" << std::endl;
      outs << std::endl;
      return is_help ? 0 : 1;
   }

   try {
      eosio::chain::genesis_state gen;
      fc::json::from_file(argv[1]).as<eosio::chain::genesis_state>(gen);

      std::cout << (std::string)gen.compute_chain_id() << std::endl;
   } FC_LOG_AND_RETHROW();
   return 0;
}