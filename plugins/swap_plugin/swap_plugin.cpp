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

#include <websocketpp/common/thread.hpp>

#include <boost/optional/optional.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>
#include <sstream>

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
   using namespace fc::crypto;
   using namespace std;

class swap_plugin_impl {
  public:
    private_key    _swap_signing_key;
    name           _swap_signing_account;
    string         _swap_signing_permission;
    string         _eth_wss_provider;
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
      string swap_auth = options.at( "swap-authority" ).as<string>();

      auto space_pos = swap_auth.find('@');
      EOS_ASSERT( (space_pos != string::npos), chain::plugin_config_exception,
                  "invalid authority" );

      string permission = swap_auth.substr( space_pos + 1 );
      string account = swap_auth.substr( 0, space_pos );
      struct name swap_signing_account(account);

      my->_swap_signing_key = private_key(options.at( "swap-signing-key" ).as<string>());
      my->_swap_signing_account = swap_signing_account;
      my->_swap_signing_permission = permission;

      my->_eth_wss_provider = options.at( "eth-wss-provider" ).as<string>();
    } FC_LOG_AND_RETHROW()
}

void swap_plugin::plugin_startup() {
    ilog("swap plugin started");
    using websocketpp::lib::bind;
    thread t1(bind(&swap_plugin::start_monitor,this));
    t1.detach();
}


void swap_plugin::plugin_shutdown() {
}

struct swap_event_data {
    string   txid;
    string   chain_id;
    string   swap_pubkey;
    uint64_t amount;
    string   return_address;
    string   return_chain_id;
    uint64_t timestamp;

};

std::string hex_to_string(const std::string& input) {
  static const char* const lut = "0123456789abcdef";
  size_t len = input.length();
  if (len & 1) throw;
  std::string output;
  output.reserve(len / 2);
  for (size_t i = 0; i < len; i += 2) {
    char a = input[i];
    const char* p = std::lower_bound(lut, lut + 16, a);
    if (*p != a) throw;
    char b = input[i + 1];
    const char* q = std::lower_bound(lut, lut + 16, b);
    if (*q != b) throw;
    output.push_back(((p - lut) << 4) | (q - lut));
  }
  return output;
}

swap_event_data* parse_event_hex(const string& hex_data, swap_event_data* data) {
    string chain_id = hex_data.substr(0, 64);
    string amount_hex = hex_data.substr(64*2, 64);
    string return_address = hex_data.substr(64*3 + 24, 40);
    string timestamp = hex_data.substr(64*4, 64);
    string swap_pubkey = hex_to_string(hex_data.substr(64*6, 64));

    stringstream ss;
    ss << std::hex << amount_hex;
    unsigned long long amount_dec;
    ss >> amount_dec;
    /*string amount_dec_rem(to_string(amount_dec));
    amount_dec_rem.insert (amount_dec_rem.end()-4,'.');
    amount_dec_rem += " REM";*/

    stringstream ss2;
    ss2 << std::hex << timestamp;
    time_t timestamp_dec;
    ss2 >> timestamp_dec;

    // 124 163 8943
    // 156 692 2308
    //cout << swap_pubkey << endl;
    //cout << chain_id << " " << " " << amount_dec_rem << " " << return_address << " " << timestamp_dec_str << " " << swap_pubkey << endl;
    data->chain_id = chain_id;
    data->swap_pubkey = swap_pubkey;
    data->amount = amount_dec;
    data->return_address = return_address;
    data->timestamp = timestamp_dec;

    return data;
}

swap_event_data* get_swap_event_data(const string& event_str, swap_event_data* data) {
    boost::iostreams::stream<boost::iostreams::array_source> stream(event_str.c_str(), event_str.size());
    namespace pt = boost::property_tree;
    pt::ptree root;
    pt::read_json(stream, root);
    boost::optional<string> hex_data_opt = root.get_optional<string>("params.result.data");
    boost::optional<string> txid = root.get_optional<string>("params.result.transactionHash");
    if( !hex_data_opt )
        return nullptr;
    string hex_data = hex_data_opt.get();
    parse_event_hex(hex_data.substr(2), data);

    data->return_chain_id = return_chain_id;
    data->txid = txid.get().substr(2);

    return data;
}

