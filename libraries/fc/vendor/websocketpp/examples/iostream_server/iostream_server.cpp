#include <websocketpp/config/core.hpp>

#include <websocketpp/server.hpp>

#include <iostream>
#include <fstream>

typedef websocketpp::server<websocketpp::config::core> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        s->get_alog().write(websocketpp::log::alevel::app,
                    "Text Message Received: "+msg->get_payload());
    } else {
        s->get_alog().write(websocketpp::log::alevel::app,
                    "Binary Message Received: "+websocketpp::utility::to_hex(msg->get_payload()));
    }

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        s->get_alog().write(websocketpp::log::alevel::app,
                    "Echo Failed: "+e.message());
    }
}

int main() {
    server s;
    std::ofstream log;

    try {
        // set up access channels to only log interesting things
        s.clear_access_channels(websocketpp::log::alevel::all);
        s.set_access_channels(websocketpp::log::alevel::connect);
        s.set_access_channels(websocketpp::log::alevel::disconnect);
        s.set_access_channels(websocketpp::log::alevel::app);

        // Log to a file rather than stdout, as we are using stdout for real
        // output
        log.open("output.log");
        s.get_alog().set_ostream(&log);
        s.get_elog().set_ostream(&log);

        // print all output to stdout
        s.register_ostream(&std::cout);

        // Register our message handler
        s.set_message_handler(bind(&on_message,&s,::_1,::_2));

        server::connection_ptr con = s.get_connection();

        con->start();

        // C++ iostream's don't support the idea of asynchronous i/o. As such
        // there are two input strategies demonstrated here. Buffered I/O will
        // read from stdin in chunks until EOF. This works very well for
        // replaying canned connections as would be done in automated testing.
        //
        // If the server is being used live however, assuming input is being
        // piped from elsewhere in realtime, this strategy will result in small
        // messages being buffered forever. The non-buffered strategy below
        // reads characters from stdin one at a time. This is inefficient and
        // for more serious uses should be replaced with a platform specific
        // asyncronous i/o technique like select, poll, IOCP, etc
        bool buffered_io = false;

        if (buffered_io) {
            std::cin >> *con;
            con->eof();
        } else {
            char a;
            while(std::cin.get(a)) {
                con->read_some(&a,1);
            }
            con->eof();
        }
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
    log.close();
}
