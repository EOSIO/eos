#include <iostream>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

struct connection_data {
    int sessionid;
    std::string name;
};

struct custom_config : public websocketpp::config::asio {
    // pull default settings from our core config
    typedef websocketpp::config::asio core;
    
    typedef core::concurrency_type concurrency_type;
    typedef core::request_type request_type;
    typedef core::response_type response_type;
    typedef core::message_type message_type;
    typedef core::con_msg_manager_type con_msg_manager_type;
    typedef core::endpoint_msg_manager_type endpoint_msg_manager_type;
    typedef core::alog_type alog_type;
    typedef core::elog_type elog_type;
    typedef core::rng_type rng_type;
    typedef core::transport_type transport_type;
    typedef core::endpoint_base endpoint_base;
    
    // Set a custom connection_base class
    typedef connection_data connection_base;
};

typedef websocketpp::server<custom_config> server;
typedef server::connection_ptr connection_ptr;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

class print_server {
public:
    print_server() : m_next_sessionid(1) {
        m_server.init_asio();
                
        m_server.set_open_handler(bind(&print_server::on_open,this,::_1));
        m_server.set_close_handler(bind(&print_server::on_close,this,::_1));
        m_server.set_message_handler(bind(&print_server::on_message,this,::_1,::_2));
    }
    
    void on_open(connection_hdl hdl) {
        connection_ptr con = m_server.get_con_from_hdl(hdl);
        
        con->sessionid = m_next_sessionid++;
    }
    
    void on_close(connection_hdl hdl) {
        connection_ptr con = m_server.get_con_from_hdl(hdl);
        
        std::cout << "Closing connection " << con->name 
                  << " with sessionid " << con->sessionid << std::endl;
    }
    
    void on_message(connection_hdl hdl, server::message_ptr msg) {
        connection_ptr con = m_server.get_con_from_hdl(hdl);
        
        if (con->name.empty()) {
            con->name = msg->get_payload();
            std::cout << "Setting name of connection with sessionid " 
                      << con->sessionid << " to " << con->name << std::endl;
        } else {
            std::cout << "Got a message from connection " << con->name 
                      << " with sessionid " << con->sessionid << std::endl;
        }
    }
    
    void run(uint16_t port) {
        m_server.listen(port);
        m_server.start_accept();
        m_server.run();
    }
private:
    int m_next_sessionid;
    server m_server;
};

int main() {
    print_server server;
    server.run(9002);
}