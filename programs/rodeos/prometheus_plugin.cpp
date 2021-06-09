#include <algorithm>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <fc/exception/exception.hpp>
#include <fc/log/logger.hpp>
#include <prometheus/text_serializer.h>
#include "prometheus_plugin.hpp"

namespace b1 {

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http  = beast::http;          // from <boost/beast/http.hpp>
namespace net   = boost::asio;          // from <boost/asio.hpp>
using tcp       = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>


metric_collection::metric_collection(const std::map<std::string, std::string>& labels)
    : summary_family(&prometheus::BuildSummary()
                      .Name("http_request_duration_seconds")
                      .Help("How long it took to process the request in seconds")
                      .Labels(labels)
                      .Register(registry)),
      counter_family(&prometheus::BuildCounter()
                     .Name("http_requests_total")
                     .Help("How many requests the service received")
                     .Labels(labels)
                     .Register(registry)) {}

void metric_collection::increment_http_request_total(std::string target) {
   auto [itr, _] = counters.try_emplace(target,
                                        &(counter_family->Add({ { "target", target } })));
   itr->second->Increment();
}

metric_collection::duration_recoder metric_collection::record_http_request_duration(std::string target) {
   static prometheus::Summary::Quantiles quantiles{{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}};
   auto [itr, _] = summaries.try_emplace(target, &(summary_family->Add({ { "target", target } }, quantiles)));
   return duration_recoder(itr->second);
}

static std::unique_ptr<metric_collection> the_metrics;

namespace rodeos::prometheus {


// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
   // Returns a bad request response
   auto const bad_request = [&req](beast::string_view why) {
      http::response<http::string_body> res{ http::status::bad_request, req.version() };
      res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
      res.set(http::field::content_type, "text/html");
      res.keep_alive(req.keep_alive());
      res.body() = std::string(why);
      res.prepare_payload();
      return res;
   };

   // Make sure we can handle the method
   if (req.method() != http::verb::get)
      return send(bad_request("Unknown HTTP-method"));

   // Request path must be "/metrics".
   if (req.target() != "/metrics")
      return send(bad_request("Illegal request-target"));

   // Respond to GET request
   const auto data = ::prometheus::TextSerializer().Serialize(the_metrics->registry.Collect());

   http::response<http::dynamic_body> res{ http::status::ok, req.version() };
   res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
   res.set(http::field::content_type, "text/plain");

   beast::ostream(res.body()) << data;
   res.prepare_payload();

   return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const* what) { elog("${w}: ${m}", ("w", what)("m", ec.message())); }

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session> {

   beast::tcp_stream                stream_;
   beast::flat_buffer               buffer_;
   http::request<http::string_body> req_;
   
 public:
   // Take ownership of the stream
   session(tcp::socket&& socket) : stream_(std::move(socket)) {}

   // Start the asynchronous operation
   void run() {
      // We need to be executing within a strand to perform async operations
      // on the I/O objects in this session. Although not strictly necessary
      // for single-threaded contexts, this example code is written to be
      // thread-safe by default.
      net::dispatch(stream_.get_executor(), beast::bind_front_handler(&session::do_read, shared_from_this()));
   }

   void do_read() {
      // Make the request empty before reading,
      // otherwise the operation behavior is undefined.
      req_ = {};

      // Set the timeout.
      stream_.expires_after(std::chrono::seconds(30));

      // Read a request
      http::async_read(stream_, buffer_, req_, beast::bind_front_handler(&session::on_read, shared_from_this()));
   }

   void on_read(beast::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);
      // This means they closed the connection
      if (ec == http::error::end_of_stream)
         return do_close();

      if (ec)
         return fail(ec, "read");

      // Send the response
      handle_request(std::move(req_), [self = shared_from_this()](auto&& msg) {
         auto sp = std::make_shared<std::decay_t<decltype(msg)>>(std::move(msg));
         http::async_write(self->stream_, *sp, [self, sp](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(sp->need_eof(), ec, bytes_transferred);
         });
      });
   }

