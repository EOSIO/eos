#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <list>
#include "test_common.hpp"
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;
    std::list<std::string> m_sendQueue;
    bool connected;
    int read_cnt;


public:
    bool is_connected(){return connected; }
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc)), connected (false),read_cnt(0)
    {
       
    }

    // Start the asynchronous operation
    void
    start_connect(
        char const* host,
        char const* port,
        char const* text)
    {
        // Save these for later
        host_ = host;
        text_ = text;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if(ec)
            return fail(ec, "connect");

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void
    on_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");
        connected = true;
        // Send the message
      //  ws_.async_write(
      //      net::buffer(text_),
      //      beast::bind_front_handler(
      //          &session::on_write,
      //          shared_from_this()));
    }

    void write(std::string & msg)
    {
       ws_.async_write(
            net::buffer(msg),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void read()
    {
              ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
      }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");
        
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");
        // The make_printable() function helps print a ConstBufferSequence
        std::cout << read_cnt++ << ":"<< beast::make_printable(buffer_.data()) << std::endl;
        read();
    }

    void close()
    {
        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));
    }
    void
    on_close(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

    }


void HandleAsyncWrite(std::string  msg)
    {
        bool write_in_progress = !m_sendQueue.empty();
        m_sendQueue.emplace_back(msg);
        if (!write_in_progress)
        {
            AsyncWrite();
        }
    }

    void AsyncWrite()
    {
        auto msg = m_sendQueue.front();
        ws_.async_write(
            net::buffer(msg),
            beast::bind_front_handler(
                [this](const boost::system::error_code& ec, std::size_t size)
        {
            if (!ec)
            {
                m_sendQueue.pop_front();

                if (!m_sendQueue.empty())
                {
                    AsyncWrite();
                }
            }
            else
            {
                std::cout << "error code : " <<  ec <<  std::endl;
                if (!m_sendQueue.empty())
                    m_sendQueue.clear();
            }
         }));
    }

};

//------------------------------------------------------------------------------




int main(int argc, char** argv)
{
    // Check command line arguments.
    //if(argc != 4)
    //{
    //    std::cerr <<
    //        "Usage: websocket-client-async <host> <port> <text>\n" <<
    //        "Example:\n" <<
    //        "    websocket-client-async echo.websocket.org 80 \"Hello, world!\"\n";
    //    return EXIT_FAILURE;
    //}

      char * default_address="127.0.0.1";
      char * default_port="8080";
      char * default_text = "default message";
      char * config_address;
      char * config_port;
      char * config_text;
      if (argc == 4 )
        {
            //std::cerr <<
            //    "Usage: websocket-server-sync <address> <port>\n" <<
            //    "Example:\n" <<
            //    "    websocket-server-sync 0.0.0.0 8080\n";
            //return EXIT_FAILURE;
            config_address = argv[1];
            config_port = argv[2];
            config_text = argv[3];
        }else if(argc == 3){
           config_address = argv[1];
           config_port = argv[2];
           config_text = default_text;
        }else if(argc == 2){
           config_address = argv[1];
           config_port = default_port;
           config_text = default_text;
        }else{
           config_address = default_address;
           config_port = default_port;
           config_text = default_text;
        }
         
      
      uint64_t t1 = my_clock();
        
      auto const host = config_address;
      auto const port = config_port;
      auto const text = config_text;

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    auto psession = std::make_shared<session>(ioc);
    psession->start_connect(host, port, text);

   

    // Run the I/O service. The call will return when
    // the socket is closed
    std::function<void(void)> ioc_run  = [&ioc]
        {
            ioc.run();
        };
    std::thread th(ioc_run);

    int i = 0;
    while(true){
      if (psession->is_connected()) break;
      std::cout << "connect..." << i++ << std::endl;
    }
    for(i = 0; i < MSG_NUM; ++i){
      std::string msg = get_random_req_string(MSG_LEN);
      psession->HandleAsyncWrite (std::string("sssssssssss"));
    }
    //psession->close();
    psession->read();

    th.join();


    return EXIT_SUCCESS;
}
