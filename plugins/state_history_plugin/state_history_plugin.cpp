#include <eosio/chain/config.hpp>
#include <eosio/state_history_plugin/state_history_log.hpp>
#include <eosio/state_history_plugin/state_history_serialization.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/signals2/connection.hpp>

#include "cppkin.h"

using tcp    = boost::asio::ip::tcp;
namespace ws = boost::beast::websocket;

extern const char* const state_history_plugin_abi;

namespace eosio {
using namespace chain;
using boost::signals2::scoped_connection;

static appbase::abstract_plugin& _state_history_plugin = app().register_plugin<state_history_plugin>();

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

namespace bio = boost::iostreams;
static bytes zlib_compress_bytes(bytes in) {
   bytes                  out;
   bio::filtering_ostream comp;
   comp.push(bio::zlib_compressor(bio::zlib::default_compression));
   comp.push(bio::back_inserter(out));
   bio::write(comp, in.data(), in.size());
   bio::close(comp);
   return out;
}

static bytes zlib_decompress(const bytes& in) {
   bytes                  out;
   bio::filtering_ostream decomp;
   decomp.push(bio::zlib_decompressor());
   decomp.push(bio::back_inserter(out));
   bio::write(decomp, in.data(), in.size());
   bio::close(decomp);
   return out;
}

template <typename T>
bool include_delta(const T& old, const T& curr) {
   return true;
}

bool include_delta(const eosio::chain::table_id_object& old, const eosio::chain::table_id_object& curr) {
   return old.payer != curr.payer;
}

bool include_delta(const eosio::chain::resource_limits::resource_limits_object& old,
                   const eosio::chain::resource_limits::resource_limits_object& curr) {
   return                                   //
       old.net_weight != curr.net_weight || //
       old.cpu_weight != curr.cpu_weight || //
       old.ram_bytes != curr.ram_bytes;
}

bool include_delta(const eosio::chain::resource_limits::resource_limits_state_object& old,
                   const eosio::chain::resource_limits::resource_limits_state_object& curr) {
   return                                                                                       //
       old.average_block_net_usage.last_ordinal != curr.average_block_net_usage.last_ordinal || //
       old.average_block_net_usage.value_ex != curr.average_block_net_usage.value_ex ||         //
       old.average_block_net_usage.consumed != curr.average_block_net_usage.consumed ||         //
       old.average_block_cpu_usage.last_ordinal != curr.average_block_cpu_usage.last_ordinal || //
       old.average_block_cpu_usage.value_ex != curr.average_block_cpu_usage.value_ex ||         //
       old.average_block_cpu_usage.consumed != curr.average_block_cpu_usage.consumed ||         //
       old.total_net_weight != curr.total_net_weight ||                                         //
       old.total_cpu_weight != curr.total_cpu_weight ||                                         //
       old.total_ram_bytes != curr.total_ram_bytes ||                                           //
       old.virtual_net_limit != curr.virtual_net_limit ||                                       //
       old.virtual_cpu_limit != curr.virtual_cpu_limit;
}

bool include_delta(const eosio::chain::account_metadata_object& old,
                   const eosio::chain::account_metadata_object& curr) {
   return                                               //
       old.name != curr.name ||                         //
       old.is_privileged() != curr.is_privileged() ||   //
       old.last_code_update != curr.last_code_update || //
       old.vm_type != curr.vm_type ||                   //
       old.vm_version != curr.vm_version ||             //
       old.code_hash != curr.code_hash;
}

bool include_delta(const eosio::chain::code_object& old, const eosio::chain::code_object& curr) { //
   return false;
}

bool include_delta(const eosio::chain::protocol_state_object& old, const eosio::chain::protocol_state_object& curr) {
   return old.activated_protocol_features != curr.activated_protocol_features;
}

struct state_history_plugin_impl : std::enable_shared_from_this<state_history_plugin_impl> {
   chain_plugin*                                              chain_plug = nullptr;
   fc::optional<state_history_log>                            trace_log;
   fc::optional<state_history_log>                            chain_state_log;
   bool                                                       trace_debug_mode = false;
   bool                                                       stopping = false;
   fc::optional<scoped_connection>                            applied_transaction_connection;
   fc::optional<scoped_connection>                            accepted_block_connection;
   string                                                     endpoint_address = "0.0.0.0";
   uint16_t                                                   endpoint_port    = 8080;
   std::unique_ptr<tcp::acceptor>                             acceptor;
   std::map<transaction_id_type, augmented_transaction_trace> cached_traces;
   fc::optional<augmented_transaction_trace>                  onblock_trace;