   void on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);

      if (ec)
         return fail(ec, "write");

      if (close) {
         // This means we should close the connection, usually because
         // the response indicated the "Connection: close" semantic.
         return do_close();
      }

      // Read another request
      do_read();
   }

   void do_close() {
      // Send a TCP shutdown
      beast::error_code ec;
      stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

      // At this point the connection is closed gracefully
   }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
   net::io_context& ioc_;
   tcp::acceptor    acceptor_;

 public:
   listener(net::io_context& ioc, tcp::endpoint endpoint) : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
      beast::error_code ec;

      // Open the acceptor
      acceptor_.open(endpoint.protocol(), ec);
      if (ec) {
         fail(ec, "open");
         return;
      }

      // Allow address reuse
      acceptor_.set_option(net::socket_base::reuse_address(true), ec);
      if (ec) {
         fail(ec, "set_option");
         return;
      }

      // Bind to the server address
      acceptor_.bind(endpoint, ec);
      if (ec) {
         fail(ec, "bind");
         return;
      }

      // Start listening for connections
      acceptor_.listen(net::socket_base::max_listen_connections, ec);
      if (ec) {
         fail(ec, "listen");
         return;
      }
   }

   // Start accepting incoming connections
   void run() { do_accept(); }

 private:
   void do_accept() {
      // The new connection gets its own strand
      acceptor_.async_accept(net::make_strand(ioc_),
                             beast::bind_front_handler(&listener::on_accept, shared_from_this()));
   }

   void on_accept(beast::error_code ec, tcp::socket socket) {
      if (ec) {
         fail(ec, "accept");
      } else {
         // Create the session and run it
         std::make_shared<session>(std::move(socket))->run();
      }

      // Accept another connection
      do_accept();
   }
};
} // namespace rodeos::prometheus

struct prometheus_plugin_impl {
   boost::asio::ip::address address;
   uint16_t                 port;
   net::io_context          ioc;
   std::thread              thr;
};

static appbase::abstract_plugin& _prometheus_plugin = appbase::app().register_plugin<prometheus_plugin>();

prometheus_plugin::prometheus_plugin() : my(std::make_shared<prometheus_plugin_impl>()) {}

prometheus_plugin::~prometheus_plugin() { my->thr.join(); }

void prometheus_plugin::set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
   auto op = cfg.add_options();
   op("prometheus-listen", appbase::bpo::value<std::string>()->default_value("127.0.0.1:9001"),
      "Endpoint to listen on for prometheus exporting");
}

void prometheus_plugin::plugin_initialize(const appbase::variables_map& options) {
   try {
      auto ip_port = options.at("prometheus-listen").as<std::string>();
      if (ip_port.size()) {
         if (ip_port.find(':') == std::string::npos)
            throw std::runtime_error("invalid --wql-listen value: " + ip_port);
         my->port    = (unsigned short)std::atoi(ip_port.substr(ip_port.find(':') + 1, ip_port.size()).c_str());
         my->address = net::ip::make_address(ip_port.substr(0, ip_port.find(':')));
      }
      the_metrics  = std::make_unique<metric_collection>(std::map<std::string, std::string>{{"job", "rodeos"}});
      ilog("initialized prometheus_plugin");
   }
   FC_LOG_AND_RETHROW()
}

void prometheus_plugin::plugin_startup() {
   try {
      auto& ioc = my->ioc;
      std::make_shared<rodeos::prometheus::listener>(ioc, tcp::endpoint{ my->address, my->port })->run();
      my->thr = std::thread([&ioc]() { ioc.run(); });
   }
   FC_LOG_AND_RETHROW()
}

void prometheus_plugin::plugin_shutdown() { my->ioc.stop(); }

metric_collection& prometheus_plugin::metrics() { return *the_metrics; }
} // namespace b1
