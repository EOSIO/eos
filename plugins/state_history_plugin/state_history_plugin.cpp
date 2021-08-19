#include <eosio/chain/config.hpp>
#include <eosio/resource_monitor_plugin/resource_monitor_plugin.hpp>
#include <eosio/state_history/log.hpp>
#include <eosio/state_history/serialization.hpp>
#include <eosio/state_history_plugin/state_history_plugin.hpp>

#include <fc/log/trace.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/signals2/connection.hpp>

#include <variant>
#include <thread>

using tcp    = boost::asio::ip::tcp;
using unixs  = boost::asio::local::stream_protocol;
namespace ws = boost::beast::websocket;

extern const char* const state_history_plugin_abi;

namespace eosio {
using namespace chain;
using namespace state_history;
using boost::signals2::scoped_connection;

static appbase::abstract_plugin& _state_history_plugin = app().register_plugin<state_history_plugin>();

const std::string logger_name("state_history");
fc::logger _log;

template <typename F>
auto catch_and_log(F f) {
   try {
      return f();
   } catch (const fc::exception& e) {
      fc_elog(_log, "${e}", ("e", e.to_detail_string()));
   } catch (const std::exception& e) {
      fc_elog(_log, "${e}", ("e", e.what()));
   } catch (...) {
      fc_elog(_log, "unknown exception");
   }
}

struct state_history_plugin_impl : std::enable_shared_from_this<state_history_plugin_impl> {
   chain_plugin*                                              chain_plug = nullptr;
   std::optional<state_history_traces_log>                    trace_log;
   std::optional<state_history_chain_state_log>               chain_state_log;
   std::atomic<bool>                                          stopping = false;
   std::optional<scoped_connection>                           applied_transaction_connection;
   std::optional<scoped_connection>                           block_start_connection;
   std::optional<scoped_connection>                           accepted_block_connection;
   string                                                     endpoint_address;
   uint16_t                                                   endpoint_port    = 8080;
   std::unique_ptr<tcp::acceptor>                             acceptor;
   string                                                     unix_path;
   std::unique_ptr<unixs::acceptor>                           unix_acceptor;

   std::thread                                                              thr;
   boost::asio::io_context                                                  ctx;
   boost::asio::io_context::strand                                          work_strand{ctx};
   boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard =
       boost::asio::make_work_guard(ctx);

   std::optional<chain::block_id_type> get_block_id(uint32_t block_num) {
      std::optional<chain::block_id_type> result;

      if (trace_log)
         result = trace_log->get_block_id(block_num);

      if (!result && chain_state_log)
         result = chain_state_log->get_block_id(block_num);

      if (result)
         return result;

      try {
         return chain_plug->chain().get_block_id_for_num(block_num);
      } catch (...) {
         return {};
      }
   }

   using get_blocks_request = std::variant<get_blocks_request_v0, get_blocks_request_v1>;

   struct session_base {
      virtual void send_update(const block_state_ptr& block_state, ::std::optional<::fc::zipkin_span>&& span) = 0;
      virtual void close()                                                                                    = 0;
      virtual ~session_base()                                                                                 = default;

      std::optional<get_blocks_request>          current_request; 
   };

   template <typename SessionType>
   struct session : session_base {
      state_history_plugin_impl*                 plugin;
      bool                                       need_to_send_update = false;

      uint32_t                                       to_send_block_num = 0;
      std::optional<std::vector<block_position>::const_iterator> position_it;

      session(state_history_plugin_impl* plugin)
          : plugin(std::move(plugin)) {}

      ~session() {
      }

      SessionType& derived_session() {
         return static_cast<SessionType&>(*this);
      }
      
      void operator()(get_status_request_v0&&) {
         auto request_span = fc_create_trace("get_status_request");
         fc_ilog(_log, "got get_status_request_v0");
         auto&                chain = plugin->chain_plug->chain();
         get_status_result_v0 result;
         result.head              = {chain.head_block_num(), chain.head_block_id()};
         result.last_irreversible = {chain.last_irreversible_block_num(), chain.last_irreversible_block_id()};
         result.chain_id          = chain.get_chain_id();
         if (plugin->trace_log) {
            std::tie(result.trace_begin_block, result.trace_end_block) = plugin->trace_log->begin_end_block_nums();
         }
         if (plugin->chain_state_log) {
            std::tie(result.chain_state_begin_block, result.chain_state_end_block) =
                plugin->chain_state_log->begin_end_block_nums();
         }
         fc_ilog(_log, "pushing get_status_result_v0 to send queue");
         derived_session().send(std::move(result), fc_get_token(request_span));
      }

