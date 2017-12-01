#pragma once
#include <fc/vector.hpp>
#include <fc/network/ip.hpp>

namespace fc
{
  std::vector<boost::asio::ip::udp::endpoint> resolve(boost::asio::io_service& io_service,
                                                      const std::string& host, uint16_t port);
}
