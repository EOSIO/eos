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

#define FC_LOG_WAIT_AND_CONTINUE(t)  \
   FC_LOG_AND_DROP() \
   sleep(t); \
   continue;

#define FC_LOG_AND_RETURN(...) \
    FC_LOG_AND_DROP() \
    return;

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

constexpr int32_t block_interval_ms = 500;
constexpr int64_t block_timestamp_epoch = 946684800000ll;  // epoch is year 2000
typedef eosio::chain::block_timestamp<block_interval_ms, block_timestamp_epoch> epoch_block_timestamp;

const char* rem_token_id              = "REM";
const char* eth_swap_contract_address = "0x9fB8A18fF402680b47387AE0F4e38229EC64f098";
const char* eth_swap_request_event    = "0x0e918020302bf93eb479360905c1535ba1dbc8aeb6d20eff433206bf4c514e13";
const char* return_chain_id           = "ethropsten";
const char* chain_id                  = "93ece941df27a5787a405383a66a7c26d04e80182adf504365710331ac0625a7";

const size_t request_swap_hex_data_length = 512;

const uint32_t wait_for_eth_node = 20;

struct swap_event_data {
    std::string   txid;
    std::string   chain_id;
    std::string   swap_pubkey;
    uint64_t      amount;
    std::string   return_address;
    std::string   return_chain_id;
    uint64_t timestamp;

};

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