   void get_log_entry(state_history_log& log, uint32_t block_num, fc::optional<bytes>& result) {
      if (block_num < log.begin_block() || block_num >= log.end_block())
         return;
      state_history_log_header header;
      auto&                    stream = log.get_entry(block_num, header);
      uint32_t                 s;
      stream.read((char*)&s, sizeof(s));
      bytes compressed(s);
      if (s)
         stream.read(compressed.data(), s);
      result = zlib_decompress(compressed);
   }

   void get_block(uint32_t block_num, fc::optional<bytes>& result) {
      chain::signed_block_ptr p;
      try {
         p = chain_plug->chain().fetch_block_by_number(block_num);
      } catch (...) {
         return;
      }
      if (p)
         result = fc::raw::pack(*p);
   }

   fc::optional<chain::block_id_type> get_block_id(uint32_t block_num) {
      if (trace_log && block_num >= trace_log->begin_block() && block_num < trace_log->end_block())
         return trace_log->get_block_id(block_num);
      if (chain_state_log && block_num >= chain_state_log->begin_block() && block_num < chain_state_log->end_block())
         return chain_state_log->get_block_id(block_num);
      try {
         auto block = chain_plug->chain().fetch_block_by_number(block_num);
         if (block)
            return block->id();
      } catch (...) {
      }
      return {};
   }

   struct session : std::enable_shared_from_this<session> {
      std::shared_ptr<state_history_plugin_impl> plugin;
      std::unique_ptr<ws::stream<tcp::socket>>   socket_stream;
      bool                                       sending  = false;
      bool                                       sent_abi = false;
      std::vector<std::vector<char>>             send_queue;
      fc::optional<get_blocks_request_v0>        current_request;
      bool                                       need_to_send_update = false;

      session(std::shared_ptr<state_history_plugin_impl> plugin)
          : plugin(std::move(plugin)) {}

      void start(tcp::socket socket) {
         ilog("incoming connection");
         socket_stream = std::make_unique<ws::stream<tcp::socket>>(std::move(socket));
         socket_stream->binary(true);
         socket_stream->next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
         socket_stream->next_layer().set_option(boost::asio::socket_base::send_buffer_size(1024 * 1024));
         socket_stream->next_layer().set_option(boost::asio::socket_base::receive_buffer_size(1024 * 1024));
         socket_stream->async_accept([self = shared_from_this()](boost::system::error_code ec) {
            self->callback(ec, "async_accept", [self] {
               self->start_read();
               self->send(state_history_plugin_abi);
            });
         });
      }

      void start_read() {
         auto in_buffer = std::make_shared<boost::beast::flat_buffer>();
         socket_stream->async_read(
             *in_buffer, [self = shared_from_this(), in_buffer](boost::system::error_code ec, size_t) {
                self->callback(ec, "async_read", [self, in_buffer] {
                   auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(in_buffer->data()));
                   auto s = boost::asio::buffer_size(in_buffer->data());
                   fc::datastream<const char*> ds(d, s);
                   state_request               req;
                   fc::raw::unpack(ds, req);
                   req.visit(*self);
                   self->start_read();
                });
             });
      }

      void send(const char* s) {
         send_queue.push_back({s, s + strlen(s)});
         send();
      }

      template <typename T>
      void send(T obj) {
         send_queue.push_back(fc::raw::pack(state_result{std::move(obj)}));
         send();
      }

      void send() {
         if (sending)
            return;
         if (send_queue.empty())
            return send_update();
         sending = true;
         socket_stream->binary(sent_abi);
         sent_abi = true;
         socket_stream->async_write( //
             boost::asio::buffer(send_queue[0]),
             [self = shared_from_this()](boost::system::error_code ec, size_t) {
                self->callback(ec, "async_write", [self] {
                   self->send_queue.erase(self->send_queue.begin());
                   self->sending = false;
                   self->send();
                });
             });
      }

      using result_type = void;
      void operator()(get_status_request_v0&) {
         auto&                chain = plugin->chain_plug->chain();
         get_status_result_v0 result;
         result.head              = {chain.head_block_num(), chain.head_block_id()};
         result.last_irreversible = {chain.last_irreversible_block_num(), chain.last_irreversible_block_id()};
         if (plugin->trace_log) {
            result.trace_begin_block = plugin->trace_log->begin_block();
            result.trace_end_block   = plugin->trace_log->end_block();
         }
         if (plugin->chain_state_log) {
            result.chain_state_begin_block = plugin->chain_state_log->begin_block();
            result.chain_state_end_block   = plugin->chain_state_log->end_block();
         }
         send(std::move(result));
      }

