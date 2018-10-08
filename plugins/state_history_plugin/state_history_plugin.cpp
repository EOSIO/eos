/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/state_history_plugin/history_log.hpp>
#include <eosio/state_history_plugin/state_history_plugin.hpp>
#include <eosio/state_history_plugin/state_history_serialization.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/signals2/connection.hpp>

using tcp    = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;

extern const char* const state_history_plugin_abi;

namespace eosio {
using namespace chain;
using boost::signals2::scoped_connection;

static appbase::abstract_plugin& _state_history_plugin = app().register_plugin<state_history_plugin>();

template <typename F>
static void for_each_table(const chainbase::database& db, F f) {
   f("account", db.get_index<account_index>(), [](auto&) { return true; });
   f("account_sequence", db.get_index<account_sequence_index>(), [](auto&) { return true; });

   f("table_id", db.get_index<table_id_multi_index>(), [](auto&) { return true; });
   f("key_value", db.get_index<key_value_index>(), [](auto&) { return true; });
   f("index64", db.get_index<index64_index>(), [](auto&) { return true; });
   f("index128", db.get_index<index128_index>(), [](auto&) { return true; });
   f("index256", db.get_index<index256_index>(), [](auto&) { return true; });
   f("index_double", db.get_index<index_double_index>(), [](auto&) { return true; });
   f("index_long_double", db.get_index<index_long_double_index>(), [](auto&) { return true; });

   f("global_property", db.get_index<global_property_multi_index>(), [](auto&) { return true; });
   f("dynamic_global_property", db.get_index<dynamic_global_property_multi_index>(), [](auto&) { return true; });
   f("block_summary", db.get_index<block_summary_multi_index>(),
     [](auto& row) { return row.block_id != block_id_type(); });
   f("transaction", db.get_index<transaction_multi_index>(), [](auto&) { return true; });
   f("generated_transaction", db.get_index<generated_transaction_multi_index>(), [](auto&) { return true; });

   f("permission", db.get_index<permission_index>(), [](auto&) { return true; });
   f("permission_usage", db.get_index<permission_usage_index>(), [](auto&) { return true; });
   f("permission_link", db.get_index<permission_link_index>(), [](auto&) { return true; });

   f("resource_limits", db.get_index<resource_limits::resource_limits_index>(), [](auto&) { return true; });
   f("resource_usage", db.get_index<resource_limits::resource_usage_index>(), [](auto&) { return true; });
   f("resource_limits_state", db.get_index<resource_limits::resource_limits_state_index>(), [](auto&) { return true; });
   f("resource_limits_config", db.get_index<resource_limits::resource_limits_config_index>(),
     [](auto&) { return true; });
}

template <typename F>
auto catch_and_log(F f) {
   try {
      return f();
   } catch (const fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
   } catch (const std::exception& e) {
      elog("${e}", ("e", e.what()));
   } catch (...) {
      elog("unknown exception");
   }
}

struct state_history_plugin_impl : std::enable_shared_from_this<state_history_plugin_impl> {
   chain_plugin*                   chain_plug = nullptr;
   history_log                     state_log{"state_history"};
   bool                            stopping = false;
   fc::optional<scoped_connection> accepted_block_connection;
   fc::optional<scoped_connection> irreversible_block_connection;
   string                          endpoint_address = "0.0.0.0";
   uint16_t                        endpoint_port    = 4321;
   std::unique_ptr<tcp::acceptor>  acceptor;

   void get_state(uint32_t block_num, bytes& deltas) {
      history_log_header header;
      auto&              stream = state_log.get_entry(block_num, header);
      uint32_t           s;
      stream.read((char*)&s, sizeof(s));
      deltas.resize(s);
      if (s)
         stream.read(deltas.data(), s);
   }

   struct session : std::enable_shared_from_this<session> {
      std::shared_ptr<state_history_plugin_impl> plugin;
      std::unique_ptr<ws::stream<tcp::socket>>   stream;
      bool                                       sending = false;
      std::vector<std::vector<char>>             send_queue;

      session(std::shared_ptr<state_history_plugin_impl> plugin)
          : plugin(std::move(plugin)) {}

      void start(tcp::socket socket) {
         ilog("incoming connection");
         stream = std::make_unique<ws::stream<tcp::socket>>(std::move(socket));
         stream->binary(true);
         stream->next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
         stream->next_layer().set_option(boost::asio::socket_base::send_buffer_size(1024 * 1024));
         stream->next_layer().set_option(boost::asio::socket_base::receive_buffer_size(1024 * 1024));
         stream->async_accept([self = shared_from_this(), this](boost::system::error_code ec) {
            if (plugin->stopping)
               return;
            callback(ec, "async_accept", [&] {
               start_read();
               send(state_history_plugin_abi);
            });
         });
      }