void swap_plugin::on_swap_request(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
  // std::ofstream outfile ("/root/swap_plugin.log");
  // outfile << "on_message called with hdl: " << hdl.lock().get()
  //         << " and message: " << msg->get_payload()
  //         << std::endl;
  // outfile.close();
  string payload = msg->get_payload();
  ilog("swap request event ${p}", ("p", payload));

  std::vector<signed_transaction> trxs;
  trxs.reserve(2);
  swap_event_data data;
  get_swap_event_data(payload, &data);

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

  // trx.actions.emplace_back(vector<chain::permission_level>{{my->_swap_signing_account,my->_swap_signing_permission}},
  //                          newaccount{my->_swap_signing_account, _new_account_name, owner_auth, active_auth});
  // trx.actions.emplace_back(vector<chain::permission_level>{{my->_swap_signing_account,my->_swap_signing_permission}},
  //                          delegatebw{my->_swap_signing_account, _new_account_name, asset::from_string("500.0000 REM"), true});

  // trx.actions.emplace_back(vector<chain::permission_level>{{my->_swap_signing_account,my->_swap_signing_permission}},
  //   init{my->_swap_signing_account,
  //     "e70c52411567f22d7bb5fc43d419f0d6c06e26055250b3fc988ac6f3bbfa377b",
  //     "EOS56GpEVDkUaLoab6h6RHreho9Mf6Qu",
  //      asset::from_string("500.0000 REM"),
  //     "9fb8a18ff402680b47387ae0f4e38229ec64f098",
  //     "ethropsten",
  //     block_timestamp<500, 946684800000ll>(fc::time_point::now())});

  if( !get_swap_event_data(payload, &data) )
      return;

  if( data.chain_id != chain_id )
      return;

  uint32_t slot = (data.timestamp * 1000 - block_timestamp_epoch) / block_interval_ms;

  string amount_dec_rem(to_string(data.amount));
  amount_dec_rem.insert (amount_dec_rem.end()-4,'.');
  amount_dec_rem += " ";
  amount_dec_rem += rem_token_id;
  asset quantity = asset::from_string(amount_dec_rem);

  trx.actions.emplace_back(vector<chain::permission_level>{{my->_swap_signing_account,my->_swap_signing_permission}},
    init{my->_swap_signing_account,
      data.txid,
      data.swap_pubkey,
      quantity,
      data.return_address,
      data.return_chain_id,
      epoch_block_timestamp(slot)});


  trx.expiration = cc.head_block_time() + fc::seconds(30);
  trx.set_reference_block(cc.head_block_id());
  trx.max_net_usage_words = 5000;
  trx.sign(my->_swap_signing_key, chainid);
  trxs.emplace_back(std::move(trx));

  try {
     auto trxs_copy = std::make_shared<std::decay_t<decltype(trxs)>>(std::move(trxs));
     app().post(priority::low, [trxs_copy]() {
       for (size_t i = 0; i < trxs_copy->size(); ++i) {
           app().get_plugin<chain_plugin>().accept_transaction( packed_transaction(trxs_copy->at(i)), [=](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result){
             if (result.contains<fc::exception_ptr>()) {
               std::ofstream outfile ("/root/swap_plugin2.log");
               outfile << result.get<fc::exception_ptr>()->to_string() << std::endl;
               outfile.close();
                ;//next(result.get<fc::exception_ptr>());
             } else {
                if (result.contains<transaction_trace_ptr>() && result.get<transaction_trace_ptr>()->receipt) {

                  app().get_plugin<chain_plugin>().get_read_write_api().push_transaction( fc::variant_object( fc::mutable_variant_object(packed_transaction(trxs_copy->at(i))) ), [=](const fc::static_variant<fc::exception_ptr>& result){
                     if (result.contains<fc::exception_ptr>()) {
                        try {
                          std::ofstream outfile ("/root/swap_plugin2.log");
                          outfile << result.get<fc::exception_ptr>()->to_string() << std::endl;
                          outfile.close();
                           //result.get<fc::exception_ptr>()->dynamic_rethrow_exception();
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
  } catch (const account_name_exists_exception& ) {

  }
}

void swap_plugin::start_monitor() {
  sleep(30);
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

  wait_a_bit();

  string infura_request = "{\"id\": 1," \
                          "\"method\": \"eth_subscribe\"," \
                          "\"params\": [\"logs\", {\"address\": \""+string(eth_swap_contract_address)+"\"," \
                                                    "\"topics\": [\""+string(eth_swap_request_event)+"\"]}]}";

  m_client.get_alog().write(websocketpp::log::alevel::app, infura_request);
  m_client.send(m_hdl,infura_request,websocketpp::frame::opcode::text,ec);

  // The most likely error that we will get is that the connection is
  // not in the right state. Usually this means we tried to send a
  // message to a connection that was closed or in the process of
  // closing. While many errors here can be easily recovered from,
  // in this simple example, we'll stop the telemetry loop.
  if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
          "Send Error: "+ec.message());
  }
  asio_thread.join();
}

}
