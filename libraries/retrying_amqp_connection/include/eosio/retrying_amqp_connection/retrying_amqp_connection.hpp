#pragma once

#include <amqpcpp.h>

#include <boost/asio.hpp>

#include <fc/log/logger.hpp>
#include <fc/exception/exception.hpp>

/*
 * A retrying_amqp_connection manages a connection to an AMQP server that will retry the connection
 * on failure. Most users should consider single_channel_retrying_amqp_connection instead, which additionally
 * manages a single channel.
 */

namespace eosio {
template <typename Executor>
struct retrying_amqp_connection : public AMQP::ConnectionHandler {
   using connection_ready_callback_t = std::function<void(AMQP::Connection*)>;
   using connection_failed_callback_t = std::function<void()>;

   /// \param executor executor to use for all asio operations; use a strand when appropriate as one is not used internally
   /// \param address AMQP address to connect to
   /// \param ready a callback when the AMQP connection has been established
   /// \param failed a callback when the AMQP connection has failed after being established; should no longer use the AMQP::Connection* after this callback
   /// \param logger logger to send logging to
   retrying_amqp_connection(Executor& executor, const AMQP::Address& address, connection_ready_callback_t ready,
                                  connection_failed_callback_t failed, fc::logger logger = fc::logger::get()) :
     _executor(executor), _resolver(executor), _sock(executor), _timer(executor), _address(address),
     _ready_callback(ready), _failed_callback(failed), _logger(logger) {

      FC_ASSERT(!_address.secure(), "Only amqp:// URIs are supported for AMQP addresses (${a})", ("a", _address));
      start_connection();
   }

   const AMQP::Address& address() const {
      return _address;
   }

protected:
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

private:
  void start_connection() {
       _resolver.async_resolve(_address.hostname(), std::to_string(_address.port()), boost::asio::bind_executor(_executor, [this](const auto ec, const auto endpoints) {
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
         boost::asio::async_connect(_sock, endpoints, boost::asio::bind_executor(_executor, [this](const auto ec, const auto endpoint) {
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
      if(_indicated_ready)
         _failed_callback();
      _indicated_ready = false;
      _sock.close();
      _resolver.cancel();

      boost::system::error_code ec;
      _timer.expires_from_now(std::chrono::seconds(1), ec);
      if(ec)
         return;
      _timer.async_wait(boost::asio::bind_executor(_executor, [this](const auto ec) {
         if(ec)
            return;
         start_connection();
      }));
   }

   void send_some() {
      if(_state->send_outstanding || _state->outgoing_queue.empty())
         return;
      _state->send_outstanding = true;
      boost::asio::async_write(_sock, boost::asio::buffer(_state->outgoing_queue.front()), boost::asio::bind_executor(_executor, [this](const auto& ec, size_t wrote) {
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
      _sock.async_read_some(boost::asio::buffer(_read_buff), boost::asio::bind_executor(_executor, [this](const auto& ec, size_t sz) {
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

         //parse() could have resulted in an error on a channel. In an earlier implementation users could call a function
         // which may have caused a socket connection to .close() due to that. This check below may no longer be needed since
         // retrying_amqp_connection no longer exposes a way to "bump" a connection
         if(_sock.is_open())
            receive_some();
      }));
   }

   char _read_buff[64*1024];

   Executor& _executor;

   boost::asio::ip::tcp::resolver  _resolver;
   boost::asio::ip::tcp::socket    _sock;
   boost::asio::steady_timer       _timer;

   AMQP::Address _address;

   connection_ready_callback_t  _ready_callback;
   connection_failed_callback_t _failed_callback;
   bool _indicated_ready = false;

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

template <typename Executor>
struct single_channel_retrying_amqp_connection {
   using channel_ready_callback_t = std::function<void(AMQP::Channel*)>;
   using failed_callback_t = std::function<void()>;

   /// \param executor executor to use for all asio operations; use a strand when appropriate as one is not used internally
   /// \param address AMQP address to connect to
   /// \param ready a callback when the AMQP channel has been established
   /// \param failed a callback when the AMQP channel has failed after being established; should no longer use the AMQP::Channel* within or after this callback
   /// \param logger logger to send logging to
   single_channel_retrying_amqp_connection(Executor& executor, const AMQP::Address& address, channel_ready_callback_t ready,
                                                 failed_callback_t failed, fc::logger logger = fc::logger::get()) :
     _executor(executor),
     _connection(executor, address, [this](AMQP::Connection* c){conn_ready(c);},[this](){conn_failed();}, logger),
     _timer(executor), _channel_ready(ready), _failed(failed)
   {}

   const AMQP::Address& address() const {
      return _connection.address();
   }

private:
   void conn_ready(AMQP::Connection* c) {
      _amqp_connection = c;
      bring_up_channel();
   }

   void start_retry() {
      boost::system::error_code ec;
      _timer.expires_from_now(std::chrono::seconds(1), ec);
      if(ec)
         return;
      _timer.async_wait(boost::asio::bind_executor(_executor, [this](const auto ec) {
         if(ec)
            return;
         bring_up_channel();
      }));
   }

   void set_channel_on_error() {
      _amqp_channel->onError([this](const char* e) {
         wlog("AMQP channel failure on AMQP connection ${c}; retrying : ${m}", ("c", _connection.address())("m", e));
         _failed();
         start_retry();
      });
   }

   void bring_up_channel() {
      try {
         _amqp_channel.emplace(_amqp_connection);
      }
      catch(...) {
         wlog("AMQP channel could not start for AMQP connection ${c}; retrying", ("c", _connection.address()));
         start_retry();
      }
      set_channel_on_error();
      _amqp_channel->onReady([this]() {
         _channel_ready(&*_amqp_channel);
         //in case someone tried to set their own onError()...
         set_channel_on_error();
      });
   }

   void conn_failed() {
      _amqp_connection = nullptr;
      _amqp_channel.reset();
      boost::system::error_code ec;
      _timer.cancel(ec);
      _failed();
   }

   Executor& _executor;

   retrying_amqp_connection<Executor> _connection;
   boost::asio::steady_timer _timer;
   std::optional<AMQP::Channel> _amqp_channel;
   AMQP::Connection* _amqp_connection = nullptr;

   channel_ready_callback_t _channel_ready;
   failed_callback_t _failed;
};

}

namespace fc {
void to_variant(const AMQP::Address& a, fc::variant& v);
}