      template <typename T>
      std::enable_if_t<std::is_base_of_v<get_blocks_request_v0,T>>
      operator()(T&& req) {
         fc_ilog(_log, "received get_blocks_request = ${req}", ("req",req) );
         auto request_span = fc_create_trace("get_blocks_request");
         to_send_block_num = req.start_block_num;
         for (auto& cp : req.have_positions) {
            if (req.start_block_num <= cp.block_num)
               continue;
            auto id = plugin->get_block_id(cp.block_num);
            if (!id) {
               to_send_block_num = std::min(to_send_block_num, cp.block_num);
               fc_dlog(_log, "block ${block_num} is not available", ("block_num", cp.block_num));
            } else if (*id != cp.block_id) {
               to_send_block_num = std::min(to_send_block_num, cp.block_num);
               fc_dlog(_log, "the id for block ${block_num} in block request have_positions does not match the existing", ("block_num", cp.block_num));
            }
         }
         
         fc_dlog(_log, "  get_blocks_request start_block_num set to ${num}", ("num", to_send_block_num));

         if (req.have_positions.size()) {
            position_it = req.have_positions.begin();
         }

         current_request = std::move(req);
         send_update(std::move(request_span), true);
      }

      void operator()(get_blocks_ack_request_v0&& ack_req) {
         fc_ilog(_log, "received get_blocks_ack_request_v0 = ${req}", ("req",ack_req));
         if (!current_request.has_value()) {
            fc_dlog(_log, " no current get_blocks_request_v0, discarding the get_blocks_ack_request_v0");
            return;
         }
         auto request_span = fc_create_trace("get_blocks_ack_request");
         std::visit([num_messages = ack_req.num_messages](auto& req) { req.max_messages_in_flight += num_messages; },
                    *current_request);

         send_update(std::move(request_span));
      }

      void set_result_block_header(get_blocks_result_v1&, const signed_block_ptr& block) {}
      void set_result_block_header(get_blocks_result_v2& result, const signed_block_ptr& block) {
         bool fetch_block_header = std::get<get_blocks_request_v1>(*current_request).fetch_block_header;
         if (fetch_block_header && block) {
            result.block_header = static_cast<const signed_block_header&>(*block); 
         }
      }

      uint32_t max_messages_in_flight() const {
         if (current_request)
            return std::visit( [](const auto& x){ return x.max_messages_in_flight; }, *current_request);
         return 0;
      }

