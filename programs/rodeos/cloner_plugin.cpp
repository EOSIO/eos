#include "cloner_plugin.hpp"
#include "ship_client.hpp"

#include <b1/rodeos/rodeos.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace b1 {

using namespace appbase;
using namespace std::literals;
using namespace eosio::ship_protocol;

namespace asio          = boost::asio;
namespace bpo           = boost::program_options;
namespace ship_protocol = eosio::ship_protocol;
namespace websocket     = boost::beast::websocket;

using asio::ip::tcp;
using boost::beast::flat_buffer;
using boost::system::error_code;

using rodeos::rodeos_db_partition;
using rodeos::rodeos_db_snapshot;
using rodeos::rodeos_filter;

struct cloner_session;

struct cloner_config : ship_client::connection_config {
   uint32_t    skip_to     = 0;
   uint32_t    stop_before = 0;
   eosio::name filter_name = {}; // todo: remove
   std::string filter_wasm = {}; // todo: remove
};

struct cloner_plugin_impl : std::enable_shared_from_this<cloner_plugin_impl> {
   std::shared_ptr<cloner_config>  config = std::make_shared<cloner_config>();
   std::shared_ptr<cloner_session> session;
   boost::asio::deadline_timer     timer;

   cloner_plugin_impl() : timer(app().get_io_service()) {}

   ~cloner_plugin_impl();

   void schedule_retry() {
      timer.expires_from_now(boost::posix_time::seconds(1));
      timer.async_wait([this](auto&) {
         ilog("retry...");
         start();
      });
   }

   void start();
};

struct cloner_session : ship_client::connection_callbacks, std::enable_shared_from_this<cloner_session> {
   cloner_plugin_impl*                  my = nullptr;
   std::shared_ptr<cloner_config>       config;
   std::shared_ptr<chain_kv::database>  db = app().find_plugin<rocksdb_plugin>()->get_db();
   std::shared_ptr<rodeos_db_partition> partition =
         std::make_shared<rodeos_db_partition>(db, std::vector<char>{}); // todo: prefix

   std::optional<rodeos_db_snapshot>        rodeos_snapshot;
   std::shared_ptr<ship_client::connection> connection;
   bool                                     reported_block = false;
   std::unique_ptr<rodeos_filter>           filter         = {}; // todo: remove

   cloner_session(cloner_plugin_impl* my) : my(my), config(my->config) {
      // todo: remove
      if (!config->filter_wasm.empty())
         filter = std::make_unique<rodeos_filter>(config->filter_name, config->filter_wasm);
   }

   void connect(asio::io_context& ioc) {
      rodeos_snapshot.emplace(partition, true);

      ilog("cloner database status:");
      ilog("    revisions:    ${f} - ${r}",
           ("f", rodeos_snapshot->undo_stack->first_revision())("r", rodeos_snapshot->undo_stack->revision()));
      ilog("    chain:        ${a}", ("a", eosio::convert_to_json(rodeos_snapshot->chain_id)));
      ilog("    head:         ${a} ${b}",
           ("a", rodeos_snapshot->head)("b", eosio::convert_to_json(rodeos_snapshot->head_id)));
      ilog("    irreversible: ${a} ${b}",
           ("a", rodeos_snapshot->irreversible)(
                 "b", eosio::convert_to_json(rodeos_snapshot->irreversible_id)));

      rodeos_snapshot->end_write(true);
      db->flush(true, true);

      connection = std::make_shared<ship_client::connection>(ioc, *config, shared_from_this());
      connection->connect();
   }

   void received_abi() override {
      ilog("request status");
      connection->send(get_status_request_v0{});
   }

   bool received(get_status_result_v0& status, eosio::input_stream bin) override {
      ilog("nodeos has chain ${c}", ("c", eosio::convert_to_json(status.chain_id)));
      if (rodeos_snapshot->chain_id == eosio::checksum256{})
         rodeos_snapshot->chain_id = status.chain_id;
      if (rodeos_snapshot->chain_id != status.chain_id)
         throw std::runtime_error(
               "database is for chain " + eosio::convert_to_json(rodeos_snapshot->chain_id) +
               " but nodeos has chain " + eosio::convert_to_json(status.chain_id));
      ilog("request blocks");
      connection->request_blocks(status, std::max(config->skip_to, rodeos_snapshot->head + 1), get_positions(),
                                 ship_client::request_block | ship_client::request_traces |
                                       ship_client::request_deltas);
      return true;
   }

