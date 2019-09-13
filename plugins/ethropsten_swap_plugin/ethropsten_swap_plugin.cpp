/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/ethropsten_swap_plugin/ethropsten_swap_plugin.hpp>
#include <eosio/ethropsten_swap_plugin/my_web3.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>

#include <Inline/BasicTypes.h>
#include <IR/Module.h>
#include <IR/Validate.h>
#include <WAST/WAST.h>
#include <WASM/WASM.h>
#include <Runtime/Runtime.h>

#include <contracts.hpp>

#include <sstream>
#include <boost/asio/high_resolution_timer.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/optional/optional.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>


namespace eosio {
   static appbase::abstract_plugin& _ethropsten_swap_plugin = app().register_plugin<ethropsten_swap_plugin>();

   using namespace eosio::chain;

class ethropsten_swap_plugin_impl {
  public:
    fc::crypto::private_key    _swap_signing_key;
    name                       _swap_signing_account;
    std::string                _swap_signing_permission;
    std::string                _eth_wss_provider;

    void start_monitor() {
      while (true) {
          try {
              client m_client;
              websocketpp::connection_hdl m_hdl;
              websocketpp::lib::mutex m_lock;
              m_client.clear_access_channels(websocketpp::log::alevel::all);
              m_client.set_access_channels(websocketpp::log::alevel::connect);
              m_client.set_access_channels(websocketpp::log::alevel::disconnect);
              m_client.set_access_channels(websocketpp::log::alevel::app);
              m_client.init_asio();
              using websocketpp::lib::placeholders::_1;
              using websocketpp::lib::placeholders::_2;
              using websocketpp::lib::bind;
              m_client.set_message_handler(bind(&ethropsten_swap_plugin_impl::on_swap_request,this,&m_client,_1,_2));
              m_client.set_tls_init_handler([](websocketpp::connection_hdl){
                  return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
              });
              websocketpp::lib::error_code ec;
              client::connection_ptr con = m_client.get_connection(this->_eth_wss_provider, ec);
              if (ec) {
                  m_client.get_alog().write(websocketpp::log::alevel::app,
                          "Get Connection Error: "+ec.message());
                  throw ec.message();
              }

              m_hdl = con->get_handle();
              m_client.connect(con);

              websocketpp::lib::thread asio_thread(&client::run, &m_client);
              sleep(2);

              string infura_request = "{\"id\": 1," \
                                      "\"method\": \"eth_subscribe\"," \
                                      "\"params\": [\"logs\", {\"address\": \""+string(eth_swap_contract_address)+"\"," \
                                                                "\"topics\": [\""+string(eth_swap_request_event)+"\"]}]}";
              m_client.get_alog().write(websocketpp::log::alevel::app, infura_request);
              m_client.send(m_hdl,infura_request,websocketpp::frame::opcode::text,ec);
              if (ec) {
                  m_client.get_alog().write(websocketpp::log::alevel::app,
                      "Send Error: "+ec.message());
                  throw ec.message();
              }
              asio_thread.join();
          } FC_LOG_WAIT_AND_CONTINUE()
        }
    }

    void on_swap_request(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
        std::string payload = msg->get_payload();
        //payload = "{\"jsonrpc\":\"2.0\",\"method\":\"eth_subscription\",\"params\":{\"subscription\":\"0x1feed3403f747b73a04d4cacd4221281\",\"result\":{\"removed\":false,\"logIndex\":\"0x3\",\"transactionIndex\":\"0x1\",\"transactionHash\":\"0xd9f4f600e0556e0d2fa284db40fc01d7e44c3f3c58c9966f5691e2ab6694806d\",\"blockHash\":\"0xa595c247384dab461d1ab2b9739acb58d9acb23b1149deb7bdd13d8b71037e18\",\"blockNumber\":\"0x606bf8\",\"address\":\"0x9fB8A18fF402680b47387AE0F4e38229EC64f098\",\"data\":\"0x93ece941df27a5787a405383a66a7c26d04e80182adf504365710331ac0625a700000000000000000000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000026e8f00000000000000000000000009f21f19180c8692ebaa061fd231cd1b029ff2326000000000000000000000000000000000000000000000000000000005d71540f0000000000000000000000000000000000000000000000000000000000000035454f53376f4e6d6d786f38796838676d594c55474e43774e4146664c6d724d78746d727a6d46504732394370476d354271344647430000000000000000000000\",\"topics\":[\"0x0e918020302bf93eb479360905c1535ba1dbc8aeb6d20eff433206bf4c514e13\"]}}}";

        ilog("Received swap request ${p}", ("p", payload));

        swap_event_data data;
        try {
            if( !get_swap_event_data(payload, &data, "params.result.data", "params.result.transactionHash") ) {
                elog("Invalid swap request payload ${p}", ("p", payload));
                return;
            }
        } FC_LOG_AND_RETURN()
        std::vector<swap_event_data> swap_requests;
        swap_requests.push_back(data);
        wait_for_tx_confirmation_and_push(swap_requests);
    }

