#include <eosio/libeosioc/remote.hpp>

#include <boost/asio.hpp>
#include <fc/variant_object.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/json.hpp>

using boost::asio::ip::tcp;

namespace eosio {
namespace client {

std::string remote::host() const
{
    return m_host;
}

void remote::set_host(const std::string &host)
{
    m_host = host;
}

uint32_t remote::port() const
{
    return m_port;
}

void remote::set_port(uint32_t port)
{
    m_port = port;
}

fc::variant remote::call(const std::string &path, const fc::variant &postdata) const
{
    try {
        std::string postjson;
        if( !postdata.is_null() )
            postjson = fc::json::to_string( postdata );

        boost::asio::io_service io_service;

        // Get a list of endpoints corresponding to the server name.
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(m_host, std::to_string(m_port) );
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
        tcp::resolver::iterator end;

        while( endpoint_iterator != end ) {
            // Try each endpoint until we successfully establish a connection.
            tcp::socket socket(io_service);
            try {
                boost::asio::connect(socket, endpoint_iterator);
                endpoint_iterator = end;
            } catch( std::exception& e ) {
                ++endpoint_iterator;
                if( endpoint_iterator != end )
                    continue;
                else throw;
            }

            // Form the request. We specify the "Connection: close" header so that the
            // server will close the socket after transmitting the response. This will
            // allow us to treat all data up until the EOF as the content.
            boost::asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << "POST " << path << " HTTP/1.0\r\n";
            request_stream << "Host: " << m_host << "\r\n";
            request_stream << "content-length: " << postjson.size() << "\r\n";
            request_stream << "Accept: */*\r\n";
            request_stream << "Connection: close\r\n\r\n";
            request_stream << postjson;

            // Send the request.
            boost::asio::write(socket, request);

            // Read the response status line. The response streambuf will automatically
            // grow to accommodate the entire line. The growth may be limited by passing
            // a maximum size to the streambuf constructor.
            boost::asio::streambuf response;
            boost::asio::read_until(socket, response, "\r\n");

            // Check that response is OK.
            std::istream response_stream(&response);
            std::string http_version;
            response_stream >> http_version;
            unsigned int status_code;
            response_stream >> status_code;
            std::string status_message;
            std::getline(response_stream, status_message);
            FC_ASSERT( !(!response_stream || http_version.substr(0, 5) != "HTTP/"), "Invalid Response" );

            // Read the response headers, which are terminated by a blank line.
            boost::asio::read_until(socket, response, "\r\n\r\n");

            // Process the response headers.
            std::string header;
            while (std::getline(response_stream, header) && header != "\r")
            {
                //         std::cout << header << "\n";
            }
            //      std::cout << "\n";

            std::stringstream re;
            // Write whatever content we already have to output.
            if (response.size() > 0)
                //   std::cout << &response;
                re << &response;

            // Read until EOF, writing data to output as we go.
            boost::system::error_code error;
            while (boost::asio::read(socket, response,
                                     boost::asio::transfer_at_least(1), error))
                re << &response;

            if (error != boost::asio::error::eof)
                throw boost::system::system_error(error);

            //  std::cout << re.str() <<"\n";
            if( status_code == 200 || status_code == 201 || status_code == 202 ) {
                return fc::json::from_string(re.str());
            }

            FC_ASSERT( status_code == 200, "Error code ${c}\n: ${msg}\n", ("c", status_code)("msg", re.str()) );
        }

        FC_ASSERT( !"unable to connect" );
    } FC_CAPTURE_AND_RETHROW( (m_host)(m_port)(path)(postdata) )
}

} // namespace client
} // namespace eosio
