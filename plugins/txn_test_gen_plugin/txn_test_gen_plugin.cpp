/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/txn_test_gen_plugin/txn_test_gen_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>

#include <boost/asio/high_resolution_timer.hpp>
#include <boost/algorithm/clamp.hpp>

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
      name creator(init_name);

      chain_controller& cc = app().get_plugin<chain_plugin>().chain();
      chain::chain_id_type chainid;
      app().get_plugin<chain_plugin>().get_chain_id(chainid);
      uint64_t stake = 10000;

      fc::crypto::private_key txn_test_receiver_A_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'a')));
      fc::crypto::private_key txn_test_receiver_B_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'b')));
      fc::crypto::public_key  txn_text_receiver_A_pub_key = txn_test_receiver_A_priv_key.get_public_key();
      fc::crypto::public_key  txn_text_receiver_B_pub_key = txn_test_receiver_B_priv_key.get_public_key();
      fc::crypto::private_key creator_priv_key = fc::crypto::private_key(init_priv_key);

      //stake some currency; and then create some test accounts
      auto memo = fc::variant(fc::time_point::now()).as_string() + " " + fc::variant(fc::time_point::now().time_since_epoch()).as_string();
      signed_transaction trx;
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.set_reference_block(cc.head_block_id());
      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::lock{creator, creator, 300});

      //create "A" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_A_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_A_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};
      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountA, owner_auth, active_auth, recovery_auth, stake});
      }
      //create "B" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_B_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_B_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountB, owner_auth, active_auth, recovery_auth, stake});
      }
      trx.sign(creator_priv_key, chainid);
      cc.push_transaction(trx);

      //now, transfer some balance to new accounts
      {
      uint64_t balance = 10000;
      signed_transaction trx;
      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::transfer{creator, newaccountA, balance, memo});
      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::transfer{creator, newaccountB, balance, memo});
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.set_reference_block(cc.head_block_id());
      trx.sign(creator_priv_key, chainid);
      cc.push_transaction(trx);
      }
   }

   void start_generation(const std::string& salt, const uint64_t& persecond) {
      if(running)
         throw fc::exception(fc::invalid_operation_exception_code);
      running = true;
      memo_salt = salt;

      timer_timeout = 1000/boost::algorithm::clamp(persecond/2, 1, 200);

      ilog("Started transaction test plugin; performing ${p} transactions/second", ("p", 1000/timer_timeout*2));

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

      std::string memo = memo_salt + fc::variant(fc::time_point::now()).as_string() + " " + fc::variant(fc::time_point::now().time_since_epoch()).as_string();
      //make transaction a->b
      {
      signed_transaction trx;
      trx.actions.emplace_back(vector<chain::permission_level>{{sender,"active"}}, contracts::transfer{sender, recipient, 1, memo});
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.set_reference_block(cc.head_block_id());

      fc::crypto::private_key creator_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'a')));
      trx.sign(creator_priv_key, chainid);
      cc.push_transaction(trx);
      }

      //make transaction b->a
      {
      signed_transaction trx;
      trx.actions.emplace_back(vector<chain::permission_level>{{recipient,"active"}}, contracts::transfer{recipient, sender, 1, memo});
      trx.expiration = cc.head_block_time() + fc::seconds(30);
      trx.set_reference_block(cc.head_block_id());

      fc::crypto::private_key b_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'b')));
      trx.sign(b_priv_key, chainid);
      cc.push_transaction(trx);
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

   std::string memo_salt;
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
      CALL(txn_test_gen, my, start_generation, INVOKE_V_R_R(my, start_generation, std::string, uint64_t), 200)
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
