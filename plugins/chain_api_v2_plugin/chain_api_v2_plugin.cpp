#include <boost/signals2/connection.hpp>
#include <eosio/chain/controller.hpp>
#include <eosio/chain/exceptions.hpp>
#include <eosio/chain_api_v2_plugin/chain_api_v2_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/http_plugin/http_plugin.hpp>
#include <fc/io/json.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/log/log_message.hpp>

#include <deque>
#include <functional>
#include <queue>
#include <unordered_map>

namespace eosio {
using namespace chain;
using boost::signals2::scoped_connection;
using eosio::chain::controller;
static appbase::abstract_plugin& _chain_api_v2_plugin = app().register_plugin<chain_api_v2_plugin>();

fc::variant make_error_results(uint16_t status_code, const char* role, const char* message) {
   auto log = FC_LOG_MESSAGE(error, message);
   return fc::variant(error_results{status_code, role, error_results::error_info(fc::exception({log}), false)});
}

struct pending_response {
   pending_response(const url_response_callback& cb, chain_apis::read_write::push_transaction_results& r)
       : trx_result_cb(cb)
       , push_trx_result(r) {}
   uint32_t                                         block_num = 0;
   url_response_callback                            trx_result_cb;
   url_response_callback                            wait_result_cb;
   chain_apis::read_write::push_transaction_results push_trx_result;

   void send_push_response() { trx_result_cb(202, fc::variant(push_trx_result)); }
   void send_wait_response(uint32_t num) {
      if (wait_result_cb)
         wait_result_cb(201, fc::variant_object("block_num", num));
      else
         block_num = num;
   }
   void send_expired() {
      if (wait_result_cb) {
         wait_result_cb( 504, make_error_results(504, "Gateway Timeout", "transaction expired"));
      }
   }
};

struct expiration_queue_element {
   fc::time_point      expiration;
   transaction_id_type transaction_id;
};

struct expiration_queue_element_cmp {
   bool operator()(const expiration_queue_element& lhs, const expiration_queue_element& rhs) const {
      return lhs.expiration > rhs.expiration;
   }
};

using pending_responses_t = std::unordered_map<transaction_id_type, pending_response>;
using expiration_queue_t =
    std::priority_queue<expiration_queue_element, std::deque<expiration_queue_element>, expiration_queue_element_cmp>;

class chain_api_v2_plugin_impl {
 public:
   chain_api_v2_plugin_impl(chain_plugin& chain_plug)
       : db(chain_plug.chain())
       , rw_api(chain_plug.get_read_write_api()) {
      accepted_block_connection.emplace(
          db.accepted_block.connect([this](const block_state_ptr& p) { on_accepted_block(p); }));

      irreversible_block_connection.emplace(
          db.irreversible_block.connect([this](const block_state_ptr& p) { on_irreversible_block(p); }));
   }

   using api_handler = decltype(&chain_apis::read_write::push_transaction);

   void handle_transaction_request(string, string body, url_response_callback cb, const char* action,
                                   api_handler handler);
   void handle_wait_transaction_request(string, string body, url_response_callback cb);
   void on_accepted_block(const block_state_ptr& block_state);
   void on_irreversible_block(const block_state_ptr& block_state);
   void clear_expired(fc::time_point timestamp);

