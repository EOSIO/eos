/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/swap_plugin/swap_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

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

#include <contracts.hpp>
#include <fstream>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

// This header pulls in the WebSocket++ abstracted thread support that will
// select between boost::thread and std::thread based on how the build system
// is configured.
#include <websocketpp/common/thread.hpp>

/**
 * Define a semi-cross platform helper method that waits/sleeps for a bit.
 */
void wait_a_bit() {
#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
}

namespace eosio {
   static appbase::abstract_plugin& _swap_plugin = app().register_plugin<swap_plugin>();

   using namespace eosio::chain;

class swap_plugin_impl {
  public:

    void push_next_transaction(const std::shared_ptr<std::vector<signed_transaction>>& trxs ) {
       chain_plugin& cp = app().get_plugin<chain_plugin>();

       for (size_t i = 0; i < trxs->size(); ++i) {
          cp.accept_transaction( packed_transaction(trxs->at(i)), [=](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result){
             if (result.contains<fc::exception_ptr>()) {
                ;//next(result.get<fc::exception_ptr>());
             } else {
                if (result.contains<transaction_trace_ptr>() && result.get<transaction_trace_ptr>()->receipt) {

                  ; // _total_us += result.get<transaction_trace_ptr>()->receipt->cpu_usage_us;
                   // ++_txcount;
                }
             }
          });
       }
    }

    void push_transactions( std::vector<signed_transaction>&& trxs, const std::function<void(fc::exception_ptr)>& next ) {
       auto trxs_copy = std::make_shared<std::decay_t<decltype(trxs)>>(std::move(trxs));
       app().post(priority::low, [this, trxs_copy, next]() {
          push_next_transaction(trxs_copy);
       });
    }

    fc::crypto::private_key _swap_signing_key;
    name                    _swap_signing_account;
    string                  _swap_signing_permission;
    string                  _eth_wss_provider;
};

swap_plugin::swap_plugin():my(new swap_plugin_impl()){}
swap_plugin::~swap_plugin(){}

void swap_plugin::set_program_options(options_description&, options_description& cfg) {
  cfg.add_options()
        ("eth-wss-provider", bpo::value<string>(),
         "Ethereum websocket provider. For example wss://ropsten.infura.io/ws/v3/<infura_id>")
        ("swap-authority", bpo::value<string>(),
         "Account name and permission to authorize init swap actions. For example blockproducer1@active")
        ("swap-signing-key", bpo::value<string>(),
         "A private key to sign init swap actions")
        ;
}

void swap_plugin::plugin_initialize(const variables_map& options) {
    try {
      string swap_auth = options.at( "swap-authority" ).as<std::string>();

      auto space_pos = swap_auth.find('@');
      EOS_ASSERT( (space_pos != string::npos), chain::plugin_config_exception,
                  "invalid authority" );

      string permission = swap_auth.substr( space_pos + 1 );
      string account = swap_auth.substr( 0, space_pos );
      struct name swap_signing_account(account);

      my->_swap_signing_key = fc::crypto::private_key(options.at( "swap-signing-key" ).as<std::string>());
      my->_swap_signing_account = swap_signing_account;
      my->_swap_signing_permission = permission;

      my->_eth_wss_provider = options.at( "eth-wss-provider" ).as<std::string>();
    } FC_LOG_AND_RETHROW()
}

void swap_plugin::on_swap_request(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
  std::ofstream outfile ("/root/swap_plugin.log");
  outfile << "on_message called with hdl: " << hdl.lock().get()
          << " and message: " << msg->get_payload()
          << std::endl;
  outfile.close();

  std::vector<signed_transaction> trxs;
  trxs.reserve(2);

  controller& cc = app().get_plugin<chain_plugin>().chain();
  auto chainid = app().get_plugin<chain_plugin>().get_chain_id();

  signed_transaction trx;
  auto memo = fc::variant(fc::time_point::now()).as_string() + " " + fc::variant(fc::time_point::now().time_since_epoch()).as_string();

  fc::crypto::private_key owner_prv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'a')));
  fc::crypto::private_key active_prv_key = fc::crypto::private_key::regenerate(fc::sha256(std::string(64, 'b')));

  fc::crypto::public_key owner_pub_key = owner_prv_key.get_public_key();;
  fc::crypto::public_key active_pub_key = active_prv_key.get_public_key();

  auto owner_auth   = chain::authority{1, {{owner_pub_key, 1}}, {}};
  auto active_auth  = chain::authority{1, {{active_pub_key, 1}}, {}};

  struct name _new_account_name(std::string("test22222222"));

  trx.actions.emplace_back(vector<chain::permission_level>{{my->_swap_signing_account,"active"}},
                           newaccount{my->_swap_signing_account, _new_account_name, owner_auth, active_auth});
  trx.actions.emplace_back(vector<chain::permission_level>{{my->_swap_signing_account,"active"}}, delegatebw{my->_swap_signing_account, _new_account_name, core_sym::from_string("500.0000"), true});

  trx.expiration = cc.head_block_time() + fc::seconds(30);
  trx.set_reference_block(cc.head_block_id());
  trx.max_net_usage_words = 5000;
  trx.sign(my->_swap_signing_key, chainid);
  trxs.emplace_back(std::move(trx));

  try {
     auto next = [=](const fc::static_variant<fc::exception_ptr>& result){
        if (result.contains<fc::exception_ptr>()) {
           try {
              result.get<fc::exception_ptr>()->dynamic_rethrow_exception();
           } FC_LOG_AND_DROP()
        } else {
           // auto trx_id = result.get<chain_apis::read_write::push_transaction_results>().transaction_id;
           // dlog("pushed transaction: ${id}", ( "id", trx_id ));
        }
     };
     //my->push_transactions(std::move(trxs), next);
     auto trxs_copy = std::make_shared<std::decay_t<decltype(trxs)>>(std::move(trxs));
     app().post(priority::low, [trxs_copy]() {
       for (size_t i = 0; i < trxs_copy->size(); ++i) {
           app().get_plugin<chain_plugin>().accept_transaction( packed_transaction(trxs_copy->at(i)), [=](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result){
             if (result.contains<fc::exception_ptr>()) {
                ;//next(result.get<fc::exception_ptr>());
             } else {
                if (result.contains<transaction_trace_ptr>() && result.get<transaction_trace_ptr>()->receipt) {

                  app().get_plugin<chain_plugin>().get_read_write_api().push_transaction( fc::variant_object( fc::mutable_variant_object(packed_transaction(trxs_copy->at(i))) ), [=](const fc::static_variant<fc::exception_ptr>& result){
                     if (result.contains<fc::exception_ptr>()) {
                        try {
                           result.get<fc::exception_ptr>()->dynamic_rethrow_exception();
                        } FC_LOG_AND_DROP()
                     } else {
                        // auto trx_id = result.get<chain_apis::read_write::push_transaction_results>().transaction_id;
                        // dlog("pushed transaction: ${id}", ( "id", trx_id ));
                     }
                  });

                  ; // _total_us += result.get<transaction_trace_ptr>()->receipt->cpu_usage_us;
                   // ++_txcount;
                }
             }
          });
       }
     });
     //chain_plugin& cp = app().get_plugin<chain_plugin>();
     //cp.accept_transaction( packed_transaction(trx), next);
     //cp.get_read_write_api().push_transaction( fc::variant_object( fc::mutable_variant_object(packed_transaction(trx)) ), next );
  } catch (const account_name_exists_exception& ) {

  }
}

