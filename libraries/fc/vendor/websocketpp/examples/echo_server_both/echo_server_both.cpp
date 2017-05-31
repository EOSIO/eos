#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <iostream>

// define types for two different server endpoints, one for each config we are
// using
typedef websocketpp::server<websocketpp::config::asio> server_plain;
typedef websocketpp::server<websocketpp::config::asio_tls> server_tls;

// alias some of the bind related functions as they are a bit long
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// type of the ssl context pointer is long so alias it
typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;

// The shared on_message handler takes a template parameter so the function can
// resolve any endpoint dependent types like message_ptr or connection_ptr
template <typename EndpointType>
void on_message(EndpointType* s, websocketpp::connection_hdl hdl,
    typename EndpointType::message_ptr msg)
{
    std::cout << "on_message called with hdl: " << hdl.lock().get()
              << " and message: " << msg->get_payload()
              << std::endl;

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e
                  << "(" << e.message() << ")" << std::endl;
    }
}

// No change to TLS init methods from echo_server_tls
std::string get_password() {
    return "test";
}

context_ptr on_tls_init(websocketpp::connection_hdl hdl) {
    std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
    context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));

    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::no_sslv3 |
                         boost::asio::ssl::context::single_dh_use);
        ctx->set_password_callback(bind(&get_password));
        ctx->use_certificate_chain_file("server.pem");
        ctx->use_private_key_file("server.pem", boost::asio::ssl::context::pem);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}

int main() {
    // set up an external io_service to run both endpoints on. This is not
    // strictly necessary, but simplifies thread management a bit.
    boost::asio::io_service ios;

    // set up plain endpoint
    server_plain endpoint_plain;
    // initialize asio with our external io_service rather than an internal one
    endpoint_plain.init_asio(&ios);
    endpoint_plain.set_message_handler(
        bind(&on_message<server_plain>,&endpoint_plain,::_1,::_2));
    endpoint_plain.listen(80);
    endpoint_plain.start_accept();

    // set up tls endpoint
    server_tls endpoint_tls;
    endpoint_tls.init_asio(&ios);
    endpoint_tls.set_message_handler(
        bind(&on_message<server_tls>,&endpoint_tls,::_1,::_2));
    // TLS endpoint has an extra handler for the tls init
    endpoint_tls.set_tls_init_handler(bind(&on_tls_init,::_1));
    // tls endpoint listens on a different port
    endpoint_tls.listen(443);
    endpoint_tls.start_accept();

    // Start the ASIO io_service run loop running both endpoints
    ios.run();
}
