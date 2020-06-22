//#ifndef _WEBSOCKETPP_CPP11_STL_
//    #define _WEBSOCKETPP_CPP11_STL_
//#endif

#include <random>
#include <boost/timer/timer.hpp>

#include <websocketpp/config/core.hpp>

//#include <websocketpp/security/none.hpp>

//#include <websocketpp/concurrency/none.hpp>
//#include <websocketpp/concurrency/stl.hpp>

//#include <websocketpp/transport/iostream.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <sstream>

//typedef websocketpp::concurrency::stl concurrency;
//typedef websocketpp::transport::iostream<concurrency> transport;
//typedef websocketpp::server<concurrency,transport> server;
typedef websocketpp::server<websocketpp::config::core> server;

/*class handler : public server::handler {
    bool validate(connection_ptr con) {
        std::cout << "handler validate" << std::endl;
        if (con->get_origin() != "http://www.example.com") {
            con->set_status(websocketpp::http::status_code::FORBIDDEN);
            return false;
        }
        return true;
    }

    void http(connection_ptr con) {
        std::cout << "handler http" << std::endl;
    }

    void on_load(connection_ptr con, ptr old_handler) {
        std::cout << "handler on_load" << std::endl;
    }
    void on_unload(connection_ptr con, ptr new_handler) {
        std::cout << "handler on_unload" << std::endl;
    }

    void on_open(connection_ptr con) {
        std::cout << "handler on_open" << std::endl;
    }
    void on_fail(connection_ptr con) {
        std::cout << "handler on_fail" << std::endl;
    }

    void on_message(connection_ptr con, message_ptr msg) {
        std::cout << "handler on_message" << std::endl;


    }

    void on_close(connection_ptr con) {
        std::cout << "handler on_close" << std::endl;
    }
};*/

int main() {
    typedef websocketpp::message_buffer::message<websocketpp::message_buffer::alloc::con_msg_manager>
        message_type;
    typedef websocketpp::message_buffer::alloc::con_msg_manager<message_type>
        con_msg_man_type;

    con_msg_man_type::ptr manager = websocketpp::lib::make_shared<con_msg_man_type>();

    size_t foo = 1024;

    message_type::ptr input = manager->get_message(websocketpp::frame::opcode::TEXT,foo);
    message_type::ptr output = manager->get_message(websocketpp::frame::opcode::TEXT,foo);
    websocketpp::frame::masking_key_type key;

    std::random_device dev;



    key.i = 0x12345678;

    double m = 18094238402394.0824923;

    /*std::cout << "Some Math" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        for (int i = 0; i < foo; i++) {
            m /= 1.001;
        }

    }*/

    std::cout << m << std::endl;

    std::cout << "Random Gen" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        input->get_raw_payload().replace(0,foo,foo,'\0');
        output->get_raw_payload().replace(0,foo,foo,'\0');
    }

    std::cout << "Out of place accelerated" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        websocketpp::frame::word_mask_exact(reinterpret_cast<uint8_t*>(const_cast<char*>(input->get_raw_payload().data())), reinterpret_cast<uint8_t*>(const_cast<char*>(output->get_raw_payload().data())), foo, key);
    }

    std::cout << websocketpp::utility::to_hex(input->get_payload().c_str(),20) << std::endl;
    std::cout << websocketpp::utility::to_hex(output->get_payload().c_str(),20) << std::endl;

    input->get_raw_payload().replace(0,foo,foo,'\0');
    output->get_raw_payload().replace(0,foo,foo,'\0');

    std::cout << "In place accelerated" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        websocketpp::frame::word_mask_exact(reinterpret_cast<uint8_t*>(const_cast<char*>(input->get_raw_payload().data())), reinterpret_cast<uint8_t*>(const_cast<char*>(input->get_raw_payload().data())), foo, key);
    }

    std::cout << websocketpp::utility::to_hex(input->get_payload().c_str(),20) << std::endl;
    std::cout << websocketpp::utility::to_hex(output->get_payload().c_str(),20) << std::endl;

    input->get_raw_payload().replace(0,foo,foo,'\0');
    output->get_raw_payload().replace(0,foo,foo,'\0');
    std::cout << "Out of place byte by byte" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        websocketpp::frame::byte_mask(input->get_raw_payload().begin(), input->get_raw_payload().end(), output->get_raw_payload().begin(), key);
    }

    std::cout << websocketpp::utility::to_hex(input->get_payload().c_str(),20) << std::endl;
    std::cout << websocketpp::utility::to_hex(output->get_payload().c_str(),20) << std::endl;

    input->get_raw_payload().replace(0,foo,foo,'\0');
    output->get_raw_payload().replace(0,foo,foo,'\0');
    std::cout << "In place byte by byte" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        websocketpp::frame::byte_mask(input->get_raw_payload().begin(), input->get_raw_payload().end(), input->get_raw_payload().begin(), key);
    }

    std::cout << websocketpp::utility::to_hex(input->get_payload().c_str(),20) << std::endl;
    std::cout << websocketpp::utility::to_hex(output->get_payload().c_str(),20) << std::endl;

    input->get_raw_payload().replace(0,foo,foo,'a');
    output->get_raw_payload().replace(0,foo,foo,'b');
    std::cout << "Copy" << std::endl;
    {
        boost::timer::auto_cpu_timer t;

        std::copy(input->get_raw_payload().begin(), input->get_raw_payload().end(), output->get_raw_payload().begin());
    }

    std::cout << websocketpp::utility::to_hex(input->get_payload().c_str(),20) << std::endl;
    std::cout << websocketpp::utility::to_hex(output->get_payload().c_str(),20) << std::endl;

    /*server::handler::ptr h(new handler());

    server test_server(h);
    server::connection_ptr con;

    std::stringstream output;

    test_server.register_ostream(&output);

    con = test_server.get_connection();

    con->start();

    //foo.handle_accept(con,true);

    std::stringstream input;
    input << "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
    //input << "GET / HTTP/1.1\r\nHost: www.example.com\r\n\r\n";
    input >> *con;

    std::stringstream input2;
    input2 << "messageabc2";
    input2 >> *con;

    std::stringstream input3;
    input3 << "messageabc3";
    input3 >> *con;

    std::stringstream input4;
    input4 << "close";
    input4 >> *con;

    std::cout << "connection output:" << std::endl;
    std::cout << output.str() << std::endl;*/
}
