#ifndef REMOTE_H
#define REMOTE_H

#include "fc/variant.hpp"

namespace eosio {
namespace client {

class Remote
{
public:
    fc::variant call(const std::string &m_server_host, uint16_t m_server_port, const std::string &path, const fc::variant &postdata = fc::variant()) const;

    template<typename T>
    fc::variant call(const std::string &m_server_host, uint16_t m_server_port, const std::string& path, const T& v ) const
    {
        return call(m_server_host, m_server_port, path, fc::variant(v));
    }
};

} // namespace client
} // namespace eosio

#endif // REMOTE_H
