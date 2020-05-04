#pragma once

#include <eosio/wallet_plugin/wallet_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <fc/reflect/reflect.hpp>

#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

class wallet_api_plugin : public plugin<wallet_api_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((wallet_plugin) (http_plugin))

   wallet_api_plugin() = default;
   wallet_api_plugin(const wallet_api_plugin&) = delete;
   wallet_api_plugin(wallet_api_plugin&&) = delete;
   wallet_api_plugin& operator=(const wallet_api_plugin&) = delete;
   wallet_api_plugin& operator=(wallet_api_plugin&&) = delete;
   virtual ~wallet_api_plugin() override = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override {}
   void plugin_initialize(const variables_map& vm);
   void plugin_startup();
   void plugin_shutdown() {}

private:
};

template<typename T1, typename T2>
struct two_params {
   T1 p1;
   T2 p2;
};

typedef  two_params<std::string, std::string> two_string_params;
typedef  two_params<chain::digest_type, chain::public_key_type> two_params_sign_digest;

template<typename T1, typename T2, typename T3>
struct three_params {
   T1 p1;
   T2 p2;
   T3 p3;
};

typedef  three_params<std::string, std::string, std::string> three_string_params;
typedef  three_params<chain::signed_transaction, chain::flat_set<chain::public_key_type>, chain::chain_id_type> three_params_sign_trx;
}

FC_REFLECT( eosio::two_string_params, (p1)(p2) )
FC_REFLECT( eosio::two_params_sign_digest, (p1)(p2) )
FC_REFLECT( eosio::three_string_params, (p1)(p2)(p3) )
FC_REFLECT( eosio::three_params_sign_trx, (p1)(p2)(p3) )