      template <typename T>
      std::enable_if_t<std::is_same_v<get_blocks_result_v1,T> || std::is_same_v<get_blocks_result_v2,T>>
      send_update(const block_state_ptr& head_block_state, T&& result, ::std::optional<::fc::zipkin_span>&& span) {
         need_to_send_update = true;
         if (!max_messages_in_flight() )
            return;
         get_blocks_request_v0& block_req = std::visit([](auto& x) ->get_blocks_request_v0&{  return x; }, *current_request);
         
         auto& chain              = plugin->chain_plug->chain();
         result.last_irreversible = {chain.last_irreversible_block_num(), chain.last_irreversible_block_id()};
         uint32_t current =
               block_req.irreversible_only ? result.last_irreversible.block_num : result.head.block_num;

         if (to_send_block_num > current || to_send_block_num >= block_req.end_block_num)
            return;

         auto block_id  = plugin->get_block_id(to_send_block_num);

         if (block_id && position_it && (*position_it)->block_num == to_send_block_num) {
            // This branch happens when the head block of nodeos is behind the head block of connecting client.
            // In addition, the client told us the corresponding block id for block_num we are going to send.
            // We can send the block when the block_id is different.
            auto& itr = *position_it;
            auto block_id_seen_by_client = itr->block_id;
            ++itr;
            if (itr == block_req.have_positions.end())
               position_it.reset();

            if(block_id_seen_by_client == *block_id) {
               ++to_send_block_num;
               return;
            }            
         }

         auto send_update_span = fc_create_span(span, "ship-send-update");
         fc_add_tag( send_update_span, "head_block_num", result.head.block_num );
         fc_add_tag(send_update_span, "block_num", to_send_block_num);

         auto get_block = [&chain, block_num = to_send_block_num, head_block_state]() -> signed_block_ptr {
            try {
               if (head_block_state->block_num == block_num)
                  return head_block_state->block;
               return chain.fetch_block_by_number(block_num);
            } catch (...) {
               return {};
            }
         };

         if (block_id) {
            result.this_block  = block_position{to_send_block_num, *block_id};
            auto prev_block_id = plugin->get_block_id(to_send_block_num - 1);
            if (prev_block_id) 
               result.prev_block = block_position{to_send_block_num - 1, *prev_block_id};
            if (block_req.fetch_block) {
                  result.block = signed_block_ptr_variant{get_block()};
            }
            if (block_req.fetch_traces && plugin->trace_log) {
               result.traces = plugin->trace_log->get_log_entry(to_send_block_num);
               fc_add_tag(send_update_span, "traces_size", result.traces.data_size());
            }
            if (block_req.fetch_deltas && plugin->chain_state_log) {
               result.deltas = plugin->chain_state_log->get_log_entry(to_send_block_num);
               fc_add_tag(send_update_span, "deltas_size", result.deltas.data_size());
            }
            set_result_block_header(result, get_block());
         }
         ++to_send_block_num;
         
         fc_ilog(_log, "pushing result {\"head\":{\"block_num\":${head}},\"last_irreversible\":{\"block_num\":${last_irr}},\"this_block\":{\"block_num\":${this_block}, \"id\": ${id}}} to send queue", 
               ("head", result.head.block_num)("last_irr", result.last_irreversible.block_num)
               ("this_block", result.this_block ? result.this_block->block_num : fc::variant())
               ("id", block_id ? block_id->_hash[3] : 0 ));

         derived_session().send(std::move(result), fc_get_token(send_update_span));
         --block_req.max_messages_in_flight;
         need_to_send_update = to_send_block_num <= current &&
                               to_send_block_num < block_req.end_block_num;

      }

      void send_update(const block_state_ptr& head_block_state, std::optional<::fc::zipkin_span>&& span) override {
         if (!current_request)
            return;
         std::visit(
            [&head_block_state, this, span = std::move(span)](const auto& req) mutable {
               (void)this; // avoid unused lambda warning
               // send get_blocks_result_v1 when the request is get_blocks_request_v0 and
               // send send_block_result_v2 when the request is get_blocks_request_v1. 
               if (head_block_state->block) {
                  typename std::decay_t<decltype(req)>::response_type result;
                  result.head = {head_block_state->block_num, head_block_state->id};
                  if (::fc::zipkin_config::is_enabled() && !span) {
                     span.emplace("send-update-0", fc::zipkin_span::to_id(head_block_state->id),
                                 "ship"_n.to_uint64_t());
                  }
                  send_update(head_block_state, std::move(result), std::move(span)); 
               }
            },
            *current_request);
      }

      void send_update(::std::optional<::fc::zipkin_span>&& span, bool changed = false) {
         if (changed || need_to_send_update) {
            auto& chain = plugin->chain_plug->chain();
            send_update(chain.head_block_state(), std::move(span));
         }
      }
   };

