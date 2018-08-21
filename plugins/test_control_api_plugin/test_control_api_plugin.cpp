/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/test_control_api_plugin/test_control_api_plugin.hpp>
#include <eosio/chain/exceptions.hpp>

#include <fc/io/json.hpp>

namespace eosio {

static appbase::abstract_plugin& _test_control_api_plugin = app().register_plugin<test_control_api_plugin>();

using namespace eosio;

class test_control_api_plugin_impl {
public:
   test_control_api_plugin_impl(controller& db)
      : db(db) {}

   controller& db;
};


test_control_api_plugin::test_control_api_plugin(){}
test_control_api_plugin::~test_control_api_plugin(){}

void test_control_api_plugin::set_program_options(options_description&, options_description&) {}
void test_control_api_plugin::plugin_initialize(const variables_map&) {}

struct async_result_visitor : public fc::visitor<std::string> {
   template<typename T>
   std::string operator()(const T& v) const {
      return fc::json::to_string(v);
   }
};

#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          wlog("test_control_api_plugin CALL"); \
          try { \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define TEST_CONTROL_RW_CALL(call_name, http_response_code) CALL(test_control, rw_api, test_control_apis::read_write, call_name, http_response_code)

/*
#define CALL(api_name, api_handle, api_namespace, call_name, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this, api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             wlog("test_control_api_plugin CALL"); \
             wlog("test_control_api_plugin CALL body=${body}",("body",body)); \
             if (body.empty()) body = "{}"; \
             auto result = api_handle.call_name(fc::json::from_string(body).as<api_namespace::call_name ## _params>()); \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define TEST_CONTROL_RW_CALL(call_name, http_response_code) CALL(test_control, rw_api, test_control_apis::read_write, call_name, http_response_code)
*/
void test_control_api_plugin::plugin_startup() {
   ilog( "starting test_control_api_plugin" );
//   auto& chain = app().get_plugin<chain_plugin>().chain();
   my.reset(new test_control_api_plugin_impl(app().get_plugin<chain_plugin>().chain()));
   auto rw_api = app().get_plugin<test_control_plugin>().get_read_write_api();

   app().get_plugin<http_plugin>().add_api({
//      TEST_CONTROL_RW_CALL(kill_node_on_producer, 202)
      {std::string("/v1/test_control/kill_node_on_producer"),
         [this, rw_api](string, string body, url_response_callback cb) mutable {
                wlog("test_control_api_plugin CALL");
                try {
                   if (body.empty()) body = "{}";
                   auto result = rw_api.kill_node_on_producer(fc::json::from_string(body).as<test_control_apis::read_write::kill_node_on_producer_params>());
                   cb(202, fc::json::to_string(result));
                } catch (...) {
                   http_plugin::handle_exception("rw_api", "kill_node_on_producer", body, cb);
                }
             }}
   });
}

void test_control_api_plugin::plugin_shutdown() {}

}
