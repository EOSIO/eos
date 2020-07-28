#include <eosio/cppkin_plugin/cppkin_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <cppkin.h>

namespace {

static appbase::abstract_plugin& cppkin_plugin_ = appbase::app().register_plugin<eosio::cppkin_plugin>();

} // anonymous

namespace eosio {

namespace bpo = boost::program_options;

struct cppkin_plugin_impl {
   std::string endpoint;
   std::string service_name;
   std::string api_key;
};

cppkin_plugin::cppkin_plugin()
: my(std::make_unique<cppkin_plugin_impl>()) {}

cppkin_plugin::~cppkin_plugin() {}

void cppkin_plugin::set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
   auto op = cfg.add_options();
   op("cppkin-endpoint", bpo::value<std::string>(),
         "cppkin connection endpoint");
   op("cppkin-service-name", bpo::value<std::string>(),
         "cppkin service name");
   op("cppkin-api-key", bpo::value<std::string>(),
         "cppkin API Key");
}

void cppkin_plugin::plugin_initialize(const appbase::variables_map& options) {
   try {
      if( options.count("cppkin-endpoint") ) {
         my->endpoint = options.at("cppkin-endpoint").as<std::string>();
      }
      if( options.count("cppkin-service-name") ) {
         my->service_name = options.at("cppkin-service-name").as<std::string>();
      }
      if( options.count("cppkin-api-key") ) {
         my->api_key = options.at("cppkin-api-key").as<std::string>();
      }
   }
   FC_LOG_AND_RETHROW()
}

void cppkin_plugin::plugin_startup() {
   handle_sighup();
   try {

      cppkin::CppkinParams cppkinParams;
      cppkinParams.AddParam(cppkin::ConfigTags::ENDPOINT, my->endpoint);
      cppkinParams.AddParam(cppkin::ConfigTags::SERVICE_NAME, my->service_name);
      cppkinParams.AddParam(cppkin::ConfigTags::PORT, -1);
      cppkinParams.AddParam(cppkin::ConfigTags::SAMPLE_COUNT, 1);
      cppkinParams.AddParam(cppkin::ConfigTags::TRANSPORT_TYPE, cppkin::TransportType(cppkin::TransportType::Http).ToString());
      cppkinParams.AddParam(cppkin::ConfigTags::ENCODING_TYPE, cppkin::EncodingType(cppkin::EncodingType::Json).ToString());
      cppkinParams.AddParam(cppkin::ConfigTags::API_KEY, my->api_key);
      cppkinParams.AddParam(cppkin::ConfigTags::DATA_FORMAT, std::string("zipkin"));
      cppkinParams.AddParam(cppkin::ConfigTags::DATA_FORMAT_VERSION, 2);
      cppkin::Init(cppkinParams);

   } FC_LOG_AND_RETHROW()
}

void cppkin_plugin::plugin_shutdown() {
   try {
      cppkin::Stop();
   }
   FC_CAPTURE_AND_RETHROW()
}

void cppkin_plugin::handle_sighup() {
}

} // namespace eosio