   template <typename SocketType>
   class ws_session : public session<ws_session<SocketType>>,
                      public std::enable_shared_from_this<ws_session<SocketType>> {
    public:
      using socket_type = SocketType;
      ws_session(state_history_plugin_impl* plugin, SocketType socket)
          : session<ws_session<SocketType>>(plugin), socket_stream(std::move(socket)) {
      }
      void start() {
         fc_ilog(_log, "incoming connection");
         if constexpr (std::is_same_v<SocketType, tcp::socket>) {
            socket_stream.next_layer().set_option(boost::asio::ip::tcp::no_delay(true));
         }
         socket_stream.auto_fragment(false);
         socket_stream.binary(true);
         socket_stream.next_layer().set_option(boost::asio::socket_base::send_buffer_size(1024 * 1024));
         socket_stream.next_layer().set_option(boost::asio::socket_base::receive_buffer_size(1024 * 1024));
         socket_stream.async_accept([self = this->shared_from_this()](boost::system::error_code ec) {
            self->callback(ec, "async_accept", [self] {
               self->socket_stream.binary(false);
               self->socket_stream.async_write(
                   boost::asio::buffer(state_history_plugin_abi, strlen(state_history_plugin_abi)),
                   [self](boost::system::error_code ec, size_t) {
                      self->callback(ec, "async_write", [self] {
                         self->socket_stream.binary(true);
                         self->start_read();
                      });
                   });
            });
         });
      }

      template <typename T>
      void send(T obj, fc::zipkin_span::token token) {
         boost::asio::post(this->plugin->work_strand, [this, obj = std::move(obj), token ]() {
            send_queue.emplace_back(fc::raw::pack(state_result{std::move(obj)}), token);
            send();
         });
      }

      void close() override {
         boost::asio::post(this->plugin->work_strand, [p = std::weak_ptr(this->weak_from_this())]() {
            auto self = p.lock();
            if (self) {
               self->close_i();
            }
         });
      }

    private:
      // all private member functions are called from internal thread
      void start_read() {
         auto in_buffer = std::make_shared<boost::beast::flat_buffer>();
         socket_stream.async_read(
             *in_buffer, [self = this->shared_from_this(), in_buffer](boost::system::error_code ec, size_t) {
                self->callback(ec, "async_read", [self, in_buffer] {
                   auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(in_buffer->data()));
                   auto s = boost::asio::buffer_size(in_buffer->data());
                   fc::datastream<const char*> ds(d, s);
                   state_request               req;
                   fc::raw::unpack(ds, req);
                   app().post(priority::medium,
                              [self, req = std::move(req)]() mutable { std::visit(*self, std::move(req)); });
                   self->start_read();
                });
             });
      }

      void send() {
         if (sending)
            return;
         if (send_queue.empty()) {
            app().post(priority::medium, [self = this->shared_from_this()]() {
               self->send_update(::std::optional<::fc::zipkin_span>{});
            });
            return;
         }
         sending = true;

         auto send_span = fc_create_span_from_token(send_queue[0].second, "send");
         fc_add_tag(send_span, "buffer_size", send_queue[0].first.size());
         socket_stream.async_write( //
             boost::asio::buffer(send_queue[0].first),
             [self = this->shared_from_this(), send_span = std::move(send_span)](boost::system::error_code ec,
                                                                                 size_t) mutable {
                send_span.reset();
                self->send_queue.erase(self->send_queue.begin());
                self->sending = false;
                self->callback(ec, "async_write", [self] { self->send(); });
             });
      }

      template <typename F>
      void catch_and_close(F f) {
         try {
            f();
         } catch (const fc::exception& e) {
            fc_elog(_log, "${e}", ("e", e.to_detail_string()));
            close_i();
         } catch (const std::exception& e) {
            fc_elog(_log, "${e}", ("e", e.what()));
            close_i();
         } catch (...) {
            fc_elog(_log, "unknown exception");
            close_i();
         }
      }

      template <typename F>
      void callback(boost::system::error_code ec, const char* what, F f) {
         if (this->plugin->stopping)
            return;
         if (ec) {
            fc_elog(_log, "${w}: ${m}", ("w", what)("m", ec.message()));
            close_i();
            return;
         }
         catch_and_close(f);
      }

      void close_i() {
         boost::system::error_code ec;
         socket_stream.next_layer().close(ec);
         if (ec) fc_elog(_log, "close: ${m}", ("m", ec.message()));
         this->plugin->sessions.remove(this->shared_from_this());
      }

      ws::stream<SocketType> socket_stream;
      using send_queue_t = std::vector<std::pair<std::vector<char>, fc::zipkin_span::token>>;
      send_queue_t send_queue;
      bool         sending = false;
   };

   class session_manager_t {
      std::mutex                                                mx;
      boost::container::flat_set<std::shared_ptr<session_base>> session_set;

