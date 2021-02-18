// TODO: move to a library

#pragma once

#include <eosio/ship_protocol.hpp>

#include <abieos.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <fc/exception/exception.hpp>

namespace b1::ship_client {

namespace ship = eosio::ship_protocol;

enum request_flags {
   request_irreversible_only = 1,
   request_block             = 2,
   request_traces            = 4,
   request_deltas            = 8,
};

struct connection_callbacks {
   virtual ~connection_callbacks() = default;
   virtual void received_abi() {}
   virtual bool received(ship::get_status_result_v0& status, eosio::input_stream bin) { return true; }
   virtual bool received(ship::get_blocks_result_v0& result, eosio::input_stream bin) { return true; }
   virtual bool received(ship::get_blocks_result_v1& result, eosio::input_stream bin) { return true; }
   virtual void closed(bool retry) = 0;
};

struct tcp_connection_config {
   std::string host;
   std::string port;
};

struct unix_connection_config {
   std::string path;
};

struct connection_config {
   std::variant<tcp_connection_config, unix_connection_config> connection_config;
};

struct abi_def_skip_table : eosio::abi_def {};

EOSIO_REFLECT(abi_def_skip_table, version, types, structs, actions, ricardian_clauses, error_messages, abi_extensions,
              variants);

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

struct connection_base {
   virtual void connect() = 0;
   virtual void send(const ship::request& req) = 0;
   virtual void request_blocks(const ship::get_status_result_v0& status, uint32_t start_block_num, const std::vector<ship::block_position>& positions, int flags) = 0;
   virtual void close(bool retry) = 0;

   virtual ~connection_base() = default;
};

template <typename ConnectionType>
struct connection : connection_base {
   using error_code  = boost::system::error_code;
   using flat_buffer = boost::beast::flat_buffer;
   using tcp         = boost::asio::ip::tcp;
   using unixs       = boost::asio::local::stream_protocol;
   using abi_type    = eosio::abi_type;

   std::shared_ptr<connection_callbacks>        callbacks;
   bool                                         have_abi  = false;
   abi_def_skip_table                           abi       = {};
   std::map<std::string, abi_type>              abi_types = {};

   connection(std::shared_ptr<connection_callbacks> callbacks)
       : callbacks(callbacks) {}

   ConnectionType& derived_connection() {
      return static_cast<ConnectionType&>(*this);
   }

   void ws_handshake(const std::string& host) {
      derived_connection().stream.binary(true);
      derived_connection().stream.read_message_max(10ull * 1024 * 1024 * 1024);

      derived_connection().stream.async_handshake( //
         host, "/", [self = derived_connection().shared_from_this(), this](error_code ec) {
            enter_callback(ec, "handshake", [&] { //
               start_read();
            });
         });
   }

   void start_read() {
      auto in_buffer = std::make_shared<flat_buffer>();
      derived_connection().stream.async_read(*in_buffer, [self = derived_connection().shared_from_this(), this, in_buffer](error_code ec, size_t) {
         enter_callback(ec, "async_read", [&] {
            if (!have_abi)
               receive_abi(in_buffer);
            else {
               if (!receive_result(in_buffer)) {
                  close(false);
                  return;
               }
            }
            start_read();
         });
      });
   }

   void receive_abi(const std::shared_ptr<flat_buffer>& p) {
      auto                     data = p->data();
      std::string              json{ (const char*)data.data(), data.size() };
      eosio::json_token_stream stream{ json.data() };
      from_json(abi, stream);
      std::string error;
      if (!abieos::check_abi_version(abi.version, error))
         throw std::runtime_error(error);
      eosio::abi a;
      convert(abi, a);
      abi_types = std::move(a.abi_types);
      have_abi  = true;
      if (callbacks)
         callbacks->received_abi();
   }

   bool receive_result(const std::shared_ptr<flat_buffer>& p) {
      auto                data = p->data();
      eosio::input_stream bin{ (const char*)data.data(), (const char*)data.data() + data.size() };
      auto                orig = bin;
      ship::result        result;
      from_bin(result, bin);
      return callbacks && std::visit([&](auto& r) { return callbacks->received(r, orig); }, result);
   }

   void request_blocks(uint32_t start_block_num, const std::vector<ship::block_position>& positions, int flags) {
      ship::get_blocks_request_v0 req;
      req.start_block_num        = start_block_num;
      req.end_block_num          = 0xffff'ffff;
      req.max_messages_in_flight = 0xffff'ffff;
      req.have_positions         = positions;
      req.irreversible_only      = flags & request_irreversible_only;
      req.fetch_block            = flags & request_block;
      req.fetch_traces           = flags & request_traces;
      req.fetch_deltas           = flags & request_deltas;
      send(req);
   }

