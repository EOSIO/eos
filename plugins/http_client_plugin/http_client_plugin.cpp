/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/http_client_plugin/http_client_plugin.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

namespace eosio {

http_client_plugin::http_client_plugin():my(new http_client()){}
http_client_plugin::~http_client_plugin(){}

void http_client_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
      ("https-client-root-cert", boost::program_options::value<vector<string>>()->composing()->multitoken(),
       "PEM encoded trusted root certificate (or path to file containing one) used to validate any TLS connections made.  (may specify multiple times)\n")
      ("https-client-validate-peers", boost::program_options::value<bool>()->default_value(true),
       "true: validate that the peer certificates are valid and trusted, false: ignore cert errors")
      ;

}

void http_client_plugin::plugin_initialize(const variables_map& options) {
   if ( options.count("https-client-root-cert") ) {
      const std::vector<std::string> root_pems = options["https-client-root-cert"].as<std::vector<std::string>>();
      for (const auto& root_pem : root_pems) {
         std::string pem_str = root_pem;
         if (!boost::algorithm::starts_with(pem_str, "-----BEGIN CERTIFICATE-----\n")) {
            try {
               auto infile = std::ifstream(pem_str);
               std::stringstream sstr;
               sstr << infile.rdbuf();
               pem_str = sstr.str();
               FC_ASSERT(boost::algorithm::starts_with(pem_str, "-----BEGIN CERTIFICATE-----\n"), "File does not appear to be a PEM encoded certificate");
            } catch (const fc::exception& e) {
               elog("Failed to read PEM ${f} : ${e}", ("f", root_pem)("e",e.to_detail_string()));
            }
         }

         try {
            my->add_cert(pem_str);
         } catch (const fc::exception& e) {
            elog("Failed to read PEM : ${e} \n${pem}\n", ("pem", pem_str)("e",e.to_detail_string()));
         }
      }
   }

   my->set_verify_peers(options.at("https-client-validate-peers").as<bool>());
}

void http_client_plugin::plugin_startup() {

}

void http_client_plugin::plugin_shutdown() {

}

}
