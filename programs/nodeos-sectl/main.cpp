#include <boost/program_options.hpp>

#include <iostream>

#include <fc/crypto/public_key.hpp>
#include <eosio/se-helpers/se-helpers.hpp>

#include "config.hpp"

namespace bpo = boost::program_options;

namespace fc::crypto {
std::istream& operator>>(std::istream& in, fc::crypto::public_key& c) {
   std::string input;
   in >> input;
   c = fc::crypto::public_key(input);
   return in;
}
}

int main(int argc, char** argv) {
   using namespace eosio::nodeos_sectl::config;
   bpo::options_description cli(node_executable_name + "-sectl command line options");

   bool help = false, list = false, create = false;
   fc::crypto::public_key key_to_delete;

   cli.add_options()
      ("help,h", bpo::bool_switch(&help)->default_value(false), "Print this help message and exit.")
      ("list,l", bpo::bool_switch(&list)->default_value(false), "List Secure Enclave keys.")
      ("create,c", bpo::bool_switch(&create)->default_value(false), "Create Secure Enclave key.")
      ("delete,d", bpo::value<fc::crypto::public_key>()->notifier([&key_to_delete](fc::crypto::public_key p) {
         key_to_delete = p;
      })->value_name("pubkey"), "Delete a Secure Enclave key.");
   bpo::variables_map varmap;
   try {
      bpo::store(bpo::parse_command_line(argc, argv, cli), varmap);
      bpo::notify(varmap);
   } 
   catch ( const std::bad_alloc& ) {
     elog("bad alloc");
     return BAD_ALLOC;
   } 
   catch ( const boost::interprocess::bad_alloc& ) {
     elog("bad alloc");
     return BAD_ALLOC;
   } 
   catch(const fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return 1;
   }
   catch(const std::exception& e) {
      elog("${e}", ("e", fc::std_exception_wrapper::from_current_exception(e).to_detail_string()));
      return 1;
   }

   if((!list && !create && key_to_delete == fc::crypto::public_key()) || help) {
      std::ostream& outs = help ? std::cout : std::cerr;
      outs << node_executable_name << "-sectl manages the Secure Enclave keys available to " << node_executable_name << std::endl << std::endl;
      cli.print(outs);
      return help ? 0 : 1;
   }

   if(list + create + (key_to_delete != fc::crypto::public_key()) > 1) {
      std::cerr << "Only one option may be specified to " << node_executable_name << "-sectl" << std::endl;
      return 1;
   }

   if(!eosio::secure_enclave::application_signed()) {
      std::cerr << node_executable_name <<  "-sectl is not signed so it is unable to managed Secure Enclave keys" << std::endl;
      return 1;
   }
   if(!eosio::secure_enclave::hardware_supports_secure_enclave()) {
      std::cerr << "This device does not appear to have a Secure Enclave" << std::endl;
      return 1;
   }

   if(create)
      std::cout << eosio::secure_enclave::create_key().public_key().to_string() << std::endl;

   std::set<eosio::secure_enclave::secure_enclave_key> allkeys = eosio::secure_enclave::get_all_keys();

   if(key_to_delete != fc::crypto::public_key()) {
      auto it = allkeys.begin();
      for(; it != allkeys.end(); ++it) {
         if(it->public_key() == key_to_delete) {
            eosio::secure_enclave::delete_key(std::move(allkeys.extract(it).value()));
            break;
         }
      }
      if(it == allkeys.end()) {
         std::cerr << key_to_delete.to_string() << " not found in Secure Enclave" << std::endl;
         return 1;
      }
   }

   if(list)
      for(const auto& k : allkeys)
         std::cout << k.public_key().to_string() << std::endl;

   return 0;
}
