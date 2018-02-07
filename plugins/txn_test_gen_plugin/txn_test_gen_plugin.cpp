/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/txn_test_gen_plugin/txn_test_gen_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <eosio/utilities/key_conversion.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>

#include <boost/asio/high_resolution_timer.hpp>
#include <boost/algorithm/clamp.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <currency/currency.wast.hpp>
#include <currency/currency.abi.hpp>

namespace eosio { namespace detail {
  struct txn_test_gen_empty {};
}}

FC_REFLECT(eosio::detail::txn_test_gen_empty, );

namespace eosio {

static appbase::abstract_plugin& _txn_test_gen_plugin = app().register_plugin<txn_test_gen_plugin>();

using namespace eosio::chain;

#define CALL(api_name, api_handle, call_name, INVOKE, http_response_code) \
{std::string("/v1/" #api_name "/" #call_name), \
   [this](string, string body, url_response_callback cb) mutable { \
          try { \
             if (body.empty()) body = "{}"; \
             INVOKE \
             cb(http_response_code, fc::json::to_string(result)); \
          } catch (fc::eof_exception& e) { \
             error_results results{400, "Bad Request", e.to_string()}; \
             cb(400, fc::json::to_string(results)); \
             elog("Unable to parse arguments: ${args}", ("args", body)); \
          } catch (fc::exception& e) { \
             error_results results{500, "Internal Service Error", e.to_detail_string()}; \
             cb(500, fc::json::to_string(results)); \
             elog("Exception encountered while processing ${call}: ${e}", ("call", #api_name "." #call_name)("e", e)); \
          } \
       }}

#define INVOKE_V_R_R_R(api_handle, call_name, in_param0, in_param1, in_param2) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle->call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>(), vs.at(2).as<in_param2>()); \
     eosio::detail::txn_test_gen_empty result;

#define INVOKE_V_R_R(api_handle, call_name, in_param0, in_param1) \
     const auto& vs = fc::json::json::from_string(body).as<fc::variants>(); \
     api_handle->call_name(vs.at(0).as<in_param0>(), vs.at(1).as<in_param1>()); \
     eosio::detail::txn_test_gen_empty result;

#define INVOKE_V_V(api_handle, call_name) \
     api_handle->call_name(); \
     eosio::detail::txn_test_gen_empty result;

struct txn_test_gen_plugin_impl {
   void create_test_accounts(const std::string& init_name, const std::string& init_priv_key) {
      name newaccountA("txn.test.a");
      name newaccountB("txn.test.b");
      name newaccountC("currency");
      name creator(init_name);

      contracts::abi_def currency_abi_def = fc::json::from_string(currency_abi).as<contracts::abi_def>();

      chain_controller& cc = app().get_plugin<chain_plugin>().chain();
      chain::chain_id_type chainid;
      app().get_plugin<chain_plugin>().get_chain_id(chainid);
      asset stake(10000);

      fc::crypto::private_key txn_test_receiver_A_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'a')));
      fc::crypto::private_key txn_test_receiver_B_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'b')));
      fc::crypto::private_key txn_test_receiver_C_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'c')));
      fc::crypto::public_key  txn_text_receiver_A_pub_key = txn_test_receiver_A_priv_key.get_public_key();
      fc::crypto::public_key  txn_text_receiver_B_pub_key = txn_test_receiver_B_priv_key.get_public_key();
      fc::crypto::public_key  txn_text_receiver_C_pub_key = txn_test_receiver_C_priv_key.get_public_key();
      fc::crypto::private_key creator_priv_key = fc::crypto::private_key(init_priv_key);


      //create some test accounts
      auto memo = fc::variant(fc::time_point::now()).as_string() + " " + fc::variant(fc::time_point::now().time_since_epoch()).as_string();
      signed_transaction trx;
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.set_reference_block(cc.head_block_id());

      //create "A" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_A_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_A_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};
      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountA, owner_auth, active_auth, recovery_auth});
      }
      //create "B" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_B_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_B_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountB, owner_auth, active_auth, recovery_auth});
      }
      //create "currency" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_C_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_C_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountC, owner_auth, active_auth, recovery_auth});
      }
      
      trx.sign(creator_priv_key, chainid);
      cc.push_transaction(packed_transaction(trx));

      //create currency contract
      {
      signed_transaction trx;
      vector<uint8_t> wasm = wast_to_wasm(std::string(currency_wast));

      contracts::setcode handler;
      handler.account = newaccountC;
      handler.code.assign(wasm.begin(), wasm.end());

      trx.actions.emplace_back( vector<chain::permission_level>{{newaccountC,"active"}}, handler);

      {
      contracts::setabi handler;
      handler.account = newaccountC;
      handler.abi = currency_abi_def;
      trx.actions.emplace_back( vector<chain::permission_level>{{newaccountC,"active"}}, handler);
      }

      trx.set_reference_block(cc.head_block_id());
      trx.sign(txn_test_receiver_C_priv_key, chainid);
      cc.push_transaction(packed_transaction(trx));
      }

      //issue & fund the two accounts with currency contract currency
      {
      signed_transaction trx;
      abi_serializer currency_serializer(currency_abi_def);
      
      {
      action act;
      act.account = N(currency);
      act.name = N(issue);
      act.authorization = vector<permission_level>{{newaccountC,config::active_name}};
      act.data = currency_serializer.variant_to_binary("issue", fc::json::from_string("{\"to\":\"currency\",\"quantity\":\"600.0000 CUR\"}"));
      trx.actions.push_back(act);
      }
      {
      action act;
      act.account = N(currency);
      act.name = N(transfer);
      act.authorization = vector<permission_level>{{newaccountC,config::active_name}};
      act.data = currency_serializer.variant_to_binary("transfer", fc::json::from_string("{\"from\":\"currency\",\"to\":\"txn.test.a\",\"quantity\":\"200.0000 CUR\",\"memo\":\"\"}"));
      trx.actions.push_back(act);
      }
      {
      action act;
      act.account = N(currency);
      act.name = N(transfer);
      act.authorization = vector<permission_level>{{newaccountC,config::active_name}};
      act.data = currency_serializer.variant_to_binary("transfer", fc::json::from_string("{\"from\":\"currency\",\"to\":\"txn.test.b\",\"quantity\":\"200.0000 CUR\",\"memo\":\"\"}"));
      trx.actions.push_back(act);
      }

      trx.set_reference_block(cc.head_block_id());
      trx.sign(txn_test_receiver_C_priv_key, chainid);
      cc.push_transaction(packed_transaction(trx));
      }
   }

   void start_generation(const std::string& salt, const uint64_t& period, const uint64_t& batch_size) {
      if(running)
         throw fc::exception(fc::invalid_operation_exception_code);
      if(period < 1 || period > 2500)
         throw fc::exception(fc::invalid_operation_exception_code);
      if(batch_size < 1 || batch_size > 250)
         throw fc::exception(fc::invalid_operation_exception_code);
      if(batch_size & 1)
         throw fc::exception(fc::invalid_operation_exception_code);

      running = true;

      //create the actions here
      act_a_to_b.account = N(currency);
      act_a_to_b.name = N(transfer);
      act_a_to_b.authorization = vector<permission_level>{{name("txn.test.a"),config::active_name}};
      act_a_to_b.data = currency_serializer.variant_to_binary("transfer", fc::json::from_string(fc::format_string("{\"from\":\"txn.test.a\",\"to\":\"txn.test.b\",\"quantity\":\"1.0000 CUR\",\"memo\":\"${l}\"}", fc::mutable_variant_object()("l", salt))));

      act_b_to_a.account = N(currency);
      act_b_to_a.name = N(transfer);
      act_b_to_a.authorization = vector<permission_level>{{name("txn.test.b"),config::active_name}};
      act_b_to_a.data = currency_serializer.variant_to_binary("transfer", fc::json::from_string(fc::format_string("{\"from\":\"txn.test.b\",\"to\":\"txn.test.a\",\"quantity\":\"1.0000 CUR\",\"memo\":\"${l}\"}", fc::mutable_variant_object()("l", salt))));

      timer_timeout = period;
      batch = batch_size/2;

      ilog("Started transaction test plugin; performing ${p} transactions every ${m}ms", ("p", batch_size)("m", period));

      arm_timer(boost::asio::high_resolution_timer::clock_type::now());
   }

   void arm_timer(boost::asio::high_resolution_timer::time_point s) {
      timer.expires_at(s + std::chrono::milliseconds(timer_timeout));
      timer.async_wait([this](const boost::system::error_code& ec) {
         if(ec)
            return;
         try {
            send_transaction();
         }
         catch(fc::exception e) {
            elog("pushing transaction failed: ${e}", ("e", e.to_detail_string()));
            stop_generation();
            return;
         }
         arm_timer(timer.expires_at());
      });
   }

   void send_transaction() {
      chain_controller& cc = app().get_plugin<chain_plugin>().chain();
      chain::chain_id_type chainid;
      app().get_plugin<chain_plugin>().get_chain_id(chainid);

      name sender("txn.test.a");
      name recipient("txn.test.b");
      fc::crypto::private_key a_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'a')));
      fc::crypto::private_key b_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'b')));

      static uint64_t nonce;

      for(unsigned int i = 0; i < batch; ++i) {
      {
      signed_transaction trx;
      trx.actions.push_back(act_a_to_b);
      trx.actions.emplace_back(action({}, config::system_account_name, "nonce", fc::raw::pack(nonce)));
      trx.set_reference_block(cc.head_block_id());
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.sign(a_priv_key, chainid);
      cc.push_transaction(packed_transaction(trx));
      }

      {
      signed_transaction trx;
      trx.actions.push_back(act_b_to_a);
      trx.actions.emplace_back(action({}, config::system_account_name, "nonce", fc::raw::pack(nonce)));
      trx.set_reference_block(cc.head_block_id());
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.sign(b_priv_key, chainid);
      cc.push_transaction(packed_transaction(trx));
      }
      }
   }

   void stop_generation() {
      if(!running)
         throw fc::exception(fc::invalid_operation_exception_code);
      timer.cancel();
      running = false;
      ilog("Stopping transaction generation test");
   }

   boost::asio::high_resolution_timer timer{app().get_io_service()};
   bool running{false};

   unsigned timer_timeout;
   unsigned batch;

   action act_a_to_b;
   action act_b_to_a;

   abi_serializer currency_serializer = fc::json::from_string(currency_abi).as<contracts::abi_def>();
};

txn_test_gen_plugin::txn_test_gen_plugin() {}
txn_test_gen_plugin::~txn_test_gen_plugin() {}

void txn_test_gen_plugin::set_program_options(options_description&, options_description& cfg) {
}

void txn_test_gen_plugin::plugin_initialize(const variables_map& options) {
}

void txn_test_gen_plugin::plugin_startup() {
   app().get_plugin<http_plugin>().add_api({
      CALL(txn_test_gen, my, create_test_accounts, INVOKE_V_R_R(my, create_test_accounts, std::string, std::string), 200),
      CALL(txn_test_gen, my, stop_generation, INVOKE_V_V(my, stop_generation), 200),
      CALL(txn_test_gen, my, start_generation, INVOKE_V_R_R_R(my, start_generation, std::string, uint64_t, uint64_t), 200)
   });
   my.reset(new txn_test_gen_plugin_impl);
}

void txn_test_gen_plugin::plugin_shutdown() {
   try {
      my->stop_generation();
   }
   catch(fc::exception e) {
   }
}

}
