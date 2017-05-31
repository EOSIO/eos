/**
 * This example is presently used as a scratch space. It may or may not be broken
 * at any given time.
 */

#include <iostream>

#include <websocketpp/config/debug_asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <websocketpp/extensions/permessage_deflate/enabled.hpp>

struct deflate_config : public websocketpp::config::debug_core {
    typedef deflate_config type;
    typedef debug_core base;
    
    typedef base::concurrency_type concurrency_type;
    
    typedef base::request_type request_type;
    typedef base::response_type response_type;

    typedef base::message_type message_type;
    typedef base::con_msg_manager_type con_msg_manager_type;
    typedef base::endpoint_msg_manager_type endpoint_msg_manager_type;
    
    typedef base::alog_type alog_type;
    typedef base::elog_type elog_type;
    
    typedef base::rng_type rng_type;
    
    struct transport_config : public base::transport_config {
        typedef type::concurrency_type concurrency_type;
        typedef type::alog_type alog_type;
        typedef type::elog_type elog_type;
        typedef type::request_type request_type;
        typedef type::response_type response_type;
        typedef websocketpp::transport::asio::basic_socket::endpoint 
            socket_type;
    };

    typedef websocketpp::transport::asio::endpoint<transport_config> 
        transport_type;
        
    /// permessage_compress extension
    struct permessage_deflate_config {};

    typedef websocketpp::extensions::permessage_deflate::enabled
        <permessage_deflate_config> permessage_deflate_type;
};

typedef websocketpp::server<deflate_config> server;

typedef server::message_ptr message_ptr;

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
    /*std::cout << "on_message called with hdl: " << hdl.lock().get() 
              << " and message (" << msg->get_payload().size() << "): " << msg->get_payload()
              << std::endl;
    */
    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e  
                  << "(" << e.message() << ")" << std::endl;
    }
}

int main(int argc, char * argv[]) {
	// Create a server endpoint
    server echo_server;
	
	try {
        // Set logging settings        
        if (argc > 1 && std::string(argv[1]) == "-d") {
            echo_server.set_access_channels(websocketpp::log::alevel::all);
            echo_server.set_error_channels(websocketpp::log::elevel::all);
        } else {
            echo_server.set_access_channels(websocketpp::log::alevel::none);
            echo_server.set_error_channels(websocketpp::log::elevel::none);
        }
        
        // Initialize ASIO
        echo_server.init_asio();
        
        // Register our message handler
        using websocketpp::lib::placeholders::_1;
        using websocketpp::lib::placeholders::_2;
        echo_server.set_message_handler(bind(&on_message,&echo_server,_1,_2));
        
        // Listen on port 9002
        echo_server.listen(9002);
        
        // Start the server accept loop
        echo_server.start_accept();
	    
	    // Start the ASIO io_service run loop
        echo_server.run();
    } catch (const std::exception & e) {
        std::cout << e.what() << std::endl;
    } catch (websocketpp::lib::error_code e) {
        std::cout << e.message() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
