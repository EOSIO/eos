#include <boost/asio.hpp>
#include <fc/exception/exception.hpp>

namespace fc
{
  std::vector<boost::asio::ip::udp::endpoint> resolve(boost::asio::io_service& io_service,
                                                     const std::string& host, uint16_t port)
  {
    using q = boost::asio::ip::udp::resolver::query;
    using b = boost::asio::ip::resolver_query_base;
    boost::asio::ip::udp::resolver res(io_service);
    boost::system::error_code ec;
    auto ep = res.resolve(q(host, std::to_string(uint64_t(port)),
                            b::address_configured | b::numeric_service), ec);
    if(!ec)
    {
      std::vector<boost::asio::ip::udp::endpoint> eps;
      while(ep != boost::asio::ip::udp::resolver::iterator())
      {
        if(ep->endpoint().address().is_v4())
        {
         eps.push_back(*ep);
        }
        // TODO: add support for v6
        ++ep;
      }
      return eps;
    }
    FC_THROW_EXCEPTION(unknown_host_exception,
                       "name resolution failed: ${reason}",
                       ("reason", ec.message()));
  }
}