   void request_blocks(const ship::get_status_result_v0& status, uint32_t start_block_num,
                       const std::vector<ship::block_position>& positions, int flags) {
      uint32_t nodeos_start = 0xffff'ffff;
      if (status.trace_begin_block < status.trace_end_block)
         nodeos_start = std::min(nodeos_start, status.trace_begin_block);
      if (status.chain_state_begin_block < status.chain_state_end_block)
         nodeos_start = std::min(nodeos_start, status.chain_state_begin_block);
      if (nodeos_start == 0xffff'ffff)
         nodeos_start = 0;
      request_blocks(std::max(start_block_num, nodeos_start), positions, flags);
   }

   const abi_type& get_type(const std::string& name) {
      auto it = abi_types.find(name);
      if (it == abi_types.end())
         throw std::runtime_error(std::string("unknown type ") + name);
      return it->second;
   }

   void send(const ship::request& req) {
      auto bin = std::make_shared<std::vector<char>>();
      eosio::convert_to_bin(req, *bin);
      derived_connection().stream.async_write(boost::asio::buffer(*bin), [self = derived_connection().shared_from_this(), bin, this](error_code ec, size_t) {
         enter_callback(ec, "async_write", [&] {});
      });
   }

   template <typename F>
   void catch_and_close(F f) {
      try {
         f();
      } catch (const std::exception& e) {
         elog("${e}", ("e", e.what()));
         close(false);
      } catch (...) {
         elog("unknown exception");
         close(false);
      }
   }

   template <typename F>
   void enter_callback(error_code ec, const char* what, F f) {
      if (ec)
         return on_fail(ec, what);
      catch_and_close(f);
   }

   void on_fail(error_code ec, const char* what) {
      try {
         elog("${w}: ${m}", ("w", what)("m", ec.message()));
         close(true);
      } catch (...) { elog("exception while closing"); }
   }

   void close(bool retry) {
      ilog("closing state-history socket");
      derived_connection().stream.next_layer().close();
      if (callbacks)
         callbacks->closed(retry);
      callbacks.reset();
   }
}; // connection

struct tcp_connection : connection<tcp_connection>, std::enable_shared_from_this<tcp_connection> {
   tcp_connection(boost::asio::io_context& ioc, const tcp_connection_config& config, std::shared_ptr<connection_callbacks> callbacks) :
     connection<tcp_connection>(callbacks), config(config), resolver(ioc), stream(ioc) {}

   void connect() {
      ilog("connect to ${h}:${p}", ("h", config.host)("p", config.port));
      resolver.async_resolve( //
         config.host, config.port,
         [self = shared_from_this(), this](error_code ec, tcp::resolver::results_type results) {
            enter_callback(ec, "resolve", [&] {
               boost::asio::async_connect( //
                  stream.next_layer(), results.begin(), results.end(),
                  [self = shared_from_this(), this](error_code ec, auto&) {
                     enter_callback(ec, "connect", [&] {
                        ws_handshake(config.host);
                     });
                  });
            });
         });
   }

   tcp_connection_config                        config;
   tcp::resolver                                resolver;
   boost::beast::websocket::stream<tcp::socket> stream;
};

struct unix_connection : connection<unix_connection>, std::enable_shared_from_this<unix_connection> {
   unix_connection(boost::asio::io_context& ioc, const unix_connection_config& config, std::shared_ptr<connection_callbacks> callbacks) :
     connection<unix_connection>(callbacks), config(config), stream(ioc) {}

   void connect() {
      ilog("connect to unix path ${p}", ("p", config.path));
      stream.next_layer().async_connect(config.path, [self = shared_from_this(), this](error_code ec) {
         enter_callback(ec, "connect", [&] {
            ws_handshake("");
         });
      });
   }

   unix_connection_config                         config;
   boost::beast::websocket::stream<unixs::socket> stream;
};

std::shared_ptr<connection_base> make_connection(boost::asio::io_context& ioc, const connection_config& config,
                                                 std::shared_ptr<connection_callbacks> callbacks) {
   return std::visit(overloaded {
      [&](const tcp_connection_config& c) -> std::shared_ptr<connection_base> {
         return std::make_shared<tcp_connection>(ioc, c, callbacks);
      },
      [&](const unix_connection_config& c) -> std::shared_ptr<connection_base> {
         return std::make_shared<unix_connection>(ioc, c, callbacks);
      }
   }, config.connection_config);
}

} // namespace b1::ship_client
