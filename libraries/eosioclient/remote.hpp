#ifndef REMOTE_H
#define REMOTE_H

#include "fc/variant.hpp"

namespace eosio {
namespace client {

class Remote
{
public:
    std::string host() const;
    void set_host(const std::string& host);
    uint32_t port() const;
    void set_port(uint32_t port);

    fc::variant call(const std::string &path, const fc::variant &postdata = fc::variant()) const;

    template<typename T>
    fc::variant call(const std::string& path, const T& v ) const
    {
        return call(path, fc::variant(v));
    }

private:
    std::string m_host = "localhost";
    uint32_t m_port = 8888;
};

} // namespace client
} // namespace eosio

#endif // REMOTE_H
