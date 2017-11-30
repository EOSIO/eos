#include <fc/network/udp_socket.hpp>
#include <fc/network/ip.hpp>
#include <fc/fwd_impl.hpp>

namespace fc
{
  class udp_socket::impl : public fc::retainable
  {
    public:
      impl(){}
      ~impl(){}

      std::shared_ptr<boost::asio::ip::udp::socket> _sock;
  };

  udp_socket::udp_socket()
    : my(new impl())
  {
  }

  void udp_socket::initialize(boost::asio::io_service& service)
  {
    my->_sock.reset(new boost::asio::ip::udp::socket(service));
  }

  udp_socket::~udp_socket() 
  {
    try 
    {
      if(my->_sock)
        my->_sock->close(); //close boost socket to make any pending reads run their completion handler
    }
    catch (...) //avoid destructor throw and likely this is just happening because socket wasn't open.
    {
    }
  }

  void udp_socket::send_to(const char* buffer, size_t length, boost::asio::ip::udp::endpoint& to)
  {
    try 
    {
      my->_sock->send_to(boost::asio::buffer(buffer, length), to);
    } 
    catch(const boost::system::system_error& e)
    {
      if(e.code() != boost::asio::error::would_block)
        throw;
    }

    my->_sock->async_send_to(boost::asio::buffer(buffer, length), to,
                             [this](const boost::system::error_code& /*ec*/, std::size_t /*bytes_transferred*/)
    {
      // Swallow errors.  Currently only used for GELF logging, so depend on local
      // log to catch anything that doesn't make it across the network.
    });
  }

  void udp_socket::send_to(const std::shared_ptr<const char>& buffer, size_t length,
                           boost::asio::ip::udp::endpoint& to)
  {
    try
    {
      my->_sock->send_to(boost::asio::buffer(buffer.get(), length), to);
    } 
    catch(const boost::system::system_error& e)
    {
      if(e.code() != boost::asio::error::would_block)
        throw;
    }

    my->_sock->async_send_to(boost::asio::buffer(buffer.get(), length), to,
                             [this](const boost::system::error_code& /*ec*/, std::size_t /*bytes_transferred*/)
    {
      // Swallow errors.  Currently only used for GELF logging, so depend on local
      // log to catch anything that doesn't make it across the network.
    });
  }

  void udp_socket::open()
  {
    my->_sock->open(boost::asio::ip::udp::v4());
    my->_sock->non_blocking(true);
  }

  void udp_socket::close()
  {
    my->_sock->close();
  }

  const boost::asio::ip::udp::endpoint udp_socket::local_endpoint() const
  {
    return my->_sock->local_endpoint();
  }

  void udp_socket::connect(const boost::asio::ip::udp::endpoint& e)
  {
    my->_sock->connect(e);
  }

  void udp_socket::set_reuse_address( bool s )
  {
    my->_sock->set_option( boost::asio::ip::udp::socket::reuse_address(s) );
  }

}
