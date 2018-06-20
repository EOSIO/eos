/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/exceptions.hpp>
#include <eosio/login_plugin/login_plugin.hpp>

#include <fc/io/json.hpp>

namespace eosio {

static appbase::abstract_plugin& _login_plugin = app().register_plugin<login_plugin>();

using namespace eosio;

struct login_request {
   chain::private_key_type server_ephemeral_priv_key{};
   chain::public_key_type server_ephemeral_pub_key{};
   chain::time_point_sec expiration_time;
};

using login_request_container = boost::multi_index_container<
    login_request,
    indexed_by<
        ordered_unique<member<login_request, chain::public_key_type, &login_request::server_ephemeral_pub_key>>, //
        ordered_unique<member<login_request, chain::time_point_sec, &login_request::expiration_time>>            //
        >                                                                                                        //
    >;

class login_plugin_impl {
 public:
   login_plugin_impl(controller& db) : db(db) {}

   controller& db;
   login_request_container requests{};
};

login_plugin::login_plugin() {}
login_plugin::~login_plugin() {}

void login_plugin::set_program_options(options_description&, options_description&) {}
void login_plugin::plugin_initialize(const variables_map&) {}

#define CALL(call_name, http_response_code)                                                                            \
   {                                                                                                                   \
      std::string("/v1/login/" #call_name), [this](string, string body, url_response_callback cb) mutable {            \
         try {                                                                                                         \
            if (body.empty())                                                                                          \
               body = "{}";                                                                                            \
            auto result = call_name(fc::json::from_string(body).as<login_plugin::call_name##_params>());               \
            cb(http_response_code, fc::json::to_string(result));                                                       \
         } catch (...) {                                                                                               \
            http_plugin::handle_exception("login", #call_name, body, cb);                                              \
         }                                                                                                             \
      }                                                                                                                \
   }

void login_plugin::plugin_startup() {
   ilog("starting login_plugin");
   my.reset(new login_plugin_impl(app().get_plugin<chain_plugin>().chain()));
   app().get_plugin<http_plugin>().add_api({
       CALL(start_login_request, 200),
   });
}

void login_plugin::plugin_shutdown() {}

login_plugin::start_login_request_results
login_plugin::start_login_request(const login_plugin::start_login_request_params& params) {
   EOS_ASSERT(params.expiration_time > fc::time_point::now(), fc::timeout_exception,
              "Requested expiration time ${expiration_time} is in the past",
              ("expiration_time", params.expiration_time));
   login_request request;
   request.server_ephemeral_priv_key = chain::private_key_type::generate_r1();
   request.server_ephemeral_pub_key = request.server_ephemeral_priv_key.get_public_key();
   request.expiration_time = params.expiration_time;
   my->requests.insert(request);
   return {request.server_ephemeral_pub_key};
}

} // namespace eosio
