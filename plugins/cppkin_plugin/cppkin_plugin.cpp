#include <eosio/cppkin_plugin/cppkin_plugin.hpp>

#include <eosio/chain/exceptions.hpp>
#include <cppkin.h>
#include <fc/log/trace.hpp>

namespace {

appbase::abstract_plugin& cppkin_plugin_ = appbase::app().register_plugin<eosio::cppkin_plugin>();

} // anonymous

namespace eosio {

namespace bpo = boost::program_options;

struct cppkin_plugin_impl {
   std::string endpoint;
   std::string service_name;
   std::string api_key;
   uint16_t sample_count = 1;
};

cppkin_plugin::cppkin_plugin()
: my(std::make_unique<cppkin_plugin_impl>()) {}

void cppkin_plugin::set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
   auto op = cfg.add_options();
   op("cppkin-endpoint", bpo::value<std::string>(),
         "cppkin connection endpoint");
   op("cppkin-service-name", bpo::value<std::string>(),
         "cppkin service name");
   op("cppkin-api-key", bpo::value<std::string>(),
         "cppkin API Key");
   op("cppkin-sample-count", bpo::value<uint16_t>()->default_value(my->sample_count),
         "cppkin sample count");
}

void cppkin_plugin::plugin_initialize(const appbase::variables_map& options) {
   try {
      if( options.count("cppkin-endpoint") ) {
         my->endpoint = options.at("cppkin-endpoint").as<std::string>();
      }
      EOS_ASSERT( !my->endpoint.empty(), chain::plugin_exception, "cppkin-endpoint is required" );
      if( options.count("cppkin-service-name") ) {
         my->service_name = options.at("cppkin-service-name").as<std::string>();
      }
      if( options.count("cppkin-api-key") ) {
         my->api_key = options.at("cppkin-api-key").as<std::string>();
      }
      my->sample_count = options.at("cppkin-sample-count").as<uint16_t>();
   }
   FC_LOG_AND_RETHROW()
}

void cppkin_plugin::plugin_startup() {
   handle_sighup();
   try {

      ilog("cppkin connect: ${e}", ("e", my->endpoint));
      cppkin::CppkinParams cppkinParams;
      cppkinParams.AddParam(cppkin::ConfigTags::ENDPOINT, my->endpoint);
      cppkinParams.AddParam(cppkin::ConfigTags::SERVICE_NAME, my->service_name);
      cppkinParams.AddParam(cppkin::ConfigTags::PORT, -1);
      cppkinParams.AddParam(cppkin::ConfigTags::SAMPLE_COUNT, static_cast<int>(my->sample_count));
      cppkinParams.AddParam(cppkin::ConfigTags::TRANSPORT_TYPE, cppkin::TransportType(cppkin::TransportType::Http).ToString());
      cppkinParams.AddParam(cppkin::ConfigTags::ENCODING_TYPE, cppkin::EncodingType(cppkin::EncodingType::Json).ToString());
      cppkinParams.AddParam(cppkin::ConfigTags::API_KEY, my->api_key);
      cppkinParams.AddParam(cppkin::ConfigTags::DATA_FORMAT, std::string("zipkin"));
      cppkinParams.AddParam(cppkin::ConfigTags::DATA_FORMAT_VERSION, 2);
      cppkin::Init(cppkinParams);

      fc_cppkin_trace_enabled = true;

      cppkin::Trace trace = cppkin::Trace("cppkin initialized");
      trace.Submit();

   } FC_LOG_AND_RETHROW()
}

void cppkin_plugin::plugin_shutdown() {
   try {
      fc_cppkin_trace_enabled = false;
      cppkin::Stop();
   }
   FC_CAPTURE_AND_RETHROW()
}

void cppkin_plugin::handle_sighup() {
}

} // namespace eosio