      void start_read() {
         auto in_buffer = std::make_shared<boost::beast::flat_buffer>();
         stream->async_read(
             *in_buffer, [self = shared_from_this(), this, in_buffer](boost::system::error_code ec, size_t) {
                if (plugin->stopping)
                   return;
                callback(ec, "async_read", [&] {
                   auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(in_buffer->data()));
                   auto s = boost::asio::buffer_size(in_buffer->data());
                   fc::datastream<const char*> ds(d, s);
                   state_request               req;
                   fc::raw::unpack(ds, req);
                   req.visit(*this);
                   start_read();
                });
             });
      }

      void send(const char* s) {
         // todo: send as string
         send_queue.push_back({s, s + strlen(s)});
         send();
      }

      template <typename T>
      void send(T obj) {
         send_queue.push_back(fc::raw::pack(state_result{std::move(obj)}));
         send();
      }

      void send() {
         if (sending || send_queue.empty())
            return;
         sending = true;
         stream->async_write( //
             boost::asio::buffer(send_queue[0]),
             [self = shared_from_this(), this](boost::system::error_code ec, size_t) {
                if (plugin->stopping)
                   return;
                callback(ec, "async_write", [&] {
                   send_queue.erase(send_queue.begin());
                   sending = false;
                   send();
                });
             });
      }

      using result_type = void;
      void operator()(get_status_request_v0&) {
         get_status_result_v0 result;
         result.state_begin_block_num = plugin->state_log.begin_block();
         result.state_end_block_num   = plugin->state_log.end_block();
         send(std::move(result));
      }

      void operator()(get_block_request_v0& req) {
         // ilog("${b} get_block_request_v0", ("b", req.block_num));
         get_block_result_v0 result{req.block_num};
         if (req.block_num >= plugin->state_log.begin_block() && req.block_num < plugin->state_log.end_block()) {
            result.deltas.emplace();
            plugin->get_state(req.block_num, *result.deltas);
         }
         send(std::move(result));
      }

      template <typename F>
      void catch_and_close(F f) {
         try {
            f();
         } catch (const fc::exception& e) {
            elog("${e}", ("e", e.to_detail_string()));
            close();
         } catch (const std::exception& e) {
            elog("${e}", ("e", e.what()));
            close();
         } catch (...) {
            elog("unknown exception");
            close();
         }
      }

      template <typename F>
      void callback(boost::system::error_code ec, const char* what, F f) {
         if (ec)
            return on_fail(ec, what);
         catch_and_close(f);
      }

      void on_fail(boost::system::error_code ec, const char* what) {
         try {
            elog("${w}: ${m}", ("w", what)("m", ec.message()));
            close();
         } catch (...) {
            elog("uncaught exception on close");
         }
      }

      void close() {
         stream->next_layer().close();
         plugin->sessions.erase(this);
      }
   };
   std::map<session*, std::shared_ptr<session>> sessions;

   void listen() {
      boost::system::error_code ec;
      auto                      address  = boost::asio::ip::make_address(endpoint_address);
      auto                      endpoint = tcp::endpoint{address, endpoint_port};
      acceptor                           = std::make_unique<tcp::acceptor>(app().get_io_service());

      auto check_ec = [&](const char* what) {
         if (!ec)
            return;
         elog("${w}: ${m}", ("w", what)("m", ec.message()));
         EOS_ASSERT(false, plugin_exception, "unable top open listen socket");
      };

      acceptor->open(endpoint.protocol(), ec);
      check_ec("open");
      acceptor->set_option(boost::asio::socket_base::reuse_address(true));
      acceptor->bind(endpoint, ec);
      check_ec("bind");
      acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      check_ec("listen");
      do_accept();
   }

   void do_accept() {
      auto socket = std::make_shared<tcp::socket>(app().get_io_service());
      acceptor->async_accept(*socket, [self = shared_from_this(), socket, this](auto ec) {
         if (stopping)
            return;
         if (ec) {
            if (ec == boost::system::errc::too_many_files_open)
               catch_and_log([&] { do_accept(); });
            return;
         }
         catch_and_log([&] {
            auto s            = std::make_shared<session>(self);
            sessions[s.get()] = s;
            s->start(std::move(*socket));
         });
         do_accept();
      });
   }

