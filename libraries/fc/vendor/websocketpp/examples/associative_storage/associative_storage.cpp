#include <iostream>
#include <map>
#include <exception>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct connection_data {
    int sessionid;
    std::string name;
};

class print_server {
public:
    print_server() : m_next_sessionid(1) {
        m_server.init_asio();

        m_server.set_open_handler(bind(&print_server::on_open,this,::_1));
        m_server.set_close_handler(bind(&print_server::on_close,this,::_1));
        m_server.set_message_handler(bind(&print_server::on_message,this,::_1,::_2));
    }

    void on_open(connection_hdl hdl) {
        connection_data data;

        data.sessionid = m_next_sessionid++;
        data.name.clear();

        m_connections[hdl] = data;
    }

    void on_close(connection_hdl hdl) {
        connection_data& data = get_data_from_hdl(hdl);

        std::cout << "Closing connection " << data.name
                  << " with sessionid " << data.sessionid << std::endl;

        m_connections.erase(hdl);
    }

    void on_message(connection_hdl hdl, server::message_ptr msg) {
        connection_data& data = get_data_from_hdl(hdl);

        if (data.name.empty()) {
            data.name = msg->get_payload();
            std::cout << "Setting name of connection with sessionid "
                      << data.sessionid << " to " << data.name << std::endl;
        } else {
            std::cout << "Got a message from connection " << data.name
                      << " with sessionid " << data.sessionid << std::endl;
        }
    }

    connection_data& get_data_from_hdl(connection_hdl hdl) {
        auto it = m_connections.find(hdl);

        if (it == m_connections.end()) {
            // this connection is not in the list. This really shouldn't happen
            // and probably means something else is wrong.
            throw std::invalid_argument("No data available for session");
        }

        return it->second;
    }

    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }
private:
    typedef std::map<connection_hdl,connection_data,std::owner_less<connection_hdl>> con_list;

    int m_next_sessionid;
    server m_server;
    con_list m_connections;
};

int main() {
    print_server server;
    server.run(9002);
}
