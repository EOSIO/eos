/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
 #pragma once
 #include <appbase/application.hpp>
 #include <eosio/http_plugin/http_plugin.hpp>
 #include <eosio/chain_plugin/chain_plugin.hpp>

 #include <boost/property_tree/ptree.hpp>
 #include <boost/property_tree/json_parser.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

#define FC_LOG_WAIT_AND_CONTINUE(...)  \
    catch( const boost::interprocess::bad_alloc& ) {\
       throw;\
    } catch( fc::exception& er ) { \
       wlog( "${details}", ("details",er.to_detail_string()) ); \
       sleep(wait_for_eth_node); \
       continue; \
    } catch( const std::exception& e ) {  \
       fc::exception fce( \
                 FC_LOG_MESSAGE( warn, "rethrow ${what}: ",FC_FORMAT_ARG_PARAMS( __VA_ARGS__  )("what",e.what()) ), \
                 fc::std_exception_code,\
                 BOOST_CORE_TYPEID(e).name(), \
                 e.what() ) ; \
       wlog( "${details}", ("details",fce.to_detail_string()) ); \
       sleep(wait_for_eth_node); \
       continue; \
    } catch( ... ) {  \
       fc::unhandled_exception e( \
                 FC_LOG_MESSAGE( warn, "rethrow", FC_FORMAT_ARG_PARAMS( __VA_ARGS__) ), \
                 std::current_exception() ); \
       wlog( "${details}", ("details",e.to_detail_string()) ); \
       sleep(wait_for_eth_node); \
       continue; \
    }

#define FC_LOG_AND_RETURN(...) \
    catch( const boost::interprocess::bad_alloc& ) {\
       throw;\
    } catch( fc::exception& er ) { \
       wlog( "${details}", ("details",er.to_detail_string()) ); \
       return; \
    } catch( const std::exception& e ) {  \
       fc::exception fce( \
                 FC_LOG_MESSAGE( warn, "rethrow ${what}: ",FC_FORMAT_ARG_PARAMS( __VA_ARGS__  )("what",e.what()) ), \
                 fc::std_exception_code,\
                 BOOST_CORE_TYPEID(e).name(), \
                 e.what() ) ; \
       wlog( "${details}", ("details",fce.to_detail_string()) ); \
       return; \
    } catch( ... ) {  \
       fc::unhandled_exception e( \
                 FC_LOG_MESSAGE( warn, "rethrow", FC_FORMAT_ARG_PARAMS( __VA_ARGS__) ), \
                 std::current_exception() ); \
       wlog( "${details}", ("details",e.to_detail_string()) ); \
       return; \
    }

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;
namespace eosio {

using namespace appbase;

constexpr int32_t block_interval_ms = 500;  // eosio block interval constant 0.5s
constexpr int64_t block_timestamp_epoch = 946684800000ll;  // epoch is year 2000
typedef eosio::chain::block_timestamp<block_interval_ms, block_timestamp_epoch> epoch_block_timestamp;

std::string rem_token_id              = "REM";
std::string eth_swap_contract_address = "0x9fB8A18fF402680b47387AE0F4e38229EC64f098";
std::string eth_swap_request_event    = "0x0e918020302bf93eb479360905c1535ba1dbc8aeb6d20eff433206bf4c514e13";  // request swap event id in ethereum swap contract
std::string return_chain_id           = "ethropsten";  // return chain identifier if case of swap cancellation
std::string chain_id                  = "93ece941df27a5787a405383a66a7c26d04e80182adf504365710331ac0625a7";

uint32_t request_swap_hex_data_length = 512;  // payload length of request swap event in hex symbols

uint32_t wait_for_eth_node = 20;  // amount of seconds to sleep before reconnection if connection to ethereum node closes

/*
  amount of last blocks in ethereum to get request swap events
  1_000_000 blocks in ethereum is roughly 0.5 years
*/
uint32_t eth_events_window_length = 1000000;
/*
  amount of blocks per one filter
  infura doesn't support long filters so need to split to several filters
*/
uint32_t blocks_per_filter = 200000;

uint32_t   wait_for_tx_confirmation = 30;  // check if request swap transaction on ethereum is confirmed every wait_for_tx_confirmation seconds
/*
  maximum amount of checks for transaction confirmations of ethereum
  if transaction doesn't have min_tx_confirmations
*/
uint32_t   check_tx_confirmations_times = 5;
uint32_t min_tx_confirmations = 3;  // minimum required request swap transaction confirmations on ethereum to init swap on remprotocol

uint32_t init_swap_expiration_time = 300;  // init swap transaction expiration time
/*
  if transaction didn't make it to blockchain then retry to push it after <retry_push_tx_time> seconds
*/
uint32_t retry_push_tx_time = init_swap_expiration_time + 60;
uint32_t wait_for_accept_tx = 1;  // amount of seconds to wait before checking if transaction is included to blockchain

uint32_t start_monitor_delay = 15;  // amount of seconds to wait before running swap plugin functionality

uint32_t wait_for_wss_connection_time = 2;  // amount of seconds to wait for establishing websocket connection with ethereum node

struct swap_event_data {
    std::string   txid;
    std::string   chain_id;
    std::string   swap_pubkey;
    uint64_t      amount;
    std::string   return_address;
    std::string   return_chain_id;
    uint64_t timestamp;
    uint64_t block_number;
};

std::string hex_to_string(const std::string& input);
swap_event_data* parse_swap_event_hex(const std::string& hex_data, swap_event_data* data);
swap_event_data* get_swap_event_data(boost::property_tree::ptree root, swap_event_data* data, const char* data_key, const char* txid_key, const char* block_number_key);
swap_event_data* get_swap_event_data(const std::string& event_str, swap_event_data* data, const char* data_key, const char* txid_key, const char* block_number_key);
eosio::asset uint64_to_rem_asset(unsigned long long amount);
std::vector<swap_event_data> get_prev_swap_events(const std::string& logs);

class InvalidWssLinkException : public std::exception
{
  public:
    explicit InvalidWssLinkException(const std::string& message) : message_(message) {}
  	const char * what () const throw ()
      {
      	return message_.c_str();
      }
  private:
    std::string message_;
};

class ConnectionClosedException : public std::exception
{
  public:
    explicit ConnectionClosedException(const std::string& message) : message_(message) {}
  	const char * what () const throw ()
      {
      	return message_.c_str();
      }
  private:
    std::string message_;
};


/**
 *  This is a template plugin, intended to serve as a starting point for making new plugins
 */
class eth_swap_plugin : public appbase::plugin<eth_swap_plugin> {
public:
   eth_swap_plugin();
   virtual ~eth_swap_plugin();

   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class eth_swap_plugin_impl> my;
};

}
