#ifndef EOSIOCLIENT_HPP
#define EOSIOCLIENT_HPP

#include "eosio/chain_plugin/chain_plugin.hpp"
#include "functions.hpp"

class Eosioclient
{
public:
    eosio::chain_apis::read_only::get_info_results get_info();

private:
    fc::variant call( const std::string& server, uint16_t port,
                      const std::string& path,
                      const fc::variant& postdata = fc::variant() );

    template<typename T>
    fc::variant call( const std::string& server, uint16_t port,
                      const std::string& path,
                      const T& v ) { return call( server, port, path, fc::variant(v) ); }

    template<typename T>
    fc::variant call( const std::string& path,
                      const T& v ) { return call( host, port, path, fc::variant(v) ); }

    std::string host = "localhost";
    uint32_t port = 8888;
};

#endif // EOSIOCLIENT_HPP