    void wait_for_tx_confirmation_and_push(const std::vector<swap_event_data>& swap_requests) {
        try {
          my_web3 my_w3(this->_eth_wss_provider);
          for (std::vector<swap_event_data>::const_iterator it = swap_requests.begin() ; it != swap_requests.end(); ++it) {
              swap_event_data data = *it;
              std::string txid = data.txid;
              for(int i = 0; i < check_tx_confirmations_times; i++)
                  if(my_w3.get_transaction_confirmations("0x"+data.txid) >= min_tx_confirmations) {
                    push_init_swap_transaction(data);
                    break;
                  }
                  else
                    sleep(wait_for_tx_confirmation);
          }
        } FC_LOG_AND_RETHROW()
    }

    void init_prev_swap_requests() {
        try {
          my_web3 my_w3(this->_eth_wss_provider);
          uint64_t last_block_num = my_w3.get_last_block_num();

          std::stringstream stream;
          stream << std::hex << (last_block_num-eth_events_window_length);
          std::string from_block( stream.str() );

          std::string request_swap_filter_id = my_w3.new_filter(eth_swap_contract_address, "0x"+from_block, "latest", "[\""+string(eth_swap_request_event)+"\"]");
          std::string filter_logs = my_w3.get_filter_logs(request_swap_filter_id);
          std::vector<swap_event_data> prev_swap_requests = get_prev_swap_events(filter_logs);

          wait_for_tx_confirmation_and_push(prev_swap_requests);
        } FC_LOG_AND_RETHROW()
    }

