#include <eos/connection_plugin/connection_plugin.hpp>
#include <eos/network_plugin/connection_interface.hpp>

#include <deque>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

namespace eos {
   class connection : public connection_interface {
   public:
      connection(boost::asio::ip::tcp::socket s) :
        socket(std::move(s)),
        strand(socket.get_io_service())
          {
         ilog("connection created");

         read();
      }

      ~connection() {
        ilog("connection destroyed");
      }


      bool disconnected() override {
        return socket.is_open();
      }
      boost::signals2::connection connect_on_disconnected(const boost::signals2::signal<void()>::slot_type& slot) override {
        return on_disconnected.connect(slot);
      }
  private:
     void read() {
        socket.async_read_some(boost::asio::buffer(byte_buffer), strand.wrap(boost::bind(&connection::read_ready, this, _1, _2)));
     }
      void read_ready(boost::system::error_code ec, size_t red) {
         ilog("read ${r} bytes!", ("r",red));
         if(ec == boost::asio::error::eof) {
             socket.close();
             on_disconnected();
             return;
         }

         queuedOutgoing.emplace_back(byte_buffer, byte_buffer+red);

         socket.async_send(boost::asio::buffer(queuedOutgoing.back().data(), queuedOutgoing.back().size()),
                                                  strand.wrap(boost::bind(&connection::send_complete, this, _1, _2)));

         read();
      }

      void send_complete(boost::system::error_code ec, size_t sent) {
         //sent should always == sent requested unless error, right?
         queuedOutgoing.pop_front();
      }

      boost::asio::ip::tcp::socket socket;
      boost::asio::io_service::strand strand;

      uint8_t byte_buffer[256];
      std::deque<std::vector<uint8_t>> queuedOutgoing;

      boost::signals2::signal<void()> on_disconnected;
   };

   class connection_plugin_impl {
      struct active_acceptor {
        boost::asio::ip::tcp::acceptor acceptor;
        boost::asio::ip::tcp::socket   socket_to_accept;
      };
      struct outgoing_attempt : private boost::noncopyable {
        outgoing_attempt(boost::asio::io_service& ios, boost::asio::ip::tcp::endpoint ep) : 
        remote_endpoint(ep), reconnect_timer(ios), outgoing_socket(ios) {}

        boost::asio::ip::tcp::endpoint  remote_endpoint;
        boost::asio::steady_timer       reconnect_timer;
        boost::asio::ip::tcp::socket    outgoing_socket;      //only used during conn. estab., then moved to connection object
      };
      struct active_connection : private boost::noncopyable {
        active_connection(std::shared_ptr<connection> s) :connection(s) {}

        std::shared_ptr<connection>  connection;
        boost::signals2::connection  connection_failure_connection;
      };  
   public:
      connection_plugin_impl() :
         ios(appbase::app().get_io_service()),
         strand(ios)
      {
         acceptors.emplace_front(active_acceptor{
                                {ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 5556)},
                                boost::asio::ip::tcp::socket(ios)});
         acceptors.front().acceptor.listen();
         accept_connection(acceptors.front());

         //for each IP/port to connect to...
         outgoing_connections.emplace_front(ios, boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 6666));
         retry_connection(outgoing_connections.front());

                                    
       }
   private:
      void accept_connection(active_acceptor& aa) {
        aa.acceptor.async_accept(aa.socket_to_accept, strand.wrap(boost::bind(&connection_plugin_impl::handle_incoming_connection, this, _1, boost::ref(aa))));
      }

      void handle_incoming_connection(const boost::system::error_code& ec, active_acceptor& aa) {
         ilog("accepted connection");
        
         //XX verify on some sort of acceptable incoming IP range
         active_connections.emplace_back(std::make_shared<connection>((std::move(aa.socket_to_accept))));
        
         accept_connection(aa);
      }

      void retry_connection(outgoing_attempt& outgoing) {
          outgoing.outgoing_socket = boost::asio::ip::tcp::socket(outgoing.reconnect_timer.get_io_service());
          outgoing.outgoing_socket.async_connect(outgoing.remote_endpoint, strand.wrap(boost::bind(&connection_plugin_impl::outgoing_connection_complete, this, _1, boost::ref(outgoing))));
      }

      void outgoing_connection_complete(const boost::system::error_code& ec, outgoing_attempt& outgoing) {
        if(ec) {
          ilog("connection failed");
          outgoing.reconnect_timer.expires_from_now(std::chrono::seconds(3));
          outgoing.reconnect_timer.async_wait(boost::bind(&connection_plugin_impl::retry_connection, this, boost::ref(outgoing)));
        }
          else {
            ilog("Connection good!");
            outgoing.reconnect_timer.cancel();
            active_connections.emplace_front(std::make_shared<connection>(std::move(outgoing.outgoing_socket)));
            active_connections.front().connection_failure_connection = active_connections.front().connection->connect_on_disconnected(strand.wrap([this,it=active_connections.begin(),&outgoing]() {
                it->connection_failure_connection.disconnect();
                active_connections.erase(it);
                retry_connection(outgoing);
            }));
          }
      }

      std::list<active_acceptor> acceptors;
      std::list<outgoing_attempt> outgoing_connections;

      boost::asio::io_service& ios;

      boost::asio::strand strand;
      std::list<active_connection> active_connections;
   };

   connection_plugin::connection_plugin() : pimpl(new connection_plugin_impl) {}

   connection_plugin::~connection_plugin() {
   }

   void connection_plugin::set_program_options( options_description& cli, options_description& cfg )
   {
     cfg.add_options()
       ( "listen-endpoint", bpo::value<vector<string>>()->composing(), "Local IP address and port tuples to bind to for incoming connections.")
       ( "remote-endpoint", bpo::value<vector<string>>()->composing(), "IP address and port tuples to connect to remote peers.")
       ;
   }

   void connection_plugin::plugin_initialize( const variables_map& options ) {
      ilog("Initialize network plugin");
   }
  
   void connection_plugin::plugin_startup() {
     ilog("startup");

   }

  void connection_plugin::plugin_shutdown() {
  }
}
