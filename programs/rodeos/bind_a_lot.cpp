#include <appbase/application.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/make_unique.hpp>
#include <boost/optional.hpp>
#include <eosio/version/version.hpp>
#include <fc/exception/exception.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/appender.hpp>
#include <fc/log/logger.hpp>
#include <fc/log/logger_config.hpp>

#include "config.hpp"

using namespace appbase;
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http  = beast::http;          // from <boost/beast/http.hpp>
namespace net   = boost::asio;          // from <boost/asio.hpp>
using tcp       = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

using namespace std::literals;

void fail(beast::error_code ec, const char* what) { elog("${w}: ${s}", ("w", what)("s", ec.message())); }

class bind_a_lot_plugin : public appbase::plugin<bind_a_lot_plugin> {
 public:
   APPBASE_PLUGIN_REQUIRES()

   std::string         address;
   unsigned short      port = 8880;
   net::deadline_timer timer;

   bind_a_lot_plugin() : timer(app().get_io_service()) {}

   ~bind_a_lot_plugin() {}

   void set_program_options(appbase::options_description& cli, appbase::options_description& cfg) {
      auto op = cfg.add_options();
      op("wql-listen", bpo::value<std::string>()->default_value("127.0.0.1:8880"), "Endpoint to listen on");
   }

   void plugin_initialize(const appbase::variables_map& options) {
      auto ip_port = options.at("wql-listen").as<std::string>();
      if (ip_port.find(':') == std::string::npos)
         throw std::runtime_error("invalid --wql-listen value: " + ip_port);

      port    = atoi(ip_port.substr(ip_port.find(':') + 1, ip_port.size()).c_str());
      address = ip_port.substr(0, ip_port.find(':'));
   }

   void plugin_startup() { test_bind(); }

   void plugin_shutdown() { timer.cancel(); }

   void test_bind() {
      net::ip::address a;
      try {
         a = net::ip::make_address(address);
      } catch (std::exception& e) { throw std::runtime_error("make_address(): "s + address + ": " + e.what()); }

      [&] {
         tcp::endpoint endpoint{ a, port };
         tcp::acceptor acceptor{ net::make_strand(app().get_io_service()) };

         beast::error_code ec;

         ilog("trying...");

         // Open the acceptor
         acceptor.open(endpoint.protocol(), ec);
         if (ec) {
            fail(ec, "open");
            return;
         }

         // Bind to the server address
         acceptor.bind(endpoint, ec);
         if (ec) {
            fail(ec, "bind");
            return;
         }

         // Start listening for connections
         acceptor.listen(net::socket_base::max_listen_connections, ec);
         if (ec) {
            fail(ec, "listen");
            return;
         }

         ilog("bind sucessful. closing.");
      }();

      timer.expires_from_now(boost::posix_time::milliseconds(10));
      timer.async_wait([this](auto&) { test_bind(); });
   }
};

static abstract_plugin& _bind_a_lot_plugin = app().register_plugin<bind_a_lot_plugin>();

enum return_codes {
   other_fail      = -2,
   initialize_fail = -1,
   success         = 0,
   bad_alloc       = 1,
};

int main(int argc, char** argv) {
   try {
      app().set_version(b1::rodeos::config::version);
      app().set_version_string(eosio::version::version_client());
      app().set_full_version_string(eosio::version::version_full());

      auto root = fc::app_path();
      app().set_default_data_dir(root / "eosio" / b1::rodeos::config::rodeos_executable_name / "data");
      app().set_default_config_dir(root / "eosio" / b1::rodeos::config::rodeos_executable_name / "config");
      if (!app().initialize<bind_a_lot_plugin>(argc, argv)) {
         const auto& opts = app().get_options();
         if (opts.count("help") || opts.count("version") || opts.count("full-version") ||
             opts.count("print-default-config")) {
            return success;
         }
         return initialize_fail;
      }
      ilog("bind-a-lot version ${ver} ${fv}",
           ("ver", app().version_string())(
                 "fv", app().version_string() == app().full_version_string() ? "" : app().full_version_string()));
      ilog("bind-a-lot using configuration file ${c}", ("c", app().full_config_file_path().string()));
      ilog("bind-a-lot data directory is ${d}", ("d", app().data_dir().string()));
      app().startup();
      app().set_thread_priority_max();
      app().exec();
   } catch (const fc::std_exception_wrapper& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return other_fail;
   } catch (const fc::exception& e) {
      elog("${e}", ("e", e.to_detail_string()));
      return other_fail;
   } catch (const boost::interprocess::bad_alloc& e) {
      elog("bad alloc");
      return bad_alloc;
   } catch (const boost::exception& e) {
      elog("${e}", ("e", boost::diagnostic_information(e)));
      return other_fail;
   } catch (const std::exception& e) {
      elog("${e}", ("e", e.what()));
      return other_fail;
   } catch (...) {
      elog("unknown exception");
      return other_fail;
   }

   ilog("bind_a_lot successfully exiting");
   return success;
}
