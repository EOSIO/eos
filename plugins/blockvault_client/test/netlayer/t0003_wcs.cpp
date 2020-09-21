#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include "test_common.hpp"
#include <atomic>
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
using namespace std;

bool get_connection (net::io_context & ioc,  tcp::resolver *& presolver, websocket::stream<tcp::socket> *& pws,  char * host, char * port)
{
        // These objects perform our I/O
      presolver = new tcp::resolver {ioc};
      pws = new websocket::stream<tcp::socket>{ioc};
      if(presolver == nullptr || pws == nullptr){
         std::cout << "error happen in get_connection" << std::endl;
         return false;
     }
        // Look up the domain name
        auto const results = presolver->resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        net::connect(pws->next_layer(), results.begin(), results.end());

        // Set a decorator to change the User-Agent of the handshake
        pws->set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
            }));

        // Perform the websocket handshake
        pws->handshake(host, "/");
        return true;

}

bool send_a_msg(websocket::stream<tcp::socket> & ws,  std::string & msg){
        // Send the message
        //cout << "begin:" << endl;
        ws.write(net::buffer(msg));
        //cout << "ws.write" << endl;
        // This buffer will hold the incoming message
        beast::flat_buffer buffer;
        // Read a message into our buffer
        ws.read(buffer);
        //std::cout << beast::make_printable(buffer.data()) << std::endl;
        return true;
}


// Sends a WebSocket message and prints the response
int main(int argc, char** argv)
{
   try
   {
        // Check command line arguments.

      char * default_address="127.0.0.1";
      char * default_port="8080";
      char * config_address;
      char * config_port;
      if (argc == 3 )
        {
            //std::cerr <<
            //    "Usage: websocket-server-sync <address> <port>\n" <<
            //    "Example:\n" <<
            //    "    websocket-server-sync 0.0.0.0 8080\n";
            //return EXIT_FAILURE;
            config_address = argv[1];
            config_port = argv[2];
        }else if(argc == 2){
           config_address = argv[1];
           config_port = default_port;
        }else {
           config_address = default_address;
           config_port = default_port;
        }
      
      uint64_t t1 = my_clock();
        
      auto const host = config_address;
      auto const port = config_port;

        // The io_context is required for all I/O
      net::io_context ioc[CONN_NUM];
      tcp::resolver * presolver[CONN_NUM];
      websocket::stream<tcp::socket> * pws[CONN_NUM];

      for(int i = 0; i < CONN_NUM; ++i){
         bool ret = get_connection (ioc[i],  presolver[i], pws[i], host, port);
         if(!ret) {
            cout << "get_conn error happened !" << endl;
            break;
         }
      }
      std::vector<std::thread> vec_thread(CONN_NUM);
      uint64_t t2 = my_clock();
      
      //for(int i = 0; i < CONN_NUM; ++i){
      int i = 0;
      std::atomic<int> msg_count = 0;
      for(auto it = std::begin(vec_thread); it != std::end(vec_thread) ; ++it, ++i) {
         *it = std::thread(
             [=, &msg_count](){
             //cout << "i = " << i << endl;
             for(int j = 0; j < MSG_NUM; ++j){
                auto  msg = get_random_req_string(MSG_LEN);
                //std::cout << msg << std::endl;
                send_a_msg(*(pws[i]), msg);
                ++msg_count;
             }
         });
      }
      
      for (std::thread & th : vec_thread){
         // If thread Object is Joinable then Join that thread.
         if (th.joinable())
            th.join();
      }
      std::cout << "MSG OK! msg_cout = " << msg_count  << std::endl;

      uint64_t t3 = my_clock();
      
      for(int i = 0; i < CONN_NUM; ++i){
                 // Close the WebSocket connection
         pws[i]->close(websocket::close_code::normal);
         delete pws[i];
         delete presolver[i];
      }
      uint64_t t4 = my_clock();
      std::cout << "us time "<<t4 - t1<<"us,conn use time : " << t2 - t1 << " us,  msg use time " << t3-t2 << " us, close use time" << t4 - t3 <<" us." << std::endl;

      
        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        //std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