      void operator()(get_blocks_request_v0& req) {
         for (auto& cp : req.have_positions) {
            if (req.start_block_num <= cp.block_num)
               continue;
            auto id = plugin->get_block_id(cp.block_num);
            if (!id || *id != cp.block_id)
               req.start_block_num = std::min(req.start_block_num, cp.block_num);
         }
         req.have_positions.clear();
         current_request = req;
         send_update(true);
      }

      void operator()(get_blocks_ack_request_v0& req) {
         if (!current_request)
            return;
         current_request->max_messages_in_flight += req.num_messages;
         send_update();
      }

      void send_update(bool changed = false) {
         if (changed)
            need_to_send_update = true;
         if (!send_queue.empty() || !need_to_send_update || !current_request ||
             !current_request->max_messages_in_flight)
            return;
         auto&                chain = plugin->chain_plug->chain();
         get_blocks_result_v0 result;
         result.head              = {chain.head_block_num(), chain.head_block_id()};
         result.last_irreversible = {chain.last_irreversible_block_num(), chain.last_irreversible_block_id()};
         uint32_t current =
             current_request->irreversible_only ? result.last_irreversible.block_num : result.head.block_num;
         if (current_request->start_block_num <= current &&
             current_request->start_block_num < current_request->end_block_num) {
            auto block_id = plugin->get_block_id(current_request->start_block_num);
            if (block_id) {
               result.this_block  = block_position{current_request->start_block_num, *block_id};
               auto prev_block_id = plugin->get_block_id(current_request->start_block_num - 1);
               if (prev_block_id)
                  result.prev_block = block_position{current_request->start_block_num - 1, *prev_block_id};
               if (current_request->fetch_block)
                  plugin->get_block(current_request->start_block_num, result.block);
               if (current_request->fetch_traces && plugin->trace_log)
                  plugin->get_log_entry(*plugin->trace_log, current_request->start_block_num, result.traces);
               if (current_request->fetch_deltas && plugin->chain_state_log)
                  plugin->get_log_entry(*plugin->chain_state_log, current_request->start_block_num, result.deltas);
            }
            ++current_request->start_block_num;
         }
         send(std::move(result));
         --current_request->max_messages_in_flight;
         need_to_send_update = current_request->start_block_num <= current &&
                               current_request->start_block_num < current_request->end_block_num;
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
         app().post( priority::medium, [=]() {
            if( plugin->stopping )
               return;
            if( ec )
               return on_fail( ec, what );
            catch_and_close( f );
         } );
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
         socket_stream->next_layer().close();
         plugin->sessions.erase(this);
      }
   };
   std::map<session*, std::shared_ptr<session>> sessions;

