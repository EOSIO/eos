/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
 #pragma once
 #include <appbase/application.hpp>
 #include <eosio/http_plugin/http_plugin.hpp>
 #include <eosio/chain_plugin/chain_plugin.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

constexpr int32_t block_interval_ms = 500;
constexpr int64_t block_timestamp_epoch = 946684800000ll;  // epoch is year 2000
typedef eosio::chain::block_timestamp<block_interval_ms, block_timestamp_epoch> epoch_block_timestamp;
const char* rem_token_id = "REM";

namespace eosio {

using namespace appbase;

/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class swap_plugin : public appbase::plugin<swap_plugin> {
public:
   swap_plugin();
   virtual ~swap_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   void on_swap_request(client* c, websocketpp::connection_hdl hdl, message_ptr msg);
   void start_monitor();

private:
   std::unique_ptr<class swap_plugin_impl> my;
};

}