void swap_plugin::start_monitor() {
  client m_client;
  websocketpp::connection_hdl m_hdl;
  websocketpp::lib::mutex m_lock;

  m_client.clear_access_channels(websocketpp::log::alevel::all);
  m_client.set_access_channels(websocketpp::log::alevel::connect);
  m_client.set_access_channels(websocketpp::log::alevel::disconnect);
  m_client.set_access_channels(websocketpp::log::alevel::app);

  // Initialize the Asio transport policy
  m_client.init_asio();

  // Bind the handlers we are using
  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::placeholders::_2;
  using websocketpp::lib::bind;
  //m_client.set_open_handler(bind(&telemetry_client::on_open,this,_1));
  //m_client.set_close_handler(bind(&telemetry_client::on_close,this,_1));
  //m_client.set_fail_handler(bind(&telemetry_client::on_fail,this,_1));
  m_client.set_message_handler(bind(&swap_plugin::on_swap_request,this,&m_client,_1,_2));
  m_client.set_tls_init_handler([](websocketpp::connection_hdl){
      return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
  });

  websocketpp::lib::error_code ec;
  client::connection_ptr con = m_client.get_connection(my->_eth_wss_provider, ec);
  if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
              "Get Connection Error: "+ec.message());
      return;
  }

  // Grab a handle for this connection so we can talk to it in a thread
  // safe manor after the event loop starts.
  m_hdl = con->get_handle();

  // Queue the connection. No DNS queries or network connections will be
  // made until the io_service event loop is run.
  m_client.connect(con);

  // Create a thread to run the ASIO io_service event loop
  websocketpp::lib::thread asio_thread(&client::run, &m_client);

  std::stringstream val;
  wait_a_bit();
  //val.str("{\"jsonrpc\":\"2.0\",\"method\":\"eth_getTransactionCount\",\"params\": [\"0xc94770007dda54cF92009BFF0dE90c06F603a09f\",\"0x5bad55\"],\"id\":1}");
  val.str("{\"id\": 1, \"method\": \"eth_subscribe\", \"params\": [\"logs\", {\"address\": \"0x9fB8A18fF402680b47387AE0F4e38229EC64f098\", \"topics\": [\"0x0e918020302bf93eb479360905c1535ba1dbc8aeb6d20eff433206bf4c514e13\"]}]}");

  m_client.get_alog().write(websocketpp::log::alevel::app, val.str());
  m_client.send(m_hdl,val.str(),websocketpp::frame::opcode::text,ec);

  // The most likely error that we will get is that the connection is
  // not in the right state. Usually this means we tried to send a
  // message to a connection that was closed or in the process of
  // closing. While many errors here can be easily recovered from,
  // in this simple example, we'll stop the telemetry loop.
  if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
          "Send Error: "+ec.message());
  }
  sleep(1);
  asio_thread.join();
}


void swap_plugin::plugin_startup() {
    std::ofstream outfile ("/root/swap_plugin.log");
    outfile << "swap plugin started" << std::endl;
    outfile.close();
    using websocketpp::lib::bind;
    std::thread t1(bind(&swap_plugin::start_monitor,this));
    t1.detach();
}


void swap_plugin::plugin_shutdown() {
   // OK, that's enough magic
}

}