   void listen() {
      boost::system::error_code ec;

      auto address  = boost::asio::ip::make_address(endpoint_address);
      auto endpoint = tcp::endpoint{address, endpoint_port};
      acceptor      = std::make_unique<tcp::acceptor>(app().get_io_service());

      auto check_ec = [&](const char* what) {
         if (!ec)
            return;
         elog("${w}: ${m}", ("w", what)("m", ec.message()));
         EOS_ASSERT(false, plugin_exception, "unable to open listen socket");
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
      acceptor->async_accept(*socket, [self = shared_from_this(), socket, this](const boost::system::error_code& ec) {
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
         catch_and_log([&] { do_accept(); });
      });
   }

   static bool is_onblock(const transaction_trace_ptr& p) {
      if (p->action_traces.size() != 1)
         return false;
      auto& act = p->action_traces[0].act;
      if (act.account != eosio::chain::config::system_account_name || act.name != N(onblock) ||
          act.authorization.size() != 1)
         return false;
      auto& auth = act.authorization[0];
      return auth.actor == eosio::chain::config::system_account_name &&
             auth.permission == eosio::chain::config::active_name;
   }

   void on_applied_transaction(const transaction_trace_ptr& p, const signed_transaction& t) {
      if (p->receipt && trace_log) {
         if (is_onblock(p))
            onblock_trace.emplace(p, t);
         else if (p->failed_dtrx_trace)
            cached_traces[p->failed_dtrx_trace->id] = augmented_transaction_trace{p, t};
         else
            cached_traces[p->id] = augmented_transaction_trace{p, t};
      }
   }

   void on_accepted_block(const block_state_ptr& block_state) {
      cppkin::Trace trace = cppkin::Trace("StateHistory");
      auto ship_span = trace.CreateSpan("StateHistory");
      ship_span.AddSimpleTag("block_num", int(block_state->block_num));
      ship_span.AddSimpleTag("build_tag", std::getenv("BUILD_TAG"));

      store_traces(block_state);
      store_chain_state(block_state);
      for (auto& s : sessions) {
         auto& p = s.second;
         if (p) {
            if (p->current_request && block_state->block_num < p->current_request->start_block_num)
               p->current_request->start_block_num = block_state->block_num;
            p->send_update(true);
         }
      }

      ship_span.Submit();
   }

   void store_traces(const block_state_ptr& block_state) {
      if (!trace_log)
         return;
      std::vector<augmented_transaction_trace> traces;
      if (onblock_trace)
         traces.push_back(*onblock_trace);
      for (auto& r : block_state->block->transactions) {
         transaction_id_type id;
         if (r.trx.contains<transaction_id_type>())
            id = r.trx.get<transaction_id_type>();
         else
            id = r.trx.get<packed_transaction>().id();
         auto it = cached_traces.find(id);
         EOS_ASSERT(it != cached_traces.end() && it->second.trace->receipt, plugin_exception,
                    "missing trace for transaction ${id}", ("id", id));
         traces.push_back(it->second);
      }
      cached_traces.clear();
      onblock_trace.reset();

      auto& db         = chain_plug->chain().db();
      auto  traces_bin = zlib_compress_bytes(fc::raw::pack(make_history_context_wrapper(db, trace_debug_mode, traces)));
      EOS_ASSERT(traces_bin.size() == (uint32_t)traces_bin.size(), plugin_exception, "traces is too big");

      state_history_log_header header{.magic        = ship_magic(ship_current_version),
                                      .block_id     = block_state->block->id(),
                                      .payload_size = sizeof(uint32_t) + traces_bin.size()};
      trace_log->write_entry(header, block_state->block->previous, [&](auto& stream) {
         uint32_t s = (uint32_t)traces_bin.size();
         stream.write((char*)&s, sizeof(s));
         if (!traces_bin.empty())
            stream.write(traces_bin.data(), traces_bin.size());
      });
   }

   void store_chain_state(const block_state_ptr& block_state) {
      if (!chain_state_log)
         return;
      bool fresh = chain_state_log->begin_block() == chain_state_log->end_block();
      if (fresh)
         ilog("Placing initial state in block ${n}", ("n", block_state->block->block_num()));

      std::vector<table_delta> deltas;
      auto&                    db = chain_plug->chain().db();

      const auto&                                table_id_index = db.get_index<table_id_multi_index>();
      std::map<uint64_t, const table_id_object*> removed_table_id;
      for (auto& rem : table_id_index.stack().back().removed_values)
         removed_table_id[rem.first._id] = &rem.second;

      auto get_table_id = [&](uint64_t tid) -> const table_id_object& {
         auto obj = table_id_index.find(tid);
         if (obj)
            return *obj;
         auto it = removed_table_id.find(tid);
         EOS_ASSERT(it != removed_table_id.end(), chain::plugin_exception, "can not found table id ${tid}",
                    ("tid", tid));
         return *it->second;
      };

      auto pack_row          = [&](auto& row) { return fc::raw::pack(make_history_serial_wrapper(db, row)); };
      auto pack_contract_row = [&](auto& row) {
         return fc::raw::pack(make_history_context_wrapper(db, get_table_id(row.t_id._id), row));
      };

      auto process_table = [&](auto* name, auto& index, auto& pack_row) {
         if (fresh) {
            if (index.indices().empty())
               return;
            deltas.push_back({});
            auto& delta = deltas.back();
            delta.name  = name;
            for (auto& row : index.indices())
               delta.rows.obj.emplace_back(true, pack_row(row));
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
               if (include_delta(old.second, row))
                  delta.rows.obj.emplace_back(true, pack_row(row));
            }
            for (auto& old : undo.removed_values)
               delta.rows.obj.emplace_back(false, pack_row(old.second));
            for (auto id : undo.new_ids) {
               auto& row = index.get(id);
               delta.rows.obj.emplace_back(true, pack_row(row));
            }
         }
      };

      process_table("account", db.get_index<account_index>(), pack_row);
      process_table("account_metadata", db.get_index<account_metadata_index>(), pack_row);
      process_table("code", db.get_index<code_index>(), pack_row);

      process_table("contract_table", db.get_index<table_id_multi_index>(), pack_row);
      process_table("contract_row", db.get_index<key_value_index>(), pack_contract_row);
      process_table("contract_index64", db.get_index<index64_index>(), pack_contract_row);
      process_table("contract_index128", db.get_index<index128_index>(), pack_contract_row);
      process_table("contract_index256", db.get_index<index256_index>(), pack_contract_row);
      process_table("contract_index_double", db.get_index<index_double_index>(), pack_contract_row);
      process_table("contract_index_long_double", db.get_index<index_long_double_index>(), pack_contract_row);

      process_table("global_property", db.get_index<global_property_multi_index>(), pack_row);
      process_table("generated_transaction", db.get_index<generated_transaction_multi_index>(), pack_row);
      process_table("protocol_state", db.get_index<protocol_state_multi_index>(), pack_row);

      process_table("permission", db.get_index<permission_index>(), pack_row);
      process_table("permission_link", db.get_index<permission_link_index>(), pack_row);

      process_table("resource_limits", db.get_index<resource_limits::resource_limits_index>(), pack_row);
      process_table("resource_usage", db.get_index<resource_limits::resource_usage_index>(), pack_row);
      process_table("resource_limits_state", db.get_index<resource_limits::resource_limits_state_index>(), pack_row);
      process_table("resource_limits_config", db.get_index<resource_limits::resource_limits_config_index>(), pack_row);

      auto deltas_bin = zlib_compress_bytes(fc::raw::pack(deltas));
      EOS_ASSERT(deltas_bin.size() == (uint32_t)deltas_bin.size(), plugin_exception, "deltas is too big");
      state_history_log_header header{.magic        = ship_magic(ship_current_version),
                                      .block_id     = block_state->block->id(),
                                      .payload_size = sizeof(uint32_t) + deltas_bin.size()};
      chain_state_log->write_entry(header, block_state->block->previous, [&](auto& stream) {
         uint32_t s = (uint32_t)deltas_bin.size();
         stream.write((char*)&s, sizeof(s));
         if (!deltas_bin.empty())
            stream.write(deltas_bin.data(), deltas_bin.size());
      });
   } // store_chain_state
};   // state_history_plugin_impl

