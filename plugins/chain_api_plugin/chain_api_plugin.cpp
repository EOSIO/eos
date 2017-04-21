#include <eos/chain_api_plugin/chain_api_plugin.hpp>
#include <eos/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eos {

using namespace eos;

class chain_api_plugin_impl {
public:
   chain_api_plugin_impl(database& db)
      : db(db) {}

   void get_info(url_response_callback& cb) {
      fc::mutable_variant_object response;

      response["head_block_num"] = db.head_block_num();
      response["head_block_id"] = db.head_block_id();
      response["head_block_time"] = db.head_block_time();
      response["head_block_producer"] = db.head_block_producer();
      response["recent_slots"] = std::bitset<64>(db.get_dynamic_global_properties().recent_slots_filled).to_string();
      response["participation_rate"] =
            __builtin_popcountll(db.get_dynamic_global_properties().recent_slots_filled) / 64.0;

      cb(200, fc::json::to_string(response));
   }

   database& db;
};


chain_api_plugin::chain_api_plugin()
   :my(new chain_api_plugin_impl(app().get_plugin<chain_plugin>().db())) {}
chain_api_plugin::~chain_api_plugin(){}

void chain_api_plugin::set_program_options(options_description&, options_description&) {}
void chain_api_plugin::plugin_initialize(const variables_map&) {}

void chain_api_plugin::plugin_startup() {
   app().get_plugin<http_plugin>().add_api({
      {std::string("/v1/chain/get_info"), [this](string, string, url_response_callback cb) {my->get_info(cb);}}
   });
}

void chain_api_plugin::plugin_shutdown() {}

}
