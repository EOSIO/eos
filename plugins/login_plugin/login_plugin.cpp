/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/chain/authorization_manager.hpp>
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

struct login_request_pub_key_index {};
struct login_request_time_index {};

using login_request_container = boost::multi_index_container<
    login_request,
    indexed_by< //
        ordered_unique<tag<login_request_pub_key_index>,
                       member<login_request, chain::public_key_type, &login_request::server_ephemeral_pub_key>>,
        ordered_non_unique<tag<login_request_time_index>,
                           member<login_request, chain::time_point_sec, &login_request::expiration_time>> //
        >>;

class login_plugin_impl {
 public:
   login_plugin_impl(controller& db) : db(db) {}

   controller& db;
   login_request_container requests{};

   void expire_requests() {
      auto& index = requests.get<login_request_time_index>();
      auto now = fc::time_point::now();
      for (auto it = index.begin(); it != index.end() && it->expiration_time < now; it = index.erase(it))
         ;
   }
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
       CALL(start_login_request, 200), //
       CALL(finalize_login_request, 200),
       // CALL(do_not_use_gen_r1_key, 200),  //
       // CALL(do_not_use_sign, 200),        //
       // CALL(do_not_use_get_secret, 200),  //
   });
}

void login_plugin::plugin_shutdown() {}

login_plugin::start_login_request_results
login_plugin::start_login_request(const login_plugin::start_login_request_params& params) {
   // todo: limit how far in the future timeout can be
   // todo: limit number of requests
   my->expire_requests();
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

login_plugin::finalize_login_request_results
login_plugin::finalize_login_request(const login_plugin::finalize_login_request_params& params) {
   finalize_login_request_results result;
   my->expire_requests();
   auto& index = my->requests.get<login_request_pub_key_index>();
   auto it = index.find(params.server_ephemeral_pub_key);
   if (it == index.end()) {
      result.error = "server_ephemeral_pub_key expired or not found";
      return result;
   }
   auto request = *it;
   index.erase(it);

   auto shared_secret = request.server_ephemeral_priv_key.generate_shared_secret(params.client_ephemeral_pub_key);

   chain::bytes combined_data(1024 * 1024);
   chain::datastream<char*> sig_data_ds{combined_data.data(), combined_data.size()};
   fc::raw::pack(sig_data_ds, params.permission);
   fc::raw::pack(sig_data_ds, shared_secret);
   fc::raw::pack(sig_data_ds, params.data);
   combined_data.resize(sig_data_ds.tellp());

   result.digest = chain::sha256::hash(combined_data);
   for (auto& sig : params.signatures)
      result.recovered_keys.insert(chain::public_key_type{sig, result.digest});

   try {
      auto noop_checktime = [] {};
      my->db.get_authorization_manager().check_authorization( //
          params.permission.actor, params.permission.permission, result.recovered_keys, {}, fc::microseconds(0),
          noop_checktime, true);
      result.permission_satisfied = true;
   } catch (...) {
      result.error = "keys do not satisfy permission";
   }

   return result;
}

login_plugin::do_not_use_gen_r1_key_results
login_plugin::do_not_use_gen_r1_key(const login_plugin::do_not_use_gen_r1_key_params& params) {
   auto priv = chain::private_key_type::generate_r1();
   return {priv.get_public_key(), priv};
}

login_plugin::do_not_use_sign_results
login_plugin::do_not_use_sign(const login_plugin::do_not_use_sign_params& params) {
   return {params.priv_key.sign(chain::sha256::hash(params.data))};
}

login_plugin::do_not_use_get_secret_results
login_plugin::do_not_use_get_secret(const login_plugin::do_not_use_get_secret_params& params) {
   return {params.priv_key.generate_shared_secret(params.pub_key)};
}

} // namespace eosio