    public:
      template <typename SocketType>
      void add(state_history_plugin_impl* plugin, std::shared_ptr<SocketType> socket) {
         auto s = std::make_shared<ws_session<SocketType>>(plugin, std::move(*socket));
         s->start();
         std::lock_guard lock(mx);
         session_set.insert(std::move(s));
      }

      void remove(std::shared_ptr<session_base> s) {
         std::lock_guard lock(mx);
         session_set.erase(s);
      }

      template <typename F>
      void for_each(F&& f) {
         std::lock_guard lock(mx);
         for (auto& s : session_set) {
            f(s);
         }
      }
   } sessions;

   void listen() {
      boost::system::error_code ec;

      auto address  = boost::asio::ip::make_address(endpoint_address);
      auto endpoint = tcp::endpoint{address, endpoint_port};
      acceptor      = std::make_unique<tcp::acceptor>(this->ctx);

      auto check_ec = [&](const char* what) {
         if (!ec)
            return;
         fc_elog(_log,"${w}: ${m}", ("w", what)("m", ec.message()));
         EOS_ASSERT(false, plugin_exception, "unable to open listen socket");
      };

      acceptor->open(endpoint.protocol(), ec);
      check_ec("open");
      acceptor->set_option(boost::asio::socket_base::reuse_address(true));
      acceptor->bind(endpoint, ec);
      check_ec("bind");
      acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      check_ec("listen");
      do_accept(*acceptor);
   }

   void unix_listen() {
      boost::system::error_code ec;

      auto check_ec = [&](const char* what) {
         if (!ec)
            return;
         fc_elog(_log,"${w}: ${m}", ("w", what)("m", ec.message()));
         EOS_ASSERT(false, plugin_exception, "unable to open unix socket");
      };

      //take a sniff and see if anything is already listening at the given socket path, or if the socket path exists
      // but nothing is listening
      {
         boost::system::error_code test_ec;
         unixs::socket test_socket(app().get_io_service());
         test_socket.connect(unix_path.c_str(), test_ec);

         //looks like a service is already running on that socket, don't touch it... fail out
         if(test_ec == boost::system::errc::success)
            ec = boost::system::errc::make_error_code(boost::system::errc::address_in_use);
         //socket exists but no one home, go ahead and remove it and continue on
         else if(test_ec == boost::system::errc::connection_refused)
            ::unlink(unix_path.c_str());
         else if(test_ec != boost::system::errc::no_such_file_or_directory)
            ec = test_ec;
      }

      check_ec("open");

      unix_acceptor = std::make_unique<unixs::acceptor>(this->ctx);
      unix_acceptor->open(unixs::acceptor::protocol_type(), ec);
      check_ec("open");
      unix_acceptor->bind(unix_path.c_str(), ec);
      check_ec("bind");
      unix_acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      check_ec("listen");
      do_accept(*unix_acceptor);
   }

   template <typename Acceptor>
   void do_accept(Acceptor& acceptor) {
      auto socket = std::make_shared<typename Acceptor::protocol_type::socket>(this->ctx);
      acceptor.async_accept(*socket, [this, socket, &acceptor](const boost::system::error_code& ec) {
         if (stopping)
            return;
         if (ec) {
            if (ec == boost::system::errc::too_many_files_open)
               catch_and_log([&] { do_accept(acceptor); });
            return;
         }
         catch_and_log([&] {
            sessions.add(this, socket);
         });
         catch_and_log([&] { do_accept(acceptor); });
      });
   }

   void on_applied_transaction(const transaction_trace_ptr& p, const packed_transaction_ptr& t) {
      if (trace_log)
         trace_log->add_transaction(p, t);
   }

   void store(const block_state_ptr& block_state, const ::std::optional<::fc::zipkin_span>& ship_accept_span) {
      try {
         if (trace_log) {
            auto trace_log_span = fc_create_span(ship_accept_span, "store_trace_log");
            trace_log->store(chain_plug->chain().db(), block_state);
         }
         if (chain_state_log) {
            auto delta_log_span = fc_create_span(ship_accept_span, "store_delta_log");
            chain_state_log->store(chain_plug->chain().kv_db(), block_state);
         }
         return;
      }
      FC_LOG_AND_DROP()

      // Both app().quit() and exception throwing are required. Without app().quit(),
      // the exception would be caught and drop before reaching main(). The exception is
      // to ensure the block won't be committed.
      appbase::app().quit();
      EOS_THROW(
          chain::state_history_write_exception,
          "State history encountered an Error which it cannot recover from.  Please resolve the error and relaunch "
          "the process");
   }

