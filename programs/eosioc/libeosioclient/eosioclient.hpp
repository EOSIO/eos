#ifndef EOSIOCLIENT_HPP
#define EOSIOCLIENT_HPP

#include "fc/variant.hpp"

namespace eosio {
namespace client {

class Eosioclient
{
public:
    fc::variant get_info() const;
    fc::variant get_code(const std::string &account_name) const;
    fc::variant get_table(const std::string &scope, const std::string &code, const std::string &table) const;

    std::string host = "localhost"; /// @todo make private
    uint32_t port = 8888; /// @todo make private

private:
    fc::variant call(const std::string& path, const fc::variant& postdata = fc::variant() ) const;
};

} // namespace client
} // namespace eosio

#endif // EOSIOCLIENT_HPP
