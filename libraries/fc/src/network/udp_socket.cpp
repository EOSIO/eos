#include <fc/network/udp_socket.hpp>
#include <fc/network/ip.hpp>
#include <fc/fwd_impl.hpp>
#include <fc/asio.hpp>


namespace fc {
  using boost::fibers::promise;
  
  class udp_socket::impl {
    public:
      impl():_sock( fc::asio::default_io_service() ){}
      ~impl(){ }

      boost::asio::ip::udp::socket _sock;
  };

  boost::asio::ip::udp::endpoint to_asio_ep( const fc::ip::endpoint& e ) {
    return boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4(e.get_address()), e.port() );
  }
  fc::ip::endpoint to_fc_ep( const boost::asio::ip::udp::endpoint& e ) {
    return fc::ip::endpoint( e.address().to_v4().to_ulong(), e.port() );
  }

  udp_socket::udp_socket()
  :my( new impl() ) 
  {
  }

  udp_socket::~udp_socket() 
  {
    try 
    {
      my->_sock.close(); //close boost socket to make any pending reads run their completion handler
    }
    catch (...) //avoid destructor throw and likely this is just happening because socket wasn't open.
    {
    }
  }

  size_t udp_socket::send_to( const char* buffer, size_t length, const ip::endpoint& to ) 
  {
    try 
    {
      return my->_sock.send_to( boost::asio::buffer(buffer, length), to_asio_ep(to) );
    } 
    catch( const boost::system::system_error& e ) 
    {
      if( e.code() != boost::asio::error::would_block )
        throw;
    }

    auto completion_promise = std::make_shared<promise<size_t>>();
    my->_sock.async_send_to( boost::asio::buffer(buffer, length), to_asio_ep(to), 
                             asio::detail::read_write_handler(completion_promise) );

    return completion_promise->get_future().get();
  }

  size_t udp_socket::send_to( const std::shared_ptr<const char>& buffer, size_t length, 
                              const fc::ip::endpoint& to )
  {
    try 
    {
      return my->_sock.send_to( boost::asio::buffer(buffer.get(), length), to_asio_ep(to) );
    } 
    catch( const boost::system::system_error& e ) 
    {
      if( e.code() != boost::asio::error::would_block )
        throw;
    }

    auto completion_promise = std::make_shared<promise<size_t>>();
    my->_sock.async_send_to( boost::asio::buffer(buffer.get(), length), to_asio_ep(to), 
                             asio::detail::read_write_handler_with_buffer(completion_promise, buffer) );

    return completion_promise->get_future().get();
  }

  void udp_socket::open() {
    my->_sock.open( boost::asio::ip::udp::v4() );
    my->_sock.non_blocking(true);
  }
  void udp_socket::set_receive_buffer_size( size_t s ) {
    my->_sock.set_option(boost::asio::socket_base::receive_buffer_size(s) );
  }
  void udp_socket::bind( const fc::ip::endpoint& e ) {
    my->_sock.bind( to_asio_ep(e) );
  }

  size_t udp_socket::receive_from( const std::shared_ptr<char>& receive_buffer, size_t receive_buffer_length, fc::ip::endpoint& from )
  {
    try 
    {
      boost::asio::ip::udp::endpoint boost_from_endpoint;
      size_t bytes_read = my->_sock.receive_from( boost::asio::buffer(receive_buffer.get(), receive_buffer_length), 
                                                  boost_from_endpoint );
      from = to_fc_ep(boost_from_endpoint);
      return bytes_read;
    } 
    catch( const boost::system::system_error& e ) 
    {
      if( e.code() != boost::asio::error::would_block ) 
        throw;
    }

    boost::asio::ip::udp::endpoint boost_from_endpoint;
    auto completion_promise = std::make_shared<promise<size_t>>();
    my->_sock.async_receive_from( boost::asio::buffer(receive_buffer.get(), receive_buffer_length), 
                                  boost_from_endpoint,
                                  asio::detail::read_write_handler_with_buffer(completion_promise, receive_buffer) );
    size_t bytes_read = completion_promise->get_future().get();
    from = to_fc_ep(boost_from_endpoint);
    return bytes_read;
  }

  size_t udp_socket::receive_from( char* receive_buffer, size_t receive_buffer_length, fc::ip::endpoint& from ) 
  {
    try 
    {
      boost::asio::ip::udp::endpoint boost_from_endpoint;
      size_t bytes_read = my->_sock.receive_from( boost::asio::buffer(receive_buffer, receive_buffer_length), 
                                                  boost_from_endpoint );
      from = to_fc_ep(boost_from_endpoint);
      return bytes_read;
    } 
    catch( const boost::system::system_error& e ) 
    {
      if( e.code() != boost::asio::error::would_block ) 
        throw;
    }

    boost::asio::ip::udp::endpoint boost_from_endpoint;
    auto completion_promise = std::make_shared<promise<size_t>>();
    my->_sock.async_receive_from( boost::asio::buffer(receive_buffer, receive_buffer_length), boost_from_endpoint,
                                  asio::detail::read_write_handler(completion_promise) );
    size_t bytes_read = completion_promise->get_future().get();
    from = to_fc_ep(boost_from_endpoint);
    return bytes_read;
  }

  void   udp_socket::close() {
    my->_sock.close();
  }

  fc::ip::endpoint udp_socket::local_endpoint()const {
    return to_fc_ep( my->_sock.local_endpoint() );
  }
  void udp_socket::connect( const fc::ip::endpoint& e ) {
     my->_sock.connect( to_asio_ep(e) );
  }

  void   udp_socket::set_multicast_enable_loopback( bool s )
  {
    my->_sock.set_option( boost::asio::ip::multicast::enable_loopback(s) );
  }
  void   udp_socket::set_reuse_address( bool s )
  {
    my->_sock.set_option( boost::asio::ip::udp::socket::reuse_address(s) );
  }
  void   udp_socket::join_multicast_group( const fc::ip::address& a )
  {
    my->_sock.set_option( boost::asio::ip::multicast::join_group( boost::asio::ip::address_v4(a) ) );
  }

}
