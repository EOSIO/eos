#include <fc/network/ntp.hpp>
#include <fc/network/udp_socket.hpp>
#include <fc/network/resolve.hpp>
#include <fc/network/ip.hpp>
#include <fc/thread/thread.hpp>

#include <stdint.h>
#include "../byteswap.hpp"

#include <atomic>
#include <array>

namespace fc
{
  namespace detail {
  using boost::fibers::future;

  class ntp_impl
  {
    public:
      /** vector < host, port >  */
      fc::thread                                       _ntp_thread;
      std::vector< std::pair< std::string, uint16_t> > _ntp_hosts;
      future<void>                                     _read_loop_done;
      udp_socket                                       _sock;
      uint32_t                                         _request_interval_sec;
      uint32_t                                         _retry_failed_request_interval_sec;
      fc::time_point                                   _last_valid_ntp_reply_received_time;

      std::atomic_bool                                 _last_ntp_delta_initialized;
      std::atomic<int64_t>                             _last_ntp_delta_microseconds;

      future<void>                                     _request_time_task_done;

      std::shared_ptr<scheduled_task_impl<void>>       _scheduled_request_time;

      ntp_impl() :
      _ntp_thread("ntp"),
      _request_interval_sec( 60*60 /* 1 hr */),
      _retry_failed_request_interval_sec(60 * 5),
      _last_ntp_delta_microseconds(0)
      {
        _last_ntp_delta_initialized = false;
        _ntp_hosts.push_back( std::make_pair( "pool.ntp.org",123 ) );
      }

      ~ntp_impl()
      {
         _sock.close();
         if( _scheduled_request_time ) {
            _scheduled_request_time->cancel();
            _scheduled_request_time->get_future().wait();
            _scheduled_request_time.reset();
         }
      }

      fc::time_point ntp_timestamp_to_fc_time_point(uint64_t ntp_timestamp_net_order)
      {
        uint64_t ntp_timestamp_host = bswap_64(ntp_timestamp_net_order);
        uint32_t fractional_seconds = ntp_timestamp_host & 0xffffffff;
        uint32_t microseconds = (uint32_t)((((uint64_t)fractional_seconds * 1000000) + (uint64_t(1) << 31)) >> 32);
        uint32_t seconds_since_1900 = ntp_timestamp_host >> 32;
        uint32_t seconds_since_epoch = seconds_since_1900 - 2208988800;
        return fc::time_point() + fc::seconds(seconds_since_epoch) + fc::microseconds(microseconds);
      }

      uint64_t fc_time_point_to_ntp_timestamp(const fc::time_point& fc_timestamp)
      {
        uint64_t microseconds_since_epoch = (uint64_t)fc_timestamp.time_since_epoch().count();
        uint32_t seconds_since_epoch = (uint32_t)(microseconds_since_epoch / 1000000);
        uint32_t seconds_since_1900 = seconds_since_epoch + 2208988800;
        uint32_t microseconds = microseconds_since_epoch % 1000000;
        uint32_t fractional_seconds = (uint32_t)((((uint64_t)microseconds << 32) + (uint64_t(1) << 31)) / 1000000);
        uint64_t ntp_timestamp_net_order = ((uint64_t)seconds_since_1900 << 32) + fractional_seconds;
        return bswap_64(ntp_timestamp_net_order);
      }

      void request_now()
      {
        FC_ASSERT(_ntp_thread.is_current());
        for( auto item : _ntp_hosts )
        {
          try
          {
            wlog( "resolving... ${r}", ("r", item) );
            auto eps = resolve( item.first, item.second );
            for( auto ep : eps )
            {
              wlog( "sending request to ${ep}", ("ep",ep) );
              std::shared_ptr<char> send_buffer(new char[48], [](char* p){ delete[] p; });
              std::array<unsigned char, 48> packet_to_send { {010,0,0,0,0,0,0,0,0} };
              memcpy(send_buffer.get(), packet_to_send.data(), packet_to_send.size());
              uint64_t* send_buf_as_64_array = (uint64_t*)send_buffer.get();
              send_buf_as_64_array[5] = fc_time_point_to_ntp_timestamp(fc::time_point::now()); // 5 = Transmit Timestamp
              _sock.send_to(send_buffer, packet_to_send.size(), ep);
              break;
            }
          }
          catch (const fc::canceled_exception&)
          {
            throw;
          }
          catch ( const std::bad_alloc& ) 
          {
            throw;
          } 
          catch ( const boost::interprocess::bad_alloc& ) 
          {
            throw;
          } 
          // this could fail to resolve but we want to go on to other hosts..
          catch ( const fc::exception& e )
          {
            elog( "${e}", ("e",e.to_detail_string() ) );
          }
          catch ( const std::exception& e )
          {
            elog( "${e}", ("e",e.what() ) );
          }
        }
      } // request_now

      // started for first time in ntp() constructor, canceled in ~ntp() destructor
      // this task gets invoked every _retry_failed_request_interval_sec (currently 5 min), and if
      // _request_interval_sec (currently 1 hour) has passed since the last successful update,
      // it sends a new request
      void request_time_task()
      {
         request_now();
      } // request_loop

