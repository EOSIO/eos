#include <amqpcpp.h>

#include <eosio/amqp/util.hpp>
#include <eosio/amqp/retrying_amqp_connection.hpp>

namespace eosio {

struct retrying_amqp_connection::impl : public AMQP::ConnectionHandler {
   impl(boost::asio::io_context& io_context, const AMQP::Address& address, connection_ready_callback_t ready,
        connection_failed_callback_t failed, fc::logger logger = fc::logger::get()) :
     _strand(io_context), _resolver(_strand.context()), _sock(_strand.context()), _timer(_strand.context()), _address(address),
     _ready_callback(ready), _failed_callback(failed), _logger(logger) {

      FC_ASSERT(!_address.secure(), "Only amqp:// URIs are supported for AMQP addresses (${a})", ("a", _address));
      start_connection();
   }

   void onReady(AMQP::Connection* connection) override {
      fc_ilog(_logger, "AMQP connection to ${s} is fully operational", ("s", _address));

      _ready_callback(connection);
      _indicated_ready = true;
   }

   void onData(AMQP::Connection* connection, const char* data, size_t size) override {
      if(!_sock.is_open())
         return;
      _state->outgoing_queue.emplace_back(data, data+size);
      send_some();
   }

   void onError(AMQP::Connection* connection, const char* message) override {
      fc_elog(_logger, "AMQP connection to ${s} suffered an error; will retry shortly: ${m}", ("s", _address)("m", message));
      schedule_retry();
   }

   void onClosed(AMQP::Connection *connection) override {
      fc_wlog(_logger, "AMQP connection to ${s} closed AMQP connection", ("s", _address));
      schedule_retry();
   }

  void start_connection() {
       _resolver.async_resolve(_address.hostname(), std::to_string(_address.port()), boost::asio::bind_executor(_strand, [this](const auto ec, const auto endpoints) {
         if(ec) {
            if(ec != boost::asio::error::operation_aborted) {
               fc_wlog(_logger, "Failed resolving AMQP server ${s}; will retry shortly: ${m}", ("s", _address)("m", ec.message()));
               schedule_retry();
            }
            return;
         }
         //AMQP::Connection's dtor will attempt to send a last gasp message. Resetting state here is a little easier to prove
         // as being safe as it requires pumping the event loop once vs placing the state reset directly in schedule_retry()
         _state.emplace();
         boost::asio::async_connect(_sock, endpoints, boost::asio::bind_executor(_strand, [this](const auto ec, const auto endpoint) {
            if(ec) {
               if(ec != boost::asio::error::operation_aborted) {
                  fc_wlog(_logger, "Failed connecting AMQP server ${s}; will retry shortly: ${m}", ("s", _address)("m", ec.message()));
                  schedule_retry();
               }
               return;
            }
            fc_ilog(_logger, "TCP connection to AMQP server at ${s} is up", ("s", _address));
            receive_some();
            _state->amqp_connection.emplace(this, _address.login(), _address.vhost());
         }));
      }));
   }

   void schedule_retry() {
      //in some cases, such as an async_read & async_write both outstanding at the same time during socket failure,
      // schedule_retry() can be called multiple times in quick succession. nominally this causes an already closed _sock
      // to be closed(), and cancels the pending 1 second timer when restarting the 1 second timer. In theory though, if thread
      // timing is particularly slow, the one second timer may have already expired (and the callback can no longer be cancelled)
      // which could potentially queue up two start_connection()s.
      //Bail out early if a pending timer is already running and the callback hasn't been called.
      if(_retry_scheduled)
         return;

      _sock.close();
      _resolver.cancel();

      //calling the failure callback will likely cause downstream users to take action such as closing an AMQP::Channel which
      // will attempt to send data. Ensure that _sock is closed before then so onData() will drop those attempts
      if(_indicated_ready)
         _failed_callback();
      _indicated_ready = false;

      boost::system::error_code ec;
      _timer.expires_from_now(std::chrono::seconds(1), ec);
      if(ec)
         return;
      _retry_scheduled = true;
      _timer.async_wait(boost::asio::bind_executor(_strand, [this](const auto ec) {
         _retry_scheduled = false;
         if(ec)
            return;
         start_connection();
      }));
   }

   void send_some() {
      if(_state->send_outstanding || _state->outgoing_queue.empty())
         return;
      _state->send_outstanding = true;
      boost::asio::async_write(_sock, boost::asio::buffer(_state->outgoing_queue.front()), boost::asio::bind_executor(_strand, [this](const auto& ec, size_t wrote) {
         if(ec) {
            if(ec != boost::asio::error::operation_aborted) {
               fc_wlog(_logger, "Failed writing to AMQP server ${s}; connection will retry shortly: ${m}", ("s", _address)("m", ec.message()));
               schedule_retry();
            }
            return;
         }
         _state->outgoing_queue.pop_front();
         _state->send_outstanding = false;
         send_some();
      }));
   }

