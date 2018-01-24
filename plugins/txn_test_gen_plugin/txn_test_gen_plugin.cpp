/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/txn_test_gen_plugin/txn_test_gen_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eos/utilities/key_conversion.hpp>

#include <thread>
#include <algorithm>

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

static std::vector<uint8_t> assemble_wast( const std::string& wast ) {
   IR::Module module;
   std::vector<WAST::Error> parseErrors;
   WAST::parseModule(wast.c_str(),wast.size(),module,parseErrors);
   if(parseErrors.size())
      FC_THROW_EXCEPTION(fc::parse_error_exception, "wast parsing failure");

   // Serialize the WebAssembly module.
   Serialization::ArrayOutputStream stream;
   WASM::serialize(stream,module);
   return stream.getBytes();
}

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

struct txn_test_gen_thread {
   txn_test_gen_thread(unsigned long id, unsigned int batch_size, unsigned int timerms) :
    _batch_size(batch_size), _timer_timeout(timerms) {
      abi_serializer currency_serializer(fc::json::from_string(currency_abi).as<contracts::abi_def>());
      name testa_name("txn.test.a");
      name testb_name("txn.test.b");
      app().get_plugin<chain_plugin>().get_chain_id(_chainid);

      _act_a_to_b.account = _act_b_to_a.account = N(currency);
      _act_a_to_b.name = _act_b_to_a.name = N(transfer);

      _act_a_to_b.authorization = vector<permission_level>{{testa_name,config::active_name}};
      _act_a_to_b.data = currency_serializer.variant_to_binary("transfer", fc::json::from_string("{\"from\":\"txn.test.a\",\"to\":\"txn.test.b\",\"quantity\":\"1.0000 CUR\",\"memo\":\"\"}"));
     
      _act_b_to_a.authorization = vector<permission_level>{{testb_name,config::active_name}};
      _act_b_to_a.data = currency_serializer.variant_to_binary("transfer", fc::json::from_string("{\"from\":\"txn.test.b\",\"to\":\"txn.test.a\",\"quantity\":\"1.0000 CUR\",\"memo\":\"\"}"));

      _nonce = id << 32;
      populate(pool_size);

      _thread = std::thread([this]() {
         boost::asio::io_service::work workwork(_ios);
         _ios.run();
      });

      arm_timer(boost::asio::high_resolution_timer::clock_type::now());
   }

   void populate(unsigned int how_many) {
      _ready += how_many;
      while(how_many--) {
         _txn_pool[_nonce % pool_size] = signed_transaction();
         _txn_pool[_nonce % pool_size].actions.push_back((_nonce & 1) ? _act_b_to_a : _act_a_to_b);
         _txn_pool[_nonce % pool_size].actions.emplace_back(chain::action({}, contracts::nonce{.value = _nonce}));
         _txn_pool[_nonce % pool_size].set_reference_block(_cc.head_block_id());
         _txn_pool[_nonce % pool_size].sign((_nonce & 1) ? b_priv_key : a_priv_key, _chainid);
         ++_nonce;
      }
   }

   void arm_timer(boost::asio::high_resolution_timer::time_point s) {
      _timer.expires_at(s + std::chrono::milliseconds(_timer_timeout));
      _timer.async_wait([this](const boost::system::error_code& ec) {
         if(ec)
            return;
         try {
            send_transactions();
         }
         catch(fc::exception e) {
            elog("pushing transaction failed: ${e}", ("e", e.to_detail_string()));
            //stop_generation();
            return;
         }
         arm_timer(_timer.expires_at());
      });
   }
  
