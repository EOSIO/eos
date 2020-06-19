#include <iostream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;
using websocketpp::lib::ref;

void custom_on_msg(server & s, connection_hdl hdl, server::message_ptr msg) {
        std::cout << "Message sent to custom handler" << std::endl;
}

void default_on_msg(server & s, connection_hdl hdl, server::message_ptr msg) {
    std::cout << "Message sent to default handler" << std::endl;

    if (msg->get_payload() == "upgrade") {
        // Upgrade our connection_hdl to a full connection_ptr
        server::connection_ptr con = s.get_con_from_hdl(hdl);

        // Change the on message handler for this connection only to
        // custom_on_mesage
        con->set_message_handler(bind(&custom_on_msg,ref(s),::_1,::_2));
        std::cout << "Upgrading connection to custom handler" << std::endl;
    }
}

int main() {
    server s;

    s.set_message_handler(bind(&default_on_msg,ref(s),::_1,::_2));

    s.init_asio();
    s.listen(9002);
    s.start_accept();

    s.run();
}