   void receive_some() {
      _sock.async_read_some(boost::asio::buffer(_read_buff), boost::asio::bind_executor(_strand, [this](const auto& ec, size_t sz) {
         if(ec) {
            if(ec != boost::asio::error::operation_aborted) {
               fc_wlog(_logger, "Failed reading from AMQP server ${s}; connection will retry shortly: ${m}", ("s", _address)("m", ec.message()));
               schedule_retry();
            }
            return;
         }
         _state->read_queue.insert(_state->read_queue.end(), _read_buff, _read_buff + sz);
         auto used = _state->amqp_connection->parse(_state->read_queue.data(), _state->read_queue.size());
         _state->read_queue.erase(_state->read_queue.begin(), _state->read_queue.begin()+used);

         //parse() could have resulted in an error on an AMQP channel or on the AMQP connection (causing a onError() or
         // onClosed() to be called). An error on an AMQP channel is outside the scope of retrying_amqp_connection, but an
         // onError() or onClosed() would call schedule_retry() and thus _sock.close(). Check that the socket is still open before
         // looping back around for another async_read
         if(_sock.is_open())
            receive_some();
      }));
   }

   char _read_buff[64*1024];

   boost::asio::io_context::strand _strand;

   boost::asio::ip::tcp::resolver  _resolver;
   boost::asio::ip::tcp::socket    _sock;
   boost::asio::steady_timer       _timer;

   AMQP::Address _address;

   connection_ready_callback_t  _ready_callback;
   connection_failed_callback_t _failed_callback;
   bool _indicated_ready = false;
   bool _retry_scheduled = false;

   fc::logger _logger;

   struct state {
      state() {}

      std::deque<std::vector<char>> outgoing_queue;
      bool send_outstanding = false;

      std::vector<char> read_queue;

      std::optional<AMQP::Connection> amqp_connection;
   };
   std::optional<state> _state;
   //be aware that AMQP::Connection's dtor will attempt to send a last gasp message on dtor. This means _state's
   // destruction will cause onData() to be called when _state's amqp_connection dtor is fired. So be mindful of member
   // dtor ordering here as _state & _sock will be accessed during dtor
};


struct single_channel_retrying_amqp_connection::impl {
   using channel_ready_callback_t = single_channel_retrying_amqp_connection::channel_ready_callback_t;
   using failed_callback_t = single_channel_retrying_amqp_connection::failed_callback_t;

   impl(boost::asio::io_context& io_context, const AMQP::Address& address, channel_ready_callback_t ready,
                                                 failed_callback_t failed, fc::logger logger) :
     _connection(io_context, address, [this](AMQP::Connection* c){conn_ready(c);},[this](){conn_failed();}, logger),
     _timer(_connection.strand().context()), _channel_ready(ready), _failed(failed)
   {}

   void conn_ready(AMQP::Connection* c) {
      _amqp_connection = c;
      bring_up_channel();
   }

   void start_retry() {
      boost::system::error_code ec;
      _timer.expires_from_now(std::chrono::seconds(1), ec);
      if(ec)
         return;
      _timer.async_wait(boost::asio::bind_executor(_connection.strand(), [this](const auto ec) {
         if(ec)
            return;
         bring_up_channel();
      }));
   }

   void bring_up_channel() {
      try {
         _amqp_channel.emplace(_amqp_connection);
      }
      catch(...) {
         wlog("AMQP channel could not start for AMQP connection ${c}; retrying", ("c", _connection.address()));
         start_retry();
      }
      _amqp_channel->onError([this](const char* e) {
         wlog("AMQP channel failure on AMQP connection ${c}; retrying : ${m}", ("c", _connection.address())("m", e));
         _failed();
         start_retry();
      });
      _amqp_channel->onReady([this]() {
         _channel_ready(&*_amqp_channel);
      });
   }

   void conn_failed() {
      _amqp_connection = nullptr;
      _amqp_channel.reset();
      boost::system::error_code ec;
      _timer.cancel(ec);
      _failed();
   }

   retrying_amqp_connection _connection;
   boost::asio::steady_timer _timer;
   std::optional<AMQP::Channel> _amqp_channel;
   AMQP::Connection* _amqp_connection = nullptr;

   channel_ready_callback_t _channel_ready;
   failed_callback_t _failed;
};

retrying_amqp_connection::retrying_amqp_connection(boost::asio::io_context& io_context, const AMQP::Address& address, connection_ready_callback_t ready,
                                  connection_failed_callback_t failed, fc::logger logger) :
                                  my(new impl(io_context, address, ready, failed, logger)) {}


const AMQP::Address& retrying_amqp_connection::address() const {
   return my->_address;
}

boost::asio::io_context::strand& retrying_amqp_connection::strand() {
   return my->_strand;
}

retrying_amqp_connection::~retrying_amqp_connection() = default;

single_channel_retrying_amqp_connection::single_channel_retrying_amqp_connection(boost::asio::io_context& io_context, const AMQP::Address& address, channel_ready_callback_t ready,
                                                                                 failed_callback_t failed, fc::logger logger) :
  my(new impl(io_context, address, ready, failed, logger)) {}

const AMQP::Address& single_channel_retrying_amqp_connection::address() const {
   return my->_connection.address();
}

single_channel_retrying_amqp_connection::~single_channel_retrying_amqp_connection() = default;

}
