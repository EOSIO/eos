#include <eosio/signature_provider_plugin/signature_provider_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/time.hpp>
#include <fc/network/url.hpp>

#include <boost/algorithm/string.hpp>

#ifdef __APPLE__
#include <eosio/se-helpers/se-helpers.hpp>
#endif
#ifdef ENABLE_TPM
#include <eosio/tpm-helpers/tpm-helpers.hpp>
#endif

namespace eosio {
   static appbase::abstract_plugin& _signature_provider_plugin = app().register_plugin<signature_provider_plugin>();

class signature_provider_plugin_impl {
   public:
      fc::microseconds  _keosd_provider_timeout_us;

      signature_provider_plugin::signature_provider_type
      make_key_signature_provider(const chain::private_key_type& key) const {
         return [key]( const chain::digest_type& digest ) {
            return key.sign(digest);
         };
      }

#ifdef __APPLE__
      signature_provider_plugin::signature_provider_type
      make_se_signature_provider(const chain::public_key_type pubkey) const {
         EOS_ASSERT(secure_enclave::hardware_supports_secure_enclave(), chain::secure_enclave_exception, "Secure Enclave not supported on this hardware");
         EOS_ASSERT(secure_enclave::application_signed(), chain::secure_enclave_exception, "Application is not signed, Secure Enclave use not supported");

         std::set<secure_enclave::secure_enclave_key> allkeys = secure_enclave::get_all_keys();
         for(const auto& se_key : secure_enclave::get_all_keys())
            if(se_key.public_key() == pubkey)
               return [se_key](const chain::digest_type& digest) {
                  return se_key.sign(digest);
               };

         EOS_THROW(chain::secure_enclave_exception, "${k} not found in Secure Enclave", ("k", pubkey));
      }
#endif

#ifdef ENABLE_TPM
      signature_provider_plugin::signature_provider_type
      make_tpm_signature_provider(const std::string& spec_data, const chain::public_key_type pubkey) const {
         std::vector<string> params;
         std::vector<unsigned> pcrs;

         boost::split(params,spec_data,boost::is_any_of("|"));
         EOS_ASSERT(params.size() <= 2, chain::plugin_config_exception, "Too many extra fields given to TPM provider via '|' parameter");
         if(params.size() == 2) {
            vector<string> pcr_strs;
            boost::split(pcr_strs,params[1],boost::is_any_of(","));
            for(const auto& ps : pcr_strs)
               if(ps.size())
                  pcrs.emplace_back(std::stoi(ps));
         }

         std::shared_ptr<eosio::tpm::tpm_key> tpmkey = std::make_shared<eosio::tpm::tpm_key>(params[0], pubkey, pcrs);
         return [tpmkey](const chain::digest_type& digest) {
            return tpmkey->sign(digest);
         };
      }
#endif

      signature_provider_plugin::signature_provider_type
      make_keosd_signature_provider(const string& url_str, const chain::public_key_type pubkey) const {
         fc::url keosd_url;
         if(boost::algorithm::starts_with(url_str, "unix://"))
            //send the entire string after unix:// to http_plugin. It'll auto-detect which part
            // is the unix socket path, and which part is the url to hit on the server
            keosd_url = fc::url("unix", url_str.substr(7), fc::ostring(), fc::ostring(), fc::ostring(), fc::ostring(), fc::ovariant_object(), std::optional<uint16_t>());
         else
            keosd_url = fc::url(url_str);

         return [to=_keosd_provider_timeout_us, keosd_url, pubkey](const chain::digest_type& digest) {
            fc::variant params;
            fc::to_variant(std::make_pair(digest, pubkey), params);
            auto deadline = to.count() >= 0 ? fc::time_point::now() + to : fc::time_point::maximum();
            return app().get_plugin<http_client_plugin>().get_client().post_sync(keosd_url, params, deadline).as<chain::signature_type>();
         };
      }
};

signature_provider_plugin::signature_provider_plugin():my(new signature_provider_plugin_impl()){}
signature_provider_plugin::~signature_provider_plugin(){}

void signature_provider_plugin::set_program_options(options_description&, options_description& cfg) {
   cfg.add_options()
         ("keosd-provider-timeout", boost::program_options::value<int32_t>()->default_value(5),
          "Limits the maximum time (in milliseconds) that is allowed for sending requests to a keosd provider for signing")
         ;
}

const char* const signature_provider_plugin::signature_provider_help_text() const {
   return "Key=Value pairs in the form <public-key>=<provider-spec>\n"
          "Where:\n"
          "   <public-key>    \tis a string form of a vaild EOSIO public key\n\n"
          "   <provider-spec> \tis a string in the form <provider-type>:<data>\n\n"
          "   <provider-type> \tis one of the types below\n\n"
          "   KEY:<data>      \tis a string form of a valid EOSIO private key which maps to the provided public key\n\n"
          "   KEOSD:<data>    \tis the URL where keosd is available and the approptiate wallet(s) are unlocked"
#ifdef __APPLE__
          "\n\n"
          "   SE:             \tindicates the key resides in Secure Enclave\n\n"
#endif
#ifdef ENABLE_TPM
          "\n\n"
          "   TPM:<data>     \tindicates the key resides in persistent TPM storage, 'data' is in the form <tcti>|<pcr_list> where "
                               "optional 'tcti' is the tcti and tcti options, and optional 'pcr_list' is a comma separated list of PCRs "
                               "to authenticate with\n\n"

#endif
          ;
}

void signature_provider_plugin::plugin_initialize(const variables_map& options) {
   my->_keosd_provider_timeout_us = fc::milliseconds( options.at("keosd-provider-timeout").as<int32_t>() );
}

std::pair<chain::public_key_type,signature_provider_plugin::signature_provider_type>
signature_provider_plugin::signature_provider_for_specification(const std::string& spec) const {
   auto delim = spec.find("=");
   EOS_ASSERT(delim != std::string::npos, chain::plugin_config_exception, "Missing \"=\" in the key spec pair");
   auto pub_key_str = spec.substr(0, delim);
   auto spec_str = spec.substr(delim + 1);

   auto spec_delim = spec_str.find(":");
   EOS_ASSERT(spec_delim != std::string::npos, chain::plugin_config_exception, "Missing \":\" in the key spec pair");
   auto spec_type_str = spec_str.substr(0, spec_delim);
   auto spec_data = spec_str.substr(spec_delim + 1);

   auto pubkey = chain::public_key_type(pub_key_str);

   if(spec_type_str == "KEY") {
      chain::private_key_type priv(spec_data);
      EOS_ASSERT(pubkey == priv.get_public_key(), chain::plugin_config_exception, "Private key does not match given public key for ${pub}", ("pub", pubkey));
      return std::make_pair(pubkey, my->make_key_signature_provider(priv));
   }
   else if(spec_type_str == "KEOSD")
      return std::make_pair(pubkey, my->make_keosd_signature_provider(spec_data, pubkey));
#ifdef ENABLE_TPM
   else if(spec_type_str == "TPM")
      return std::make_pair(pubkey, my->make_tpm_signature_provider(spec_data, pubkey));
#endif
#ifdef __APPLE__
   else if(spec_type_str == "SE")
      return std::make_pair(pubkey, my->make_se_signature_provider(pubkey));
#endif
   EOS_THROW(chain::plugin_config_exception, "Unsupported key provider type \"${t}\"", ("t", spec_type_str));
}

signature_provider_plugin::signature_provider_type
signature_provider_plugin::signature_provider_for_private_key(const chain::private_key_type priv) const {
   return my->make_key_signature_provider(priv);
}

}
