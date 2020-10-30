#include <boost/program_options.hpp>

#include <iostream>

#include <eosio/tpm-helpers/tpm-helpers.hpp>

namespace bpo = boost::program_options;

int main(int argc, char** argv) {
   bpo::options_description cli("eosio-tpmtool command line options");

   bool help = false, list = false, create = false;
   std::string tcti;
   std::vector<unsigned> pcrs;

   cli.add_options()
      ("help,h", bpo::bool_switch(&help)->default_value(false), "Print this help message and exit.")
      ("list,l", bpo::bool_switch(&list)->default_value(false), "List persistent TPM keys usable for EOSIO.")
      ("create,c", bpo::bool_switch(&create)->default_value(false), "Create persistent TPM key.")
      ("tcti,T", bpo::value<std::string>()->notifier([&](const std::string& s) {
         tcti = s;
      }), "Specify tcti and tcti options.")
      ("pcr,p", bpo::value<std::vector<unsigned>>()->composing()->notifier([&](const std::vector<unsigned>& p) {
         pcrs = p;
      }), "Add a PCR value to the policy of the created key. May be specified multiple times.")
      ;
   bpo::variables_map varmap;
   try {
      bpo::store(bpo::parse_command_line(argc, argv, cli), varmap);
      bpo::notify(varmap);
   }
   catch(fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return 1;
   }

   if((!list && !create) || help) {
      std::ostream& outs = help ? std::cout : std::cerr;
      outs << "eosio-tpmtool is a helper for listing and creating keys in a TPM" << std::endl << std::endl;
      cli.print(outs);
      return help ? 0 : 1;
   }

   if(!(list ^ create)) {
      std::cerr << "Only one option may be specified to eosio-tpmtool" << std::endl;
      return 1;
   }

   try {
      if(create)
         std::cout << eosio::tpm::create_key(tcti, pcrs).to_string() << std::endl;

      if(list)
         for(const auto& k : eosio::tpm::get_all_persistent_keys(tcti))
            std::cout << k.to_string() << std::endl;

      return 0;
   } FC_LOG_AND_DROP();

   return 1;
}
