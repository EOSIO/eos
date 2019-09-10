#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>

#include <boost/iostreams/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/thread.hpp>

class my_web3 {
    public:
        my_web3(const std::string& eth_address);
        void subscribe(const std::string& contract_address, const std::string& topic, std::function<void(client*,websocketpp::connection_hdl,message_ptr)> callback);
        uint64_t get_last_block_num();
        uint64_t get_transaction_confirmations(const std::string& txid);
        std::string new_filter(const std::string& contract_address, const std::string& fromBlock, const std::string& toBlock, const std::string& topics);
        std::string get_filter_logs(const std::string& filter_id);
    private:
        std::string _eth_address;
        client m_client;
        websocketpp::connection_hdl m_hdl;
        uint32_t id = 0;
        uint64_t last_block_num;
        uint64_t tx_block_num;
        std::string created_filter;
        std::string filter_logs;
        std::string tx;

        boost::mutex mutex;

        void wss_connect();
        void send_request(const std::string& request);
        void message_handler(client* c, websocketpp::connection_hdl hdl, message_ptr msg);
        void register_callback(std::function<void(client*,websocketpp::connection_hdl,message_ptr)> callback);

        void wait_for_wss_connection();

        template<class T>
        boost::optional<T> get_json_optional(const std::string& payload, const std::string& path);

        std::map<uint32_t, std::function<void(client*,websocketpp::connection_hdl,message_ptr)>> callbacks;
};
