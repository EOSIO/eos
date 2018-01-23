#include "eosioclient.hpp"

#include <boost/asio.hpp>

#include "functions.hpp"
#include "fc/variant_object.hpp"
#include "fc/exception/exception.hpp"
#include "fc/io/json.hpp"

using boost::asio::ip::tcp;

namespace eosio {
namespace client {

fc::variant Eosioclient::get_info() const
{
    return call_server(get_info_func);
}

fc::variant Eosioclient::get_code(const std::string &account_name) const
{
    return call_server(get_code_func, fc::mutable_variant_object("account_name", account_name));
}

fc::variant Eosioclient::get_table(const std::string& scope, const std::string& code, const std::string& table) const
{
    bool binary = false;
    return call_server(get_table_func, fc::mutable_variant_object("json", !binary)
                ("scope",scope)
                ("code",code)
                ("table",table)
                       );
}

fc::variant Eosioclient::push_transaction(const chain::signed_transaction &transaction) const
{
    return call_server(push_txn_func, transaction);
}

fc::variant Eosioclient::push_transaction(const fc::variant &transaction) const
{
    return call_server(push_txn_func, transaction);
}

void Eosioclient::sign_transaction(eosio::chain::signed_transaction& trx) {
   // TODO better error checking
   const auto& public_keys = call_wallet(wallet_public_keys);
   auto get_arg = fc::mutable_variant_object
         ("transaction", trx)
         ("available_keys", public_keys);
   const auto& required_keys = call_server(get_required_keys, get_arg);
   // TODO determine chain id
   fc::variants sign_args = {fc::variant(trx), required_keys["required_keys"], fc::variant(eosio::chain::chain_id_type{})};
   const auto& signed_trx = call_wallet(wallet_sign_trx, sign_args);
   trx = signed_trx.as<eosio::chain::signed_transaction>();
}

fc::variant Eosioclient::call_wallet(const std::string &path, const fc::variant &postdata) const
{
    return call(wallet_host, wallet_port, path, postdata);
}

fc::variant Eosioclient::call_server(const std::string &path, const fc::variant &postdata) const
{
    return call(host, port, path, postdata);
}

fc::variant Eosioclient::call(const std::string& host, uint16_t port, const std::string &path, const fc::variant &postdata) const
{
    try {
        std::string postjson;
        if( !postdata.is_null() )
            postjson = fc::json::to_string( postdata );

        boost::asio::io_service io_service;

        // Get a list of endpoints corresponding to the server name.
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, std::to_string(port) );
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
            request_stream << "Host: " << host << "\r\n";
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
    } FC_CAPTURE_AND_RETHROW( (host)(port)(path)(postdata) )
}

} // namespace client
} // namespace eosio