state_history_plugin::state_history_plugin()
    : my(std::make_shared<state_history_plugin_impl>()) {}

state_history_plugin::~state_history_plugin() {}

void state_history_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto options = cfg.add_options();
   options("state-history-dir", bpo::value<bfs::path>()->default_value("state-history"),
           "the location of the state-history directory (absolute path or relative to application data dir)");
   cli.add_options()("delete-state-history", bpo::bool_switch()->default_value(false), "clear state history files");
   options("trace-history", bpo::bool_switch()->default_value(false), "enable trace history");
   options("chain-state-history", bpo::bool_switch()->default_value(false), "enable chain state history");
   options("state-history-endpoint", bpo::value<string>()->default_value("127.0.0.1:8080"),
           "the endpoint upon which to listen for incoming connections. Caution: only expose this port to "
           "your internal network.");
   options("trace-history-debug-mode", bpo::bool_switch()->default_value(false),
           "enable debug mode for trace history");
}

void state_history_plugin::plugin_initialize(const variables_map& options) {
   try {
      EOS_ASSERT(options.at("disable-replay-opts").as<bool>(), plugin_exception,
                 "state_history_plugin requires --disable-replay-opts");

      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT(my->chain_plug, chain::missing_chain_plugin_exception, "");
      auto& chain = my->chain_plug->chain();
      my->applied_transaction_connection.emplace(
          chain.applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const signed_transaction&> t) {
             my->on_applied_transaction(std::get<0>(t), std::get<1>(t));
          }));
      my->accepted_block_connection.emplace(
          chain.accepted_block.connect([&](const block_state_ptr& p) { my->on_accepted_block(p); }));

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

      if (options.at("trace-history-debug-mode").as<bool>()) {
         my->trace_debug_mode = true;
      }

      if (options.at("trace-history").as<bool>())
         my->trace_log.emplace("trace_history", (state_history_dir / "trace_history.log").string(),
                               (state_history_dir / "trace_history.index").string());
      if (options.at("chain-state-history").as<bool>())
         my->chain_state_log.emplace("chain_state_history", (state_history_dir / "chain_state_history.log").string(),
                                     (state_history_dir / "chain_state_history.index").string());
   }
   FC_LOG_AND_RETHROW()
} // state_history_plugin::plugin_initialize

void state_history_plugin::plugin_startup() { my->listen(); }

void state_history_plugin::plugin_shutdown() {
   my->applied_transaction_connection.reset();
   my->accepted_block_connection.reset();
   while (!my->sessions.empty())
      my->sessions.begin()->second->close();
   my->stopping = true;
}

} // namespace eosio
