/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <fc/variant.hpp>

namespace eosio {
namespace client {

class remote
{
public:
    virtual ~remote() = default;

    std::string host() const;
    void set_host(const std::string& host);
    uint32_t port() const;
    void set_port(uint32_t port);

protected:
    remote() = default;

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