   void on_accepted_block(const block_state_ptr& block_state) {
      bool fresh = state_log.begin_block() == state_log.end_block();
      if (fresh)
         ilog("Placing initial state in block ${n}", ("n", block_state->block->block_num()));

      std::vector<table_delta> deltas;
      for_each_table(chain_plug->chain().db(), [&, this](auto* name, auto& index, auto filter) {
         if (fresh) {
            if (index.indices().empty())
               return;
            deltas.push_back({});
            auto& delta = deltas.back();
            delta.name  = name;
            for (auto& row : index.indices())
               if (filter(row))
                  delta.rows.emplace_back(row.id._id, fc::raw::pack(make_history_serial_wrapper(row)));
         } else {
            if (index.stack().empty())
               return;
            auto& undo = index.stack().back();
            if (undo.old_values.empty() && undo.new_ids.empty() && undo.removed_values.empty())
               return;
            deltas.push_back({});
            auto& delta = deltas.back();
            delta.name  = name;
            for (auto& old : undo.old_values) {
               auto& row = index.get(old.first);
               if (filter(row))
                  delta.rows.emplace_back(old.first._id, fc::raw::pack(make_history_serial_wrapper(row)));
            }
            for (auto id : undo.new_ids) {
               auto& row = index.get(id);
               if (filter(row))
                  delta.rows.emplace_back(id._id, fc::raw::pack(make_history_serial_wrapper(row)));
            }
            for (auto& old : undo.removed_values)
               if (filter(old.second))
                  delta.removed.push_back(old.first._id);
         }
      });
      auto deltas_bin = fc::raw::pack(deltas);
      EOS_ASSERT(deltas_bin.size() == (uint32_t)deltas_bin.size(), plugin_exception, "deltas is too big");
      history_log_header header{.block_num    = block_state->block->block_num(),
                                .payload_size = sizeof(uint32_t) + deltas_bin.size()};
      state_log.write_entry(header, [&](auto& stream) {
         uint32_t s = (uint32_t)deltas_bin.size();
         stream.write((char*)&s, sizeof(s));
         if (!deltas_bin.empty())
            stream.write(deltas_bin.data(), deltas_bin.size());
      });
   } // on_accepted_block

   void on_irreversible_block(const block_state_ptr& block_state) {}
}; // state_history_plugin_impl

state_history_plugin::state_history_plugin()
    : my(std::make_shared<state_history_plugin_impl>()) {}

state_history_plugin::~state_history_plugin() {}

void state_history_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto options = cfg.add_options();
   options("state-history-dir", bpo::value<bfs::path>()->default_value("state-history"),
           "the location of the state-history directory (absolute path or relative to application data dir)");
   options("delete-state-history", bpo::bool_switch()->default_value(false), "clear state history database");
   options("state-history-endpoint", bpo::value<string>()->default_value("0.0.0.0:8080"),
           "the endpoint upon which to listen for incoming connections");
}

void state_history_plugin::plugin_initialize(const variables_map& options) {
   try {
      // todo: check for --disable-replay-opts

      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT(my->chain_plug, chain::missing_chain_plugin_exception, "");
      auto& chain = my->chain_plug->chain();
      my->accepted_block_connection.emplace(
          chain.accepted_block.connect([&](const block_state_ptr& p) { my->on_accepted_block(p); }));
      my->irreversible_block_connection.emplace(
          chain.irreversible_block.connect([&](const block_state_ptr& p) { my->on_irreversible_block(p); }));

      auto                    dir_option = options.at("state-history-dir").as<bfs::path>();
      boost::filesystem::path state_history_dir;
      if (dir_option.is_relative())
         state_history_dir = app().data_dir() / dir_option;
      else
         state_history_dir = dir_option;

      auto ip_port         = options.at("state-history-endpoint").as<string>();
      auto port            = ip_port.substr(ip_port.find(':') + 1, ip_port.size());
      auto host            = ip_port.substr(0, ip_port.find(':'));
      my->endpoint_address = host;
      my->endpoint_port    = std::stoi(port);
      idump((ip_port)(host)(port));

      if (options.at("delete-state-history").as<bool>()) {
         ilog("Deleting state history");
         boost::filesystem::remove_all(state_history_dir);
      }

      boost::filesystem::create_directories(state_history_dir);
      my->state_log.open((state_history_dir / "state_history.log").string(),
                         (state_history_dir / "state_history.index").string());
   }
   FC_LOG_AND_RETHROW()
} // state_history_plugin::plugin_initialize

void state_history_plugin::plugin_startup() { my->listen(); }

void state_history_plugin::plugin_shutdown() {
   my->accepted_block_connection.reset();
   my->irreversible_block_connection.reset();
   while (!my->sessions.empty())
      my->sessions.begin()->second->close();
   my->stopping = true;
}

} // namespace eosio