   void on_accepted_block(const block_state_ptr& block_state) {
      auto ship_accept_span = fc_create_span_with_id("SHiP-Accepted", "ship"_n.to_uint64_t() , block_state->id);

      fc_add_tag(ship_accept_span, "block_id", block_state->id);
      fc_add_tag(ship_accept_span, "block_num", block_state->block_num);
      fc_add_tag(ship_accept_span, "block_time", block_state->block->timestamp.to_time_point());
      
      this->store(block_state, ship_accept_span);

      sessions.for_each([&block_state, &ship_accept_span](auto& s) {
         s->send_update(block_state, fc_create_span(ship_accept_span, "send_update"));
      });
   }

   void on_block_start(uint32_t block_num) {
      if (trace_log)
         trace_log->block_start(block_num);
   }

   ~state_history_plugin_impl() {
      boost::system::error_code ec;
      if (unix_acceptor)
         if(const auto ep = unix_acceptor->local_endpoint(ec); !ec)
            ::unlink(ep.path().c_str());
   }

};   // state_history_plugin_impl

state_history_plugin::state_history_plugin()
    : my(std::make_shared<state_history_plugin_impl>()) {}

state_history_plugin::~state_history_plugin() {}

void state_history_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto options = cfg.add_options();
   options("state-history-dir", bpo::value<bfs::path>()->default_value("state-history"),
           "the location of the state-history directory (absolute path or relative to application data dir)");
   options("state-history-retained-dir", bpo::value<bfs::path>()->default_value(""),
           "the location of the state history retained directory (absolute path or relative to state-history dir).\n"
           "If the value is empty, it is set to the value of state-history directory.");
   options("state-history-archive-dir", bpo::value<bfs::path>()->default_value("archive"),
           "the location of the state history archive directory (absolute path or relative to state-history dir).\n"
           "If the value is empty, blocks files beyond the retained limit will be deleted.\n"
           "All files in the archive directory are completely under user's control, i.e. they won't be accessed by nodeos anymore.");
   options("state-history-stride", bpo::value<uint32_t>()->default_value(UINT32_MAX),
         "split the state history log files when the block number is the multiple of the stride\n"
         "When the stride is reached, the current history log and index will be renamed '*-history-<start num>-<end num>.log/index'\n"
         "and a new current history log and index will be created with the most recent blocks. All files following\n"
         "this format will be used to construct an extended history log.");
   options("max-retained-history-files", bpo::value<uint32_t>()->default_value(UINT32_MAX),
          "the maximum number of history file groups to retain so that the blocks in those files can be queried.\n" 
          "When the number is reached, the oldest history file would be moved to archive dir or deleted if the archive dir is empty.\n"
          "The retained history log files should not be manipulated by users." );
   cli.add_options()("delete-state-history", bpo::bool_switch()->default_value(false), "clear state history files");
   options("trace-history", bpo::bool_switch()->default_value(false), "enable trace history");
   options("chain-state-history", bpo::bool_switch()->default_value(false), "enable chain state history");
   options("state-history-endpoint", bpo::value<string>()->default_value("127.0.0.1:8080"),
           "the endpoint upon which to listen for incoming connections. Caution: only expose this port to "
           "your internal network.");
   options("state-history-unix-socket-path", bpo::value<string>(),
           "the path (relative to data-dir) to create a unix socket upon which to listen for incoming connections.");
   options("trace-history-debug-mode", bpo::bool_switch()->default_value(false),
           "enable debug mode for trace history");
   options("context-free-data-compression", bpo::value<string>()->default_value("zlib"), 
           "compression mode for context free data in transaction traces. Supported options are \"zlib\" and \"none\"");
}