  void send_transactions() {
     app().get_io_service().dispatch([this] {
       /// try {
           if(_batch_size > _ready)
            elog("ick!");
           unsigned int b = 0;
           for(b = 0; b < _batch_size; ++b) {
              _cc.push_transaction(_txn_pool[_send_idx % pool_size]);
              ++_send_idx;
           }
           while(b) {
              unsigned int to_send_back = std::min(8U, b);
              _ios.dispatch([this, to_send_back] {
                populate(to_send_back);
              });
              b -= to_send_back;
           }
       /// }
        //catch(fc::exception e) {
           ///XXX shut it donw
        //}
          // _ios.dispatch([this] {  //XXX: Dangerous
           //    wlog("got here:)");
          // });
     });
  }

private:
   constexpr static unsigned int pool_size = 256;

   unsigned _timer_timeout;
   std::thread _thread;
   boost::asio::io_service _ios;
   boost::asio::high_resolution_timer _timer{_ios};

   signed_transaction _txn_pool[pool_size];

   uint64_t _nonce;
   unsigned int _batch_size;
   unsigned int _ready;
   unsigned int _send_idx = 0;

   const fc::crypto::private_key a_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'a')));
   const fc::crypto::private_key b_priv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'b')));
   action _act_a_to_b;
   action _act_b_to_a;

   chain_controller& _cc = app().get_plugin<chain_plugin>().chain();
   chain::chain_id_type _chainid;
};

struct txn_test_gen_plugin_impl {
   void create_test_accounts(const std::string& init_name, const std::string& init_priv_key) {
      name newaccountA("txn.test.a");
      name newaccountB("txn.test.b");
      name newaccountC("currency");
      name creator(init_name);

      contracts::abi_def currency_abi_def = fc::json::from_string(currency_abi).as<contracts::abi_def>();
      abi_serializer currency_serializer(currency_abi_def);

      chain_controller& cc = app().get_plugin<chain_plugin>().chain();
      chain::chain_id_type chainid;
      app().get_plugin<chain_plugin>().get_chain_id(chainid);
      uint64_t stake = 10000000;

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
      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountA, owner_auth, active_auth, recovery_auth, stake});
      }
      //create "B" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_B_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_B_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountB, owner_auth, active_auth, recovery_auth, stake});
      }
      //create "currency" account
      {
      auto owner_auth   = eosio::chain::authority{1, {{txn_text_receiver_C_pub_key, 1}}, {}};
      auto active_auth  = eosio::chain::authority{1, {{txn_text_receiver_C_pub_key, 1}}, {}};
      auto recovery_auth = eosio::chain::authority{1, {}, {{{creator, "active"}, 1}}};

      trx.actions.emplace_back(vector<chain::permission_level>{{creator,"active"}}, contracts::newaccount{creator, newaccountC, owner_auth, active_auth, recovery_auth, stake});
      }
      
      trx.sign(creator_priv_key, chainid);
      cc.push_transaction(trx);

      //now, transfer some balance to new accounts (for native currency)
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

      //create currency contract
      {
      signed_transaction trx;
      vector<uint8_t> wasm = assemble_wast(std::string(currency_wast));

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
      cc.push_transaction(trx);
      }

      //issue & fund the two accounts with currency contract currency
      {
      signed_transaction trx;
      
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
      cc.push_transaction(trx);
      }
   }

   void start_generation(const uint64_t& period, const uint64_t& batch) {
      if(running)
         throw fc::exception(fc::invalid_operation_exception_code);
      ///XXX check parameters

      current_generators.emplace(std::piecewise_construct,
              std::forward_as_tuple(next_generator_id),
              std::forward_as_tuple(next_generator_id, batch, period));

      next_generator_id++;

      //ilog("Started transaction test plugin; performing ${p} transactions/second", ("p", 1000/timer_timeout*2));

   }

   void stop_generation() {
      if(!running)
         throw fc::exception(fc::invalid_operation_exception_code);
      //timer.cancel();
      running = false;
      ilog("Stopping transaction generation test");
   }

   bool running{false};

   unsigned int next_generator_id = 0;
   std::unordered_map<unsigned int, txn_test_gen_thread> current_generators;
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
      CALL(txn_test_gen, my, start_generation, INVOKE_V_R_R(my, start_generation, uint64_t, uint64_t), 200)
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