    void push_init_swap_transaction(const swap_event_data& data) {
        bool is_tx_sent = false;
        uint32_t push_tx_attempt = 0;
        while(!is_tx_sent) {
            if(push_tx_attempt)
              wlog("Retrying to push init swap transaction: ${id}", ( "id", data.txid ));
            std::vector<signed_transaction> trxs;
            trxs.reserve(2);

            controller& cc = app().get_plugin<chain_plugin>().chain();
            auto chainid = app().get_plugin<chain_plugin>().get_chain_id();

            if( data.chain_id != chain_id ) {
                elog("Invalid chain identifier ${c}", ("c", data.chain_id));
                return;
            }
            signed_transaction trx;

            uint32_t slot = (data.timestamp * 1000 - block_timestamp_epoch) / block_interval_ms;

            trx.actions.emplace_back(vector<chain::permission_level>{{this->_swap_signing_account,this->_swap_signing_permission}},
              init{this->_swap_signing_account,
                data.txid,
                data.swap_pubkey,
                uint64_to_rem_asset(data.amount),
                data.return_address,
                data.return_chain_id,
                epoch_block_timestamp(slot)});

            trx.expiration = cc.head_block_time() + fc::seconds(30);
            trx.set_reference_block(cc.head_block_id());
            trx.max_net_usage_words = 5000;
            trx.sign(this->_swap_signing_key, chainid);
            trxs.emplace_back(std::move(trx));

            try {
               auto trxs_copy = std::make_shared<std::decay_t<decltype(trxs)>>(std::move(trxs));
               app().post(priority::low, [trxs_copy, &is_tx_sent]() {
                 for (size_t i = 0; i < trxs_copy->size(); ++i) {

                     app().get_plugin<chain_plugin>().accept_transaction( packed_transaction(trxs_copy->at(i)),
                     [&is_tx_sent](const fc::static_variant<fc::exception_ptr, transaction_trace_ptr>& result){
                       is_tx_sent = true;
                       if (result.contains<fc::exception_ptr>()) {
                          elog("Failed to push init swap transaction: ${res}", ( "res", result.get<fc::exception_ptr>()->to_string() ));
                       } else {
                          if (result.contains<transaction_trace_ptr>() && result.get<transaction_trace_ptr>()->receipt) {
                              auto trx_id = result.get<transaction_trace_ptr>()->id;
                              ilog("Pushed init swap transaction: ${id}", ( "id", trx_id ));
                          }
                       }
                    });
                 }
               });
            } FC_LOG_AND_DROP()
            sleep(wait_for_accept_tx);
            if(!is_tx_sent) {
              sleep(retry_push_tx_time);
            }
            push_tx_attempt++;
        }
    }
};

ethropsten_swap_plugin::ethropsten_swap_plugin():my(new ethropsten_swap_plugin_impl()){}
ethropsten_swap_plugin::~ethropsten_swap_plugin(){}

void ethropsten_swap_plugin::set_program_options(options_description&, options_description& cfg) {
  cfg.add_options()
        ("eth-wss-provider", bpo::value<std::string>(),
         "Ethereum websocket provider. For example wss://ropsten.infura.io/ws/v3/<infura_id>")
        ("swap-authority", bpo::value<std::string>(),
         "Account name and permission to authorize init swap actions. For example blockproducer1@active")
        ("swap-signing-key", bpo::value<std::string>(),
         "A private key to sign init swap actions")
        ;
}

void ethropsten_swap_plugin::plugin_initialize(const variables_map& options) {
    try {
      std::string swap_auth = options.at( "swap-authority" ).as<std::string>();

      auto space_pos = swap_auth.find('@');
      EOS_ASSERT( (space_pos != std::string::npos), chain::plugin_config_exception,
                  "invalid authority" );

      std::string permission = swap_auth.substr( space_pos + 1 );
      std::string account = swap_auth.substr( 0, space_pos );
      struct name swap_signing_account(account);

      my->_swap_signing_key = fc::crypto::private_key(options.at( "swap-signing-key" ).as<std::string>());
      my->_swap_signing_account = swap_signing_account;
      my->_swap_signing_permission = permission;

      my->_eth_wss_provider = options.at( "eth-wss-provider" ).as<std::string>();
    } FC_LOG_AND_RETHROW()
}

void ethropsten_swap_plugin::plugin_startup() {
  ilog("Ethropsten swap plugin started");
  try {
    my_web3 my_w3(my->_eth_wss_provider);
    ilog("last eth block: " + to_string(my_w3.get_last_block_num()));
  } FC_LOG_AND_RETHROW()

  /*std::thread t([=](){
    sleep(40);
    my->init_prev_swap_requests();
  });
  t.detach();*/

  std::thread t2([=](){
      sleep(30);
      my->start_monitor();
  });
  t2.detach();
}

void ethropsten_swap_plugin::plugin_shutdown() {
}

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

swap_event_data* parse_swap_event_hex(const std::string& hex_data, swap_event_data* data) {

    if( hex_data.length() != request_swap_hex_data_length )
        return nullptr;

    std::string chain_id = hex_data.substr(0, 64);
    std::string amount_hex = hex_data.substr(64*2, 64);
    std::string return_address = hex_data.substr(64*3 + 24, 40);
    std::string timestamp = hex_data.substr(64*4, 64);
    std::string swap_pubkey = hex_to_string(hex_data.substr(64*6, 106));

    std::stringstream ss;

    ss << std::hex << amount_hex;
    unsigned long long amount_dec;
    ss >> amount_dec;

    ss.str(std::string());
    ss.clear();

    ss << std::hex << timestamp;
    time_t timestamp_dec;
    ss >> timestamp_dec;

    data->chain_id = chain_id;
    data->swap_pubkey = swap_pubkey;
    data->amount = amount_dec;
    data->return_address = return_address;
    data->timestamp = timestamp_dec;

    return data;
}

swap_event_data* get_swap_event_data(boost::property_tree::ptree root, swap_event_data* data, const char* data_key, const char* txid_key) {
    boost::optional<string> hex_data_opt = root.get_optional<string>(data_key);
    boost::optional<string> txid = root.get_optional<string>(txid_key);
    if( !hex_data_opt )
        return nullptr;
    string hex_data = hex_data_opt.get();
    parse_swap_event_hex(hex_data.substr(2), data);

    data->return_chain_id = "ethropsten";
    data->txid = txid.get().substr(2);

    return data;
}

swap_event_data* get_swap_event_data(const std::string& event_str, swap_event_data* data, const char* data_key, const char* txid_key) {
    boost::iostreams::stream<boost::iostreams::array_source> stream(event_str.c_str(), event_str.size());
    namespace pt = boost::property_tree;
    pt::ptree root;
    pt::read_json(stream, root);
    return get_swap_event_data(root, data, data_key, txid_key);
}

asset uint64_to_rem_asset(unsigned long long amount) {
    std::string amount_dec_rem(to_string(amount));
    amount_dec_rem.insert (amount_dec_rem.end()-4,'.');
    amount_dec_rem += " ";
    amount_dec_rem += rem_token_id;
    return asset::from_string(amount_dec_rem);
}

std::vector<swap_event_data> get_prev_swap_events(const std::string& logs) {
    vector<swap_event_data> swap_events;

    boost::iostreams::stream<boost::iostreams::array_source> stream(logs.c_str(), logs.size());
    namespace pt = boost::property_tree;
    pt::ptree root;
    pt::read_json(stream, root);

    for (pt::ptree::value_type &log : root.get_child("result")) {
        const std::string& key = log.first; // key
        const boost::property_tree::ptree& subtree = log.second;

        boost::optional<std::string> data_opt = subtree.get_optional<std::string>("data");
        boost::optional<std::string> txid_opt = subtree.get_optional<std::string>("transactionHash");

        if(data_opt && txid_opt) {
            swap_event_data event_data;
            get_swap_event_data(subtree, &event_data, "data", "transactionHash");
            swap_events.push_back(event_data);
        }
        else
            continue;
    }
    return swap_events;
}

}
