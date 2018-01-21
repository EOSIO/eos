#ifndef EOSIOCLIENT_HPP
#define EOSIOCLIENT_HPP

#include "eosio/chain_plugin/chain_plugin.hpp"
#include "functions.hpp"

namespace eosio {
namespace client {

class Eosioclient
{
public:
    eosio::chain_apis::read_only::get_info_results get_info() const;
    fc::variant get_code(std::string account_name) const;

    std::string host = "localhost"; /// @todo make private
    uint32_t port = 8888; /// @todo make private

private:
    fc::variant call( const std::string& server, uint16_t port,
                      const std::string& path,
                      const fc::variant& postdata = fc::variant() ) const;

    template<typename T>
    fc::variant call( const std::string& server, uint16_t port,
                      const std::string& path,
                      const T& v ) { return call( server, port, path, fc::variant(v) ); }

    template<typename T>
    fc::variant call( const std::string& path,
                      const T& v ) { return call( host, port, path, fc::variant(v) ); }
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_HPP