   controller&                     db;
   chain_apis::read_write          rw_api;
   fc::optional<scoped_connection> accepted_block_connection;
   fc::optional<scoped_connection> irreversible_block_connection;
   pending_responses_t             pending_responses;
   expiration_queue_t              expiration_queue;
};

chain_api_v2_plugin::chain_api_v2_plugin() {}
chain_api_v2_plugin::~chain_api_v2_plugin() {}

void chain_api_v2_plugin::set_program_options(options_description&, options_description& cfg) {}
void chain_api_v2_plugin::plugin_initialize(const variables_map& options) {}

void chain_api_v2_plugin::plugin_startup() {

   auto& chain_plug = app().register_plugin<chain_plugin>();
   auto& http_plug  = app().register_plugin<http_plugin>();

   my.reset(new chain_api_v2_plugin_impl(chain_plug));

   http_plug.add_handler(std::string("/v2/chain/push_transaction"),
                         [impl = my.get()](string r, string body, url_response_callback cb) mutable {
                            impl->handle_transaction_request(r, body, cb, "push_transaction",
                                                             &chain_apis::read_write::push_transaction);
                         });

   http_plug.add_handler(std::string("/v2/chain/send_transaction"),
                         [impl = my.get()](string r, string body, url_response_callback cb) mutable {
                            impl->handle_transaction_request(r, body, cb, "send_transaction",
                                                             &chain_apis::read_write::send_transaction);
                         });

   http_plug.add_handler(std::string("/v2/chain/wait_transaction"),
                         [impl = my.get()](string r, string body, url_response_callback cb) mutable {
                            impl->handle_wait_transaction_request(r, body, cb);
                         });
}

void chain_api_v2_plugin::plugin_shutdown() {}

void chain_api_v2_plugin_impl::handle_transaction_request(string, string body, url_response_callback cb,
                                                          const char*                           action,
                                                          chain_api_v2_plugin_impl::api_handler method) {
   try {
      rw_api.validate();
      auto params = fc::json::from_string(body).as<chain_apis::read_write::push_transaction_params>();
      auto next =
          [this, cb, body, action](
              const fc::static_variant<fc::exception_ptr, chain_apis::read_write::push_transaction_results>& result) {
             if (result.contains<fc::exception_ptr>()) {
                try {
                   result.get<fc::exception_ptr>()->dynamic_rethrow_exception();
                } catch (...) {
                   http_plugin::handle_exception("v2/chain", action, body, cb);
                }
             } else {
                auto push_trx_result = result.get<chain_apis::read_write::push_transaction_results>();

                pending_responses.emplace(push_trx_result.transaction_id, pending_response{cb, push_trx_result});
                expiration_queue.push({push_trx_result.expiration, push_trx_result.transaction_id});
             }
          };

      std::invoke(method, rw_api, params, next);
   } catch (...) {
      http_plugin::handle_exception("v2/chain", action, body, cb);
   }
}

void chain_api_v2_plugin_impl::handle_wait_transaction_request(string, string body, url_response_callback cb) {

   try {
      rw_api.validate();
      transaction_id_type id =
          fc::json::from_string(body).as<fc::variant_object>()["transaction_id"].as<transaction_id_type>();

      auto itr = pending_responses.find(id);
      if (itr == pending_responses.end()) {
         cb(404, make_error_results(404, "Not Found", "the specified transaction is not currently tracked"));
      } else if (itr->second.block_num > 0) {
         cb(201, fc::variant_object("block_num", itr->second.block_num));
      } else if (!itr->second.wait_result_cb){
         itr->second.wait_result_cb = cb;
      } else {
         cb(403, make_error_results(403, "Forbidden", "pending wait on the transaction existed"));
      }

   } catch (...) {
      http_plugin::handle_exception("v2/chain", "wait_transaction", body, cb);
   }
}

void chain_api_v2_plugin_impl::on_accepted_block(const block_state_ptr& block_state) {
   for (auto& receipt : block_state->block->transactions) {
      auto id = receipt.trx.contains<packed_transaction>()
                    ? receipt.trx.get<packed_transaction>().get_transaction().id()
                    : receipt.trx.get<transaction_id_type>();

      auto itr = pending_responses.find(id);
      if (itr != pending_responses.end()) {
         itr->second.send_push_response();
      }
   }
}

void chain_api_v2_plugin_impl::on_irreversible_block(const block_state_ptr& block_state) {
   for (auto& receipt : block_state->block->transactions) {
      auto id = receipt.trx.contains<packed_transaction>()
                    ? receipt.trx.get<packed_transaction>().get_transaction().id()
                    : receipt.trx.get<transaction_id_type>();

      auto itr = pending_responses.find(id);
      if (itr != pending_responses.end()) {
         itr->second.send_wait_response(block_state->block->block_num());
         pending_responses.erase(itr);
      }
   }

   clear_expired(block_state->block->timestamp.to_time_point());
}

void chain_api_v2_plugin_impl::clear_expired(fc::time_point timestamp) {

   while (expiration_queue.size() && expiration_queue.top().expiration < timestamp) {
      auto id  = expiration_queue.top().transaction_id;
      auto itr = pending_responses.find(id);
      if (itr != pending_responses.end()) {
         itr->second.send_expired();
         pending_responses.erase(itr);
      }
      expiration_queue.pop();
   }
}
} // namespace eosio
