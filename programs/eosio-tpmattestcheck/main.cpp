#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>

#include <fc/io/json.hpp>

#include <eosio/tpm-helpers/tpm-helpers.hpp>

namespace bpo = boost::program_options;

int main(int argc, char** argv) {
   bpo::options_description cli("eosio-tpmattestcheck command line options");

   bool help = false;
   std::vector<boost::filesystem::path> input_paths;
   std::vector<std::string> pcrs;

   bpo::positional_options_description pod;
   pod.add("attest-json", -1);

   cli.add_options()
      ("help,h", bpo::bool_switch(&help)->default_value(false), "Print this help message and exit.")
      ("attest-json,i", bpo::value<std::vector<boost::filesystem::path>>()->notifier([&](const std::vector<boost::filesystem::path>& p) {
         input_paths = p;
      }), "JSON created from eosio-tpmtool's attestation. May be specified multiple times to validate multiple attestations. The attesting key will be output.")
      ("pcr,p", bpo::value<std::vector<std::string>>()->composing()->notifier([&](const std::vector<std::string>& p) {
         pcrs = p;
      }), "Expect a given PCR's sha256 to be present in the policy of the created key. May be specified multiple times. "
          "Format is pcr:hash, for example 7:12ab4s...")
      ;
   bpo::variables_map varmap;
   try {
      bpo::store(bpo::command_line_parser(argc, argv).options(cli).positional(pod).run(), varmap);
      bpo::notify(varmap);
   }
   catch(fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return 1;
   }

   if(input_paths.empty() || help) {
      std::ostream& outs = help ? std::cout : std::cerr;
      outs << "eosio-tpmattestcheck verifies creation certifications made via eosio-tpmtool's attest function" << std::endl << std::endl;
      cli.print(outs);
      return !help;
   }

   try {
      std::map<unsigned, fc::sha256> expected_pcr_in_policy;
      for(const std::string& p : pcrs) {
         size_t delim = p.find(":");
         FC_ASSERT(delim != std::string::npos, "PCR specification is missing colon");
         FC_ASSERT(delim != 0, "PCR specification is missing PCR index");
         FC_ASSERT(delim + 1 != p.size(), "PCR specification is missing sha256 value");
         expected_pcr_in_policy[std::stoi(p.substr(0, delim))] = fc::sha256(p.substr(delim + 1));
      }

      for(const boost::filesystem::path& p : input_paths) {
         eosio::tpm::attested_key ak = fc::json::from_file<decltype(ak)>(p);

         std::cout << eosio::tpm::verify_attestation(ak, expected_pcr_in_policy).to_string() << std::endl;
      }
      return 0;
   } FC_LOG_AND_DROP();

   return 1;
}
