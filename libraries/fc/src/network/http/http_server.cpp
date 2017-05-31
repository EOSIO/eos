#include <fc/network/http/server.hpp>
#include <fc/thread/thread.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/io/sstream.hpp>
#include <fc/network/ip.hpp>
#include <fc/io/stdio.hpp>
#include <fc/log/logger.hpp>


namespace fc { namespace http {

  class server::response::impl : public fc::retainable
  {
    public:
      impl( const fc::http::connection_ptr& c, const std::function<void()>& cont = std::function<void()>() )
      :body_bytes_sent(0),body_length(0),con(c),handle_next_req(cont)
      {}

      void send_header() {
         //ilog( "sending header..." );
         fc::stringstream ss;
         ss << "HTTP/1.1 " << rep.status << " ";
         switch( rep.status ) {
            case fc::http::reply::OK: ss << "OK\r\n"; break;
            case fc::http::reply::RecordCreated: ss << "Record Created\r\n"; break;
            case fc::http::reply::NotFound: ss << "Not Found\r\n"; break;
            case fc::http::reply::Found: ss << "Found\r\n"; break;
            default: ss << "Internal Server Error\r\n"; break;
         }
         for( uint32_t i = 0; i < rep.headers.size(); ++i ) {
            ss << rep.headers[i].key <<": "<<rep.headers[i].val <<"\r\n";
         }
         ss << "Content-Length: "<<body_length<<"\r\n\r\n";
         auto s = ss.str();
         //fc::cerr<<s<<"\n";
         con->get_socket().write( s.c_str(), s.size() );
      }

      http::reply           rep;
      int64_t               body_bytes_sent;
      uint64_t              body_length;
      http::connection_ptr      con;
      std::function<void()> handle_next_req;
  };


  class server::impl 
  {
    public:
      impl(){}

      impl(const fc::ip::endpoint& p ) 
      {
        tcp_serv.set_reuse_address();
        tcp_serv.listen(p);
        accept_complete = fc::async([this](){ this->accept_loop(); }, "http_server accept_loop");
      }

      ~impl() 
      {
        try
        {
          tcp_serv.close();
          if (accept_complete.valid())
            accept_complete.wait();
        }
        catch (...) 
        {
        }

        for (fc::future<void>& request_in_progress : requests_in_progress)
        {
          try
          {
            request_in_progress.cancel_and_wait();
          }
          catch (const fc::exception& e)
          {
            wlog("Caught exception while canceling http request task: ${error}", ("error", e));
          }
          catch (const std::exception& e)
          {
            wlog("Caught exception while canceling http request task: ${error}", ("error", e.what()));
          }
          catch (...)
          {
            wlog("Caught unknown exception while canceling http request task");
          }
        }
        requests_in_progress.clear();
      }

      void accept_loop() 
      {
        while( !accept_complete.canceled() )
        {
          http::connection_ptr con = std::make_shared<http::connection>();
          tcp_serv.accept( con->get_socket() );
          //ilog( "Accept Connection" );
          // clean up futures for any completed requests
          for (auto iter = requests_in_progress.begin(); iter != requests_in_progress.end();)
            if (!iter->valid() || iter->ready())
              iter = requests_in_progress.erase(iter);
            else
              ++iter;
          requests_in_progress.emplace_back(fc::async([=](){ handle_connection(con, on_req); }, "http_server handle_connection"));
        }
      }

      void handle_connection( const http::connection_ptr& c,  
                              std::function<void(const http::request&, const server::response& s )> do_on_req ) 
      {
        try 
        {
          http::server::response rep( fc::shared_ptr<response::impl>( new response::impl(c) ) );
          request req = c->read_request();
          if( do_on_req ) 
            do_on_req( req, rep );
          c->get_socket().close();
        } 
        catch ( fc::exception& e ) 
        {
          wlog( "unable to read request ${1}", ("1", e.to_detail_string() ) );//fc::except_str().c_str());
        }
        //wlog( "done handle connection" );
      }

      fc::future<void>                                                      accept_complete;
      std::function<void(const http::request&, const server::response& s)>  on_req;
      std::vector<fc::future<void> >                                        requests_in_progress;
      fc::tcp_server                                                        tcp_serv;
  };



  server::server():my( new impl() ){}
  server::server( uint16_t port ) :my( new impl(fc::ip::endpoint( fc::ip::address(),port)) ){}
  server::server( server&& s ):my(fc::move(s.my)){}

  server& server::operator=(server&& s)      { fc_swap(my,s.my); return *this; }

  server::~server(){}

  void server::listen( const fc::ip::endpoint& p ) 
  {
    my.reset( new impl(p) );
  }

  fc::ip::endpoint server::get_local_endpoint() const
  {
    return my->tcp_serv.get_local_endpoint();
  }


  server::response::response(){}
  server::response::response( const server::response& s ):my(s.my){}
  server::response::response( server::response&& s ):my(fc::move(s.my)){}
  server::response::response( const fc::shared_ptr<server::response::impl>& m ):my(m){}

  server::response& server::response::operator=(const server::response& s) { my = s.my; return *this; }
  server::response& server::response::operator=(server::response&& s)      { fc_swap(my,s.my); return *this; }

  void server::response::add_header( const fc::string& key, const fc::string& val )const {
     my->rep.headers.push_back( fc::http::header( key, val ) );
  }
  void server::response::set_status( const http::reply::status_code& s )const {
     if( my->body_bytes_sent != 0 ) {
       wlog( "Attempt to set status after sending headers" );
     }
     my->rep.status = s;
  }
  void server::response::set_length( uint64_t s )const {
    if( my->body_bytes_sent != 0 ) {
      wlog( "Attempt to set length after sending headers" );
    }
    my->body_length = s; 
  }
  void server::response::write( const char* data, uint64_t len )const {
    if( my->body_bytes_sent + len > my->body_length ) {
      wlog( "Attempt to send to many bytes.." );
      len = my->body_bytes_sent + len - my->body_length;
    }
    if( my->body_bytes_sent == 0 ) {
      my->send_header();
    }
    my->body_bytes_sent += len;
    my->con->get_socket().write( data, static_cast<size_t>(len) ); 
    if( my->body_bytes_sent == int64_t(my->body_length) ) {
      if( false || my->handle_next_req ) {
        ilog( "handle next request..." );
        //fc::async( std::function<void()>(my->handle_next_req) );
        fc::async( my->handle_next_req, "http_server handle_next_req" );
      }
    }
  }

  server::response::~response(){}

  void server::on_request( const std::function<void(const http::request&, const server::response& s )>& cb )
  { 
     my->on_req = cb; 
  }




} }