      void start_read_loop()
      {
        _read_loop_done = _ntp_thread.async( [this](){ read_loop(); } );
      }

      void read_loop()
      {
        FC_ASSERT(_ntp_thread.is_current());

        uint32_t receive_buffer_size = sizeof(uint64_t) * 1024;
        std::shared_ptr<char> receive_buffer(new char[receive_buffer_size], [](char* p){ delete[] p; });
        uint64_t* recv_buf = (uint64_t*)receive_buffer.get();

        // if you start the read while loop here, the recieve_from call will throw "invalid argument" on win32,
        // so instead we start the loop after making our first request
        _sock.open();
        _request_time_task_done = fc::async( [&](){ request_time_task(); } );

        while( true )
        {
          fc::ip::endpoint from;
          try
          {
            _sock.receive_from( receive_buffer, receive_buffer_size, from );
          //  wlog("received ntp reply from ${from}",("from",from) );
          } FC_RETHROW_EXCEPTIONS(error, "Error reading from NTP socket");

          fc::time_point receive_time = fc::time_point::now();
          fc::time_point origin_time = ntp_timestamp_to_fc_time_point(recv_buf[3]);
          fc::time_point server_receive_time = ntp_timestamp_to_fc_time_point(recv_buf[4]);
          fc::time_point server_transmit_time = ntp_timestamp_to_fc_time_point(recv_buf[5]);

          fc::microseconds offset(((server_receive_time - origin_time) +
                                   (server_transmit_time - receive_time)).count() / 2);
          fc::microseconds round_trip_delay((receive_time - origin_time) -
                                            (server_transmit_time - server_receive_time));
          //wlog("origin_time = ${origin_time}, server_receive_time = ${server_receive_time}, server_transmit_time = ${server_transmit_time}, receive_time = ${receive_time}",
          //     ("origin_time", origin_time)("server_receive_time", server_receive_time)("server_transmit_time", server_transmit_time)("receive_time", receive_time));
          // wlog("ntp offset: ${offset}, round_trip_delay ${delay}", ("offset", offset)("delay", round_trip_delay));

          //if the reply we just received has occurred more than a second after our last time request (it was more than a second ago since our last request)
          if( round_trip_delay > fc::microseconds(300000) )
          {
            wlog("received stale ntp reply requested at ${request_time}, send a new time request", ("request_time", origin_time));
            request_now(); //request another reply and ignore this one
          }
          else //we think we have a timely reply, process it
          {
            if( offset < fc::seconds(60*60*24) && offset > fc::seconds(-60*60*24) )
            {
              _last_ntp_delta_microseconds = offset.count();
              _last_ntp_delta_initialized = true;
              fc::microseconds ntp_delta_time = fc::microseconds(_last_ntp_delta_microseconds);
              _last_valid_ntp_reply_received_time = receive_time;
              wlog("ntp_delta_time updated to ${delta_time} us", ("delta_time",ntp_delta_time) );
            }
            else
              elog( "NTP time and local time vary by more than a day! ntp:${ntp_time} local:${local}",
                   ("ntp_time", receive_time + offset)("local", fc::time_point::now()) );
          }
        }
        wlog("exiting ntp read_loop");
      } //end read_loop()

      void reschedule() {
          if( _scheduled_request_time )
             _scheduled_request_time->cancel();

          _scheduled_request_time = _ntp_thread.schedule(
            [&](){
               request_now();
               reschedule();
            },
          fc::time_point::now() + fc::seconds(_request_interval_sec) );
      }
    }; //ntp_impl

  } // namespace detail



  ntp::ntp()
  :my( new detail::ntp_impl() )
  {
    my->start_read_loop();
  }

  ntp::~ntp()
  {
     ilog( "shutting down ntp" );
    my->_ntp_thread.async([=](){
      my->_sock.close();
      if( my->_scheduled_request_time ) {
         ilog( "wait cancel scheduled request " );
         my->_scheduled_request_time->cancel();
         my->_scheduled_request_time->get_future().wait();
         my->_scheduled_request_time.reset();
      }
     ilog( "wait request time task " );
      my->_request_time_task_done.wait();
     ilog( "wait read loop " );
      my->_read_loop_done.wait();
    }).wait();
    my->_ntp_thread.quit();
     ilog( "joining ntp" );
    my->_ntp_thread.join();
  }


  void ntp::add_server( const std::string& hostname, uint16_t port)
  {
    my->_ntp_thread.async( [=](){ my->_ntp_hosts.push_back( std::make_pair(hostname,port) ); }).wait();
  }

  void ntp::set_request_interval( uint32_t interval_sec )
  {
    my->_request_interval_sec = interval_sec;
    my->_retry_failed_request_interval_sec = std::min(my->_retry_failed_request_interval_sec, interval_sec);
    my->reschedule();
  }

  void ntp::request_now()
  {
    my->_ntp_thread.async( [=](){ my->request_now(); } ).get();
  }

  std::optional<time_point> ntp::get_time()const
  {
    if( my->_last_ntp_delta_initialized )
      return fc::time_point::now() + fc::microseconds(my->_last_ntp_delta_microseconds);
    return std::optional<time_point>();
  }

} //namespace fc