void state_history_plugin::plugin_initialize(const variables_map& options) {
   try {
      EOS_ASSERT(options.at("disable-replay-opts").as<bool>(), plugin_exception,
                 "state_history_plugin requires --disable-replay-opts");

      my->chain_plug = app().find_plugin<chain_plugin>();
      EOS_ASSERT(my->chain_plug, chain::missing_chain_plugin_exception, "");
      auto& chain = my->chain_plug->chain();
      my->applied_transaction_connection.emplace(
          chain.applied_transaction.connect([&](std::tuple<const transaction_trace_ptr&, const packed_transaction_ptr&> t) {
             my->on_applied_transaction(std::get<0>(t), std::get<1>(t));
          }));
      my->accepted_block_connection.emplace(
          chain.accepted_block.connect([&](const block_state_ptr& p) { my->on_accepted_block(p); }));
      my->block_start_connection.emplace(
          chain.block_start.connect([&](uint32_t block_num) { my->on_block_start(block_num); }));

      auto  dir_option = options.at("state-history-dir").as<bfs::path>();

      static eosio::state_history_config config;
      config.logger = &_log;

      if (dir_option.is_relative())
         config.log_dir = app().data_dir() / dir_option;
      else
         config.log_dir = dir_option;
      if (auto resmon_plugin = app().find_plugin<resource_monitor_plugin>())
         resmon_plugin->monitor_directory(config.log_dir);

      config.retained_dir       = options.at("state-history-retained-dir").as<bfs::path>();
      config.archive_dir        = options.at("state-history-archive-dir").as<bfs::path>();
      config.stride             = options.at("state-history-stride").as<uint32_t>();
      config.max_retained_files = options.at("max-retained-history-files").as<uint32_t>();

      auto ip_port         = options.at("state-history-endpoint").as<string>();
      if (ip_port.size()) {
         auto port            = ip_port.substr(ip_port.find(':') + 1, ip_port.size());
         auto host            = ip_port.substr(0, ip_port.find(':'));
         my->endpoint_address = host;
         my->endpoint_port    = std::stoi(port);
         idump((ip_port)(host)(port));
      }

      if (options.count("state-history-unix-socket-path")) {
         boost::filesystem::path sock_path = options.at("state-history-unix-socket-path").as<string>();
         if (sock_path.is_relative())
            sock_path = app().data_dir() / sock_path;
         my->unix_path = sock_path.generic_string();
      }

      if (options.at("delete-state-history").as<bool>()) {
         fc_ilog(_log, "Deleting state history");
         boost::filesystem::remove_all(config.log_dir);
      }
      boost::filesystem::create_directories(config.log_dir);

      if (options.at("trace-history").as<bool>()) {
         my->trace_log.emplace(config);
         if (options.at("trace-history-debug-mode").as<bool>()) 
            my->trace_log->trace_debug_mode = true;  

         auto compression = options.at("context-free-data-compression").as<string>();
         if (compression == "zlib") {
            my->trace_log->compression = state_history::compression_type::zlib;
         } else if (compression == "none") {
            my->trace_log->compression = state_history::compression_type::none;
         } else {
            throw bpo::validation_error(bpo::validation_error::invalid_option_value);
         }
      }

      if (options.at("chain-state-history").as<bool>())
         my->chain_state_log.emplace(config);
   }
   FC_LOG_AND_RETHROW()
} // state_history_plugin::plugin_initialize

void state_history_plugin::plugin_startup() { 
   handle_sighup(); // setup logging

   try {
      my->thr = std::thread([ptr = my.get()]{ ptr->ctx.run(); });

      if (my->endpoint_address.size())
         my->listen();
      if (my->unix_path.size())
         my->unix_listen();
   }
   catch(std::exception& ex) {
      appbase::app().quit();
   }
}

void state_history_plugin::plugin_shutdown() {
   try {
      my->applied_transaction_connection.reset();
      my->accepted_block_connection.reset();
      my->block_start_connection.reset();
      my->sessions.for_each([](auto& s) { s->close(); });
      my->stopping = true;
      my->trace_log->stop();
      my->chain_state_log->stop();
      if (my->thr.joinable()) {
         my->work_guard.reset();
         my->ctx.stop();
         my->thr.join();
      }
   }
   FC_CAPTURE_LOG_AND_RETHROW((""))
}

void state_history_plugin::handle_sighup() {
   fc::logger::update( logger_name, _log );
}

} // namespace eosio
