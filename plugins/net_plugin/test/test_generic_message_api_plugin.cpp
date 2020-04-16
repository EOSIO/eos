#include <eosio/net_plugin/test_generic_message_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio { namespace test_generic_message {

static appbase::abstract_plugin& _test_generic_message_api_plugin = app().register_plugin<test_generic_message_api_plugin>();

using namespace eosio;

class test_generic_message_api_plugin_impl {
public:
   test_generic_message_api_plugin_impl(controller& db)
      : db(db) {}

   controller& db;
};


test_generic_message_api_plugin::test_generic_message_api_plugin(){}
test_generic_message_api_plugin::~test_generic_message_api_plugin(){}

void test_generic_message_api_plugin::set_program_options(options_description&, options_description&) {}
void test_generic_message_api_plugin::plugin_initialize(const variables_map&) {}

struct async_result_visitor : public fc::visitor<std::string> {
   template<typename T>
   std::string operator()(const T& v) const {
      return fc::json::to_string(v);
   }
};

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             fc::variant result( api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()) ); \
             cb(http_response_code, std::move(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define TEST_CONTROL_RW_CALL(call_name, http_response_code) CALL(test_generic_message, rw_api, test_generic_message_apis::read_write, call_name, http_response_code)

void test_generic_message_api_plugin::plugin_startup() {
   my.reset(new test_generic_message_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
   auto rw_api = app().get_plugin<test_generic_message_plugin>().get_read_write_api();

   app().get_plugin<http_plugin>().add_api({
      TEST_CONTROL_RW_CALL(send_type1, 202),
      TEST_CONTROL_RW_CALL(send_type2, 202),
      TEST_CONTROL_RW_CALL(send_type3, 202),
      TEST_CONTROL_RW_CALL(registered_types, 202),
      TEST_CONTROL_RW_CALL(received_data, 202)
   });
}

void test_generic_message_api_plugin::plugin_shutdown() {}

} }