   std::vector<block_position> get_positions() {
      std::vector<block_position> result;
      if (rodeos_snapshot->head) {
         rodeos::db_view_state view_state{ rodeos::state_account, *db, *rodeos_snapshot->write_session,
                                           partition->contract_kv_prefix };
         for (uint32_t i = rodeos_snapshot->irreversible; i <= rodeos_snapshot->head; ++i) {
            auto info = rodeos::get_state_row<rodeos::block_info>(
                  view_state.kv_state.view, std::make_tuple(eosio::name{ "block.info" }, eosio::name{ "primary" }, i));
            if (!info)
               throw std::runtime_error("database is missing block.info for block " + std::to_string(i));
            auto& info0 = std::get<rodeos::block_info_v0>(info->second);
            result.push_back({ info0.num, info0.id });
         }
      }
      return result;
   }

   bool received(get_blocks_result_v0& result, eosio::input_stream bin) override {
      if (!result.this_block)
         return true;
      if (config->stop_before && result.this_block->block_num >= config->stop_before) {
         ilog("block ${b}: stop requested", ("b", result.this_block->block_num));
         rodeos_snapshot->end_write(true);
         db->flush(false, false);
         return false;
      }
      if (rodeos_snapshot->head && result.this_block->block_num > rodeos_snapshot->head + 1)
         throw std::runtime_error("state-history plugin is missing block " + std::to_string(rodeos_snapshot->head + 1));

      rodeos_snapshot->start_block(result);
      if (result.this_block->block_num <= rodeos_snapshot->head)
         reported_block = false;

      bool near      = result.this_block->block_num + 4 >= result.last_irreversible.block_num;
      bool write_now = !(result.this_block->block_num % 200) || near;
      if (write_now || !reported_block)
         ilog("block ${b} ${i}",
              ("b", result.this_block->block_num)(
                    "i", result.this_block->block_num <= result.last_irreversible.block_num ? "irreversible" : ""));
      reported_block = true;

      rodeos_snapshot->write_block_info(result);
      rodeos_snapshot->write_deltas(result, [] { return app().is_quiting(); });

      // todo: remove
      if (filter)
         filter->process(*rodeos_snapshot, result, bin, [](const char*, uint64_t) {});

      rodeos_snapshot->end_block(result, false);
      return true;
   } // receive_result()

   void closed(bool retry) override {
      if (my) {
         my->session.reset();
         if (retry)
            my->schedule_retry();
      }
   }

   ~cloner_session() {}
}; // cloner_session

static abstract_plugin& _cloner_plugin = app().register_plugin<cloner_plugin>();

cloner_plugin_impl::~cloner_plugin_impl() {
   if (session)
      session->my = nullptr;
}

void cloner_plugin_impl::start() {
   session = std::make_shared<cloner_session>(this);
   session->connect(app().get_io_service());
}

cloner_plugin::cloner_plugin() : my(std::make_shared<cloner_plugin_impl>()) {}

cloner_plugin::~cloner_plugin() {}

void cloner_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto op   = cfg.add_options();
   auto clop = cli.add_options();
   op("clone-connect-to,f", bpo::value<std::string>()->default_value("127.0.0.1:8080"),
      "State-history endpoint to connect to (nodeos)");
   clop("clone-skip-to,k", bpo::value<uint32_t>(), "Skip blocks before [arg]");
   clop("clone-stop,x", bpo::value<uint32_t>(), "Stop before block [arg]");
   // todo: remove
   op("filter-name", bpo::value<std::string>(), "Filter name");
   op("filter-wasm", bpo::value<std::string>(), "Filter wasm");
}

void cloner_plugin::plugin_initialize(const variables_map& options) {
   try {
      auto endpoint = options.at("clone-connect-to").as<std::string>();
      if (endpoint.find(':') == std::string::npos)
         throw std::runtime_error("invalid endpoint: " + endpoint);

      auto port               = endpoint.substr(endpoint.find(':') + 1, endpoint.size());
      auto host               = endpoint.substr(0, endpoint.find(':'));
      my->config->host        = host;
      my->config->port        = port;
      my->config->skip_to     = options.count("clone-skip-to") ? options["clone-skip-to"].as<uint32_t>() : 0;
      my->config->stop_before = options.count("clone-stop") ? options["clone-stop"].as<uint32_t>() : 0;
      if (options.count("filter-name") && options.count("filter-wasm")) {
         my->config->filter_name = eosio::name{ options["filter-name"].as<std::string>() };
         my->config->filter_wasm = options["filter-wasm"].as<std::string>();
      } else if (options.count("filter-name") || options.count("filter-wasm")) {
         throw std::runtime_error("filter-name and filter-wasm must be used together");
      }
   }
   FC_LOG_AND_RETHROW()
}

void cloner_plugin::plugin_startup() { my->start(); }

void cloner_plugin::plugin_shutdown() {
   if (my->session)
      my->session->connection->close(false);
   my->timer.cancel();
   ilog("cloner_plugin stopped");
}

} // namespace b1
