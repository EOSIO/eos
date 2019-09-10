#include <eosio/swap_plugin/my_web3.hpp>


my_web3::my_web3(const std::string& eth_address) {
    _eth_address = eth_address;
    std::thread t1(bind(&my_web3::wss_connect,this));
    wait_for_wss_connection();
    t1.detach();
}

void my_web3::wait_for_wss_connection() {
    sleep(2);
}

void my_web3::wss_connect() {
  websocketpp::lib::mutex m_lock;

  m_client.clear_access_channels(websocketpp::log::alevel::all);
  m_client.set_access_channels(websocketpp::log::alevel::connect);
  m_client.set_access_channels(websocketpp::log::alevel::disconnect);
  m_client.set_access_channels(websocketpp::log::alevel::app);

  m_client.init_asio();

  using websocketpp::lib::placeholders::_1;
  using websocketpp::lib::placeholders::_2;
  using websocketpp::lib::bind;
  m_client.set_message_handler(bind(&my_web3::message_handler,this,&m_client,_1,_2));
  m_client.set_tls_init_handler([](websocketpp::connection_hdl){
      return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);
  });

  websocketpp::lib::error_code ec;
  client::connection_ptr con = m_client.get_connection(_eth_address, ec);
  if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
              "Get Connection Error: "+ec.message());
      throw ec.message();
  }

  m_hdl = con->get_handle();
  m_client.connect(con);

  websocketpp::lib::thread asio_thread(&client::run, &m_client);
  wait_for_wss_connection();

  asio_thread.join();
}

void my_web3::message_handler(client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
    boost::optional<uint32_t> id_opt = get_json_optional<uint32_t>(msg->get_payload(), "id");

    if(id_opt)
        callbacks.at(id_opt.get())(c, hdl, msg);
}
void my_web3::subscribe(const std::string& contract_address, const std::string& topic, std::function<void(client*,websocketpp::connection_hdl,message_ptr)> callback) {
    mutex.lock();
    std::string request = "{\"id\": "+std::to_string(id)+"," \
      "\"method\": \"eth_subscribe\"," \
      "\"params\": [\"logs\", {\"address\": \""+contract_address+"\"," \
                              "\"topics\": [\""+topic+"\"]}]}";
    register_callback(callback);
    //my_web3::on_last_block_num(client* c, websocketpp::connection_hdl hdl, message_ptr msg)
    //register_callback(boost::bind(&my_web3::on_last_block_num, this, _1, _2));
    send_request(request);
    mutex.unlock();
}

void my_web3::send_request(const std::string& request) {
    websocketpp::lib::error_code ec;
    m_client.get_alog().write(websocketpp::log::alevel::app, request);
    m_client.send(m_hdl,request,websocketpp::frame::opcode::text,ec);

    if (ec) {
      m_client.get_alog().write(websocketpp::log::alevel::app,
          "Send Error: "+ec.message());
    throw ec.message();
    }
}

void my_web3::register_callback(std::function<void(client*,websocketpp::connection_hdl,message_ptr)> callback) {
    callbacks[id++] = callback;
}
template<typename T>
boost::optional<T> my_web3::get_json_optional(const string& payload, const string& path) {
    boost::iostreams::stream<boost::iostreams::array_source> stream(payload.c_str(), payload.size());
    namespace pt = boost::property_tree;
    pt::ptree root;
    pt::read_json(stream, root);
    return root.get_optional<T>(path.c_str());
}

uint64_t my_web3::get_last_block_num() {
    mutex.lock();
    boost::mutex response_mutex;
    response_mutex.lock();
    std::string request = "{\"id\": "+std::to_string(id)+"," \
      "\"method\": \"eth_blockNumber\"," \
      "\"params\": []}";
    auto on_last_block_num = [this, &response_mutex](client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
        boost::optional<std::string> block_num_hex = get_json_optional<std::string>(msg->get_payload(), "result");

        if(block_num_hex) {
            std::stringstream ss;
            ss << std::hex << block_num_hex;
            ss >> this->last_block_num;
        }
        else {
            this->last_block_num = 0;
        }
        response_mutex.unlock();
    };
    register_callback(on_last_block_num);
    send_request(request);

    response_mutex.lock();
    response_mutex.unlock();

    mutex.unlock();
    return last_block_num;
}

uint64_t my_web3::get_transaction_confirmations(const string& txid) {
    last_block_num = get_last_block_num();
    boost::mutex response_mutex;
    mutex.lock();
    response_mutex.lock();
    std::string request = "{\"id\": "+std::to_string(id)+"," \
      "\"method\": \"eth_getTransactionByHash\"," \
      "\"params\": [\""+txid+"\"]}";

    auto on_get_transaction = [this, &response_mutex](client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
        boost::optional<std::string> tx_block_opt = get_json_optional<std::string>(msg->get_payload(), "result.blockNumber");
        //cout << "test: " << msg->get_payload() << endl;

        if(tx_block_opt) {
            std::stringstream ss;
            ss << std::hex << tx_block_opt.get();
            ss >> this->tx_block_num;
        }
        else {
            this->tx_block_num = -1;
        }

        response_mutex.unlock();
    };
    register_callback(on_get_transaction);
    send_request(request);

    response_mutex.lock();
    response_mutex.unlock();

    mutex.unlock();
    if(this->tx_block_num != -1)
        return last_block_num - this->tx_block_num;
    else
        return 0;
}

std::string my_web3::new_filter(const std::string& contract_address, const std::string& fromBlock, const std::string& toBlock, const std::string& topics) {
    boost::mutex response_mutex;
    mutex.lock();
    response_mutex.lock();
    std::string request = "{\"id\": "+std::to_string(id)+"," \
      "\"method\": \"eth_newFilter\"," \
      "\"params\": [{\"address\": \""+contract_address+"\"," \
                    "\"fromBlock\": \""+fromBlock+"\"," \
                    "\"toBlock\": \""+toBlock+"\"," \
                    "\"topics\": "+topics+"}]}";

    auto on_new_filter = [this, &response_mutex](client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
        boost::optional<std::string> new_filter_opt = get_json_optional<std::string>(msg->get_payload(), "result");
        //cout << "test: " << msg->get_payload() << endl;

        if(new_filter_opt) {
            this->created_filter = new_filter_opt.get();
        }
        else {
            this->created_filter = "";
        }

        response_mutex.unlock();
    };
    register_callback(on_new_filter);
    send_request(request);

    response_mutex.lock();
    response_mutex.unlock();

    mutex.unlock();
    if(!this->created_filter.empty())
        return this->created_filter;
    else
        return nullptr;
}

std::string my_web3::get_filter_logs(const std::string& filter_id) {
    boost::mutex response_mutex;
    mutex.lock();
    response_mutex.lock();
    std::string request = "{\"id\": "+std::to_string(id)+"," \
      "\"method\": \"eth_getFilterLogs\"," \
      "\"params\": [\""+filter_id+"\"]}";

    auto on_filter_logs = [this, &response_mutex](client* c, websocketpp::connection_hdl hdl, message_ptr msg) {
        this->filter_logs = msg->get_payload();

        response_mutex.unlock();
    };
    register_callback(on_filter_logs);
    send_request(request);

    response_mutex.lock();
    response_mutex.unlock();

    mutex.unlock();
    return this->filter_logs;
}
