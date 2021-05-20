#include <eosio/wallet_api_plugin/wallet_api_plugin.hpp>
#include <eosio/wallet_plugin/wallet_manager.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain/transaction.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>

#include <chrono>

namespace eosio { namespace detail {
  struct wallet_api_plugin_empty {};
}}

FC_REFLECT(eosio::detail::wallet_api_plugin_empty, );

namespace eosio {

static appbase::abstract_plugin& _wallet_api_plugin = app().register_plugin<wallet_api_plugin>();

using namespace eosio;

#define CALL_WITH_400(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [&api_handle](string, string body, url_response_callback cb) mutable { \
          try { \
             INVOKE \
             cb(http_response_code, fc::variant(result)); \
          } catch (...) { \
             http_plugin::handle_exception(#api_name, #call_name, body, cb); \
          } \
       }}

#define INVOKE_R_R(api_handle, call_name, in_param) \
     auto params = parse_params<in_param, http_params_types::params_required>(body);\
     auto result = api_handle.call_name( std::move(params) );

#define INVOKE_R_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& params = parse_params<fc::variants, http_params_types::params_required>(body);\
     if (params.size() != 2) { \
        EOS_THROW(chain::invalid_http_request, "Missing valid input from POST body"); \
     } \
     auto result = api_handle.call_name(params.at(0).as<in_param0>(), params.at(1).as<in_param1>());

// chain_id_type does not have default constructor, keep it unchanged
#define INVOKE_R_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2) \
     const auto& params = parse_params<fc::variants, http_params_types::params_required>(body);\
     if (params.size() != 3) { \
        EOS_THROW(chain::invalid_http_request, "Missing valid input from POST body"); \
     } \
     auto result = api_handle.call_name(params.at(0).as<in_param0>(), params.at(1).as<in_param1>(), params.at(2).as<in_param2>());

#define INVOKE_R_V(api_handle, call_name) \
     body = parse_params<std::string, http_params_types::no_params_required>(body); \
     auto result = api_handle.call_name();

#define INVOKE_V_R(api_handle, call_name, in_param) \
     auto params = parse_params<in_param, http_params_types::params_required>(body);\
     api_handle.call_name( std::move(params) ); \
     eosio::detail::wallet_api_plugin_empty result;

#define INVOKE_V_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& params = parse_params<fc::variants, http_params_types::params_required>(body);\
     if (params.size() != 2) { \
        EOS_THROW(chain::invalid_http_request, "Missing valid input from POST body"); \
     } \
     api_handle.call_name(params.at(0).as<in_param0>(), params.at(1).as<in_param1>()); \
     eosio::detail::wallet_api_plugin_empty result;

#define INVOKE_V_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2) \
     const auto& params = parse_params<fc::variants, http_params_types::params_required>(body);\
     if (params.size() != 3) { \
        EOS_THROW(chain::invalid_http_request, "Missing valid input from POST body"); \
     } \
     api_handle.call_name(params.at(0).as<in_param0>(), params.at(1).as<in_param1>(), params.at(2).as<in_param2>()); \
     eosio::detail::wallet_api_plugin_empty result;

#define INVOKE_V_V(api_handle, call_name) \
     body = parse_params<std::string, http_params_types::no_params_required>(body); \
     api_handle.call_name(); \
     eosio::detail::wallet_api_plugin_empty result;

void wallet_api_plugin::plugin_startup() {
   ilog("starting wallet_api_plugin");
   // lifetime of plugin is lifetime of application
   auto& wallet_mgr = app().get_plugin<wallet_plugin>().get_wallet_manager();

   app().get_plugin<http_plugin>().add_api({
       CALL_WITH_400(wallet, wallet_mgr, set_timeout,
            INVOKE_V_R(wallet_mgr, set_timeout, int64_t), 200),
       //  chain::chain_id_type has an inaccessible default constructor
       CALL_WITH_400(wallet, wallet_mgr, sign_transaction,
            INVOKE_R_R_R_R(wallet_mgr, sign_transaction, chain::signed_transaction, chain::flat_set<public_key_type>, chain::chain_id_type), 201),
       CALL_WITH_400(wallet, wallet_mgr, sign_digest,
            INVOKE_R_R_R(wallet_mgr, sign_digest, chain::digest_type, public_key_type), 201),
       CALL_WITH_400(wallet, wallet_mgr, create,
            INVOKE_R_R(wallet_mgr, create, std::string), 201),
       CALL_WITH_400(wallet, wallet_mgr, open,
            INVOKE_V_R(wallet_mgr, open, std::string), 200),
       CALL_WITH_400(wallet, wallet_mgr, lock_all,
            INVOKE_V_V(wallet_mgr, lock_all), 200),
       CALL_WITH_400(wallet, wallet_mgr, lock,
            INVOKE_V_R(wallet_mgr, lock, std::string), 200),
       CALL_WITH_400(wallet, wallet_mgr, unlock,
            INVOKE_V_R_R(wallet_mgr, unlock, std::string, std::string), 200),
       CALL_WITH_400(wallet, wallet_mgr, import_key,
            INVOKE_V_R_R(wallet_mgr, import_key, std::string, std::string), 201),
       CALL_WITH_400(wallet, wallet_mgr, remove_key,
            INVOKE_V_R_R_R(wallet_mgr, remove_key, std::string, std::string, std::string), 201),
       CALL_WITH_400(wallet, wallet_mgr, create_key,
            INVOKE_R_R_R(wallet_mgr, create_key, std::string, std::string), 201),
       CALL_WITH_400(wallet, wallet_mgr, list_wallets,
            INVOKE_R_V(wallet_mgr, list_wallets), 200),
       CALL_WITH_400(wallet, wallet_mgr, list_keys,
            INVOKE_R_R_R(wallet_mgr, list_keys, std::string, std::string), 200),
       CALL_WITH_400(wallet, wallet_mgr, get_public_keys,
            INVOKE_R_V(wallet_mgr, get_public_keys), 200)
   });
}

void wallet_api_plugin::plugin_initialize(const variables_map& options) {
   try {
      const auto& _http_plugin = app().get_plugin<http_plugin>();
      if( !_http_plugin.is_on_loopback()) {
         if( !_http_plugin.is_secure()) {
            elog( "\n"
                  "********!!!SECURITY ERROR!!!********\n"
                  "*                                  *\n"
                  "* --       Wallet API           -- *\n"
                  "* - EXPOSED to the LOCAL NETWORK - *\n"
                  "* -  HTTP RPC is NOT encrypted   - *\n"
                  "* - Password and/or Private Keys - *\n"
                  "* - are at HIGH risk of exposure - *\n"
                  "*                                  *\n"
                  "************************************\n" );
         } else {
            wlog( "\n"
                  "**********SECURITY WARNING**********\n"
                  "*                                  *\n"
                  "* --       Wallet API           -- *\n"
                  "* - EXPOSED to the LOCAL NETWORK - *\n"
                  "* - Password and/or Private Keys - *\n"
                  "* -   are at risk of exposure    - *\n"
                  "*                                  *\n"
                  "************************************\n" );
         }
      }
   } FC_LOG_AND_RETHROW()
}


#undef INVOKE_R_R
#undef INVOKE_R_R_R_R
#undef INVOKE_R_V
#undef INVOKE_V_R
#undef INVOKE_V_R_R
#undef INVOKE_V_V
#undef CALL

}
