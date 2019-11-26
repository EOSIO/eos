#include <eosio/signed_validation_plugin/signed_validation_plugin.hpp>
#include <eosio/chain_plugin/chain_plugin.hpp>

#include <boost/beast/websocket.hpp>

#include <fc/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/exception/exception.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/io/json.hpp>
#include <fc/io/cfile.hpp>
#include <fc/log/logger_config.hpp>

namespace eosio {

static appbase::abstract_plugin& _signed_validation_plugin = app().register_plugin<signed_validation_plugin>();

using namespace eosio::chain;

struct signed_validation_plugin_impl {
   struct log_header_v1 {
      block_num_type first_block_num;
   };
   typedef fc::static_variant<log_header_v1> log_header;

   struct log_entry {
      block_id_type block;
      fc::crypto::signature signature;
   };

   struct ws_hello_message {
      block_num_type first_block_num;
      block_num_type last_block_num;
   };

   struct ws_streamed_validation_message {
      block_id_type block;
      fc::crypto::signature signature;
   };

   struct websocket_from_nodeos_message {
      fc::optional<ws_hello_message> hello;
      fc::optional<ws_streamed_validation_message> validated_block;
      fc::optional<std::vector<ws_streamed_validation_message>> blocks;
   };

   struct ws_request_blocks_message {
      block_num_type start;
      block_num_type end;
   };

   struct websocket_to_nodeos_message {
      ws_request_blocks_message request_blocks;
   };

   struct connection {
      signed_validation_plugin_impl& plugin_impl;

      ///XXX since we have plugin_impl consider removing these now
      boost::asio::io_context& io_context;
      boost::asio::ip::tcp::resolver& resolver;

      std::string host;
      std::string service;
      std::string path;

      boost::asio::deadline_timer connection_timer;
      unsigned retry_time = 0;

      std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws;

      std::deque<std::string> outgoing_buffers;
      boost::beast::flat_buffer incoming_buffer;

      unsigned ops_outstanding = 0;

      void reset_and_start_connect_again() {
         if(ops_outstanding) {
            ws->next_layer().close();
            return;
         }
         //always bounce this through the event loop to avoid dtoring objects during callbacks
         io_context.post([this]() {
            ws = std::make_unique<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>(io_context);
            outgoing_buffers.clear();
            retry_time = 1;
            schedule_connection_retry();
         });
      }

      void schedule_connection_retry() {
         retry_time += 2;
         retry_time = std::min(30u, retry_time);
         connection_timer.expires_from_now(boost::posix_time::seconds(retry_time));
         connection_timer.async_wait([this](auto ec) {
            if(ec)
               return;
            do_resolve();
         });
      }

      void do_resolve() {
         resolver.async_resolve(host, service, [this](auto ec, auto results) {
            if(ec) {
               schedule_connection_retry();
               return;
            }
            do_connect(results);
         });
      }

      void do_connect(const boost::asio::ip::tcp::resolver::results_type& resolved) {
         boost::asio::async_connect(ws->next_layer(), resolved, [this](auto ec, auto eptype) {
            if(ec) {
               schedule_connection_retry();
               return;
            }
            do_handshake();
         });
      }

      void do_read() {
         ++ops_outstanding;
         ws->async_read(incoming_buffer, [this](auto ec, size_t got) {
            --ops_outstanding;
            if(ec) {
               elog("READ ERROR");
               reset_and_start_connect_again();
               return;
            }

            auto d = boost::asio::buffer_cast<char const*>(boost::beast::buffers_front(incoming_buffer.data()));
            auto s = boost::asio::buffer_size(incoming_buffer.data());
            std::string str(d, s);
            incoming_buffer.consume(s);
            try {
               websocket_to_nodeos_message msg = fc::json::from_string(str).as<websocket_to_nodeos_message>();
               if(msg.request_blocks.start < plugin_impl.head_block_num)
                  throw std::runtime_error("start < head");
               if(msg.request_blocks.end > plugin_impl.tail_block_num)
                  throw std::runtime_error("end > tail");
               if(msg.request_blocks.start > msg.request_blocks.end)
                  throw std::runtime_error("start > end");
               if(msg.request_blocks.end - msg.request_blocks.start > 100)
                  throw std::runtime_error("too big");

               send_message({
                  fc::optional<ws_hello_message>(),
                  fc::optional<ws_streamed_validation_message>(),
                  plugin_impl.get_block_range(msg.request_blocks.start, msg.request_blocks.end)
               });
            }
            catch(...) {
               reset_and_start_connect_again();
            }

            do_read();
         });
      }

      void do_handshake() {
         ws->async_handshake(host, "/", [this](auto ec) {
            if(ec) {
               reset_and_start_connect_again();
               return;
            }
            say_hello();
            do_read();
         });
      }

      void say_hello() {
         websocket_from_nodeos_message msg{
            ws_hello_message {plugin_impl.head_block_num, plugin_impl.tail_block_num}
         };
         send_message(msg);
      }

      void send_message(const websocket_from_nodeos_message& msg) {
         outgoing_buffers.emplace_back(fc::json::to_string(msg));
         if (outgoing_buffers.size() == 1)
            do_write();
      }

      void do_write() {
         ++ops_outstanding;
         ws->async_write(boost::asio::buffer(outgoing_buffers.front()), [this](auto ec, size_t transfered) {
            --ops_outstanding;
            if(ec || transfered != outgoing_buffers.front().size()) {
               elog("WRITE ERROR");
               reset_and_start_connect_again();
               return;
            }
            outgoing_buffers.pop_front();
            if(outgoing_buffers.size())
               do_write();
         });
      }
   };

   chain_plugin* chain_plugin;

   fc::crypto::private_key signing_key;

   fc::cfile index_appender;
   fc::cfile index_floater;
   fc::cfile log_appender;
   fc::cfile log_floater;
   uint64_t next_log_insert_pos;

   block_num_type head_block_num;
   block_num_type tail_block_num;

   std::thread thread;
   boost::asio::io_context io_context;
   boost::asio::ip::tcp::resolver resolver{io_context};
   boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_context);

   std::vector<connection> connections;

   void sign_block(const block_id_type blockid) {
      log_entry entry = {blockid, signing_key.sign(blockid, false)};
      size_t entry_sz = fc::raw::pack_size(entry);
      char entry_buff[entry_sz];
      fc::raw::pack(entry_buff, entry_sz, entry);
      log_appender.write(entry_buff, entry_sz);

      websocket_from_nodeos_message msg {
              fc::optional<ws_hello_message>(),
              ws_streamed_validation_message{entry.block, entry.signature}
      };
      for(connection& c : connections)
         if(c.ws && c.ws->is_open())
            c.send_message(msg);

      index_appender.write((const char*)&next_log_insert_pos, sizeof(next_log_insert_pos));
      next_log_insert_pos += entry_sz;
   }

   std::vector<ws_streamed_validation_message> get_block_range(block_num_type start, block_num_type end) {
      std::vector<ws_streamed_validation_message> ret;
      unsigned sz = end-start+1;

      index_floater.seek((start - head_block_num) * sizeof(uint64_t));
      uint64_t offs;
      index_floater.read((char*)&offs, sizeof(offs));

      log_floater.seek(offs);
      uint64_t remaining = next_log_insert_pos - offs;
      uint64_t toread = std::min(remaining, UINT64_C(1024)*UINT64_C(1024));
      std::vector<char> buff(toread);
      log_floater.read(buff.data(), toread);

      fc::datastream<char*> ds(buff.data(), toread);
      do {
         log_entry entry;
         //XXX can we clean this up?
         fc::raw::unpack(ds, entry);
         ret.emplace_back(ws_streamed_validation_message{entry.block, entry.signature});
      } while(--sz);

      ///XXX assert_exceptions can blow everything up from here, why

      return ret;
   }
};

signed_validation_plugin::signed_validation_plugin() : my(std::make_unique<signed_validation_plugin_impl>()) {}
signed_validation_plugin::~signed_validation_plugin() = default;

void signed_validation_plugin::set_program_options(options_description& cli, options_description& cfg) {
   fc::crypto::private_key default_priv_key = private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("mrvalidate")));

   cfg.add_options()
      ("signed-validation-dir", bpo::value<bfs::path>()->default_value("signed-validation"),
       "the location of the validation log (absolute path or relative to application data dir)")
      ("signed-validation-private-key", bpo::value<std::string>()->default_value((std::string)default_priv_key),
       "the private key to use")
      ("signed-validation-address", bpo::value<std::vector<std::string>>(),
       "the websocket address to send signed validations to")
   ;

   cli.add_options()("delete-signed-validation-log", bpo::bool_switch(), "delete signed validation files");
}

void signed_validation_plugin::plugin_initialize(const variables_map& options) {
   my->signing_key = fc::crypto::private_key(options.at("signed-validation-private-key").as<string>());

   bfs::path dir = options.at("signed-validation-dir").as<bfs::path>();
   if(dir.is_relative())
      dir = app().data_dir() / dir;

   if(options.at("delete-signed-validation-log").as<bool>())
      boost::filesystem::remove_all(dir);
   boost::filesystem::create_directories(dir);

   my->index_appender.set_file_path(dir / "validation.index");
   my->index_floater.set_file_path(dir / "validation.index");
   my->log_appender.set_file_path(dir / "validation.log");
   my->log_floater.set_file_path(dir / "validation.log");

   my->index_appender.open("ab+");
   my->index_floater.open("rb");
   my->log_appender.open("ab+");
   my->log_floater.open("rb");

   ///XXX improve parsing here!
   if(options.count("signed-validation-address")) {
      for (const std::string &peer : options.at("signed-validation-address").as<std::vector<std::string>>()) {
         my->connections.emplace_back(signed_validation_plugin_impl::connection{
                 *my,
                 my->io_context,
                 my->resolver,
                 peer.substr(0, peer.find(':')),
                 peer.substr(peer.find(':') + 1, peer.size()),
                 "/",
                 boost::asio::deadline_timer{my->io_context}
         });
      }
   }
}

void signed_validation_plugin::plugin_startup() {
   my->chain_plugin = app().find_plugin<chain_plugin>();

   my->log_appender.seek_end(0);
   my->index_appender.seek_end(0);
   my->next_log_insert_pos = my->log_appender.tellp();

   if(my->log_appender.tellp() == 0) {
      //first time plugin is being activated
      signed_validation_plugin_impl::log_header_v1 the_log_header_v1;
      my->head_block_num = my->tail_block_num = the_log_header_v1.first_block_num = my->chain_plugin->chain().last_irreversible_block_num();
      signed_validation_plugin_impl::log_header the_log_header = the_log_header_v1;

      size_t header_sz = fc::raw::pack_size(the_log_header);
      char header_buff[header_sz];
      fc::raw::pack(header_buff, header_sz, the_log_header);
      my->log_appender.write(header_buff, header_sz);
      my->next_log_insert_pos += header_sz;

      //sign the LIB now for the first entry in to the log
      my->sign_block(my->chain_plugin->chain().last_irreversible_block_id());
   }
   else {
      char buff[512];
      signed_validation_plugin_impl::log_header the_log_header;
      my->log_floater.read(buff, fc::raw::pack_size(the_log_header));
      fc::raw::unpack(buff, sizeof(buff), the_log_header);
      EOS_ASSERT(the_log_header.contains<signed_validation_plugin_impl::log_header_v1>(),
                 plugin_exception, "validation log contains unknown header type ${w}", ("w", the_log_header.which()));
      my->head_block_num = the_log_header.get<signed_validation_plugin_impl::log_header_v1>().first_block_num;

      EOS_ASSERT(my->index_appender.tellp() % sizeof(uint64_t) == 0, plugin_exception,
                 "validation index is not an expected length");
      my->tail_block_num = my->head_block_num + my->index_appender.tellp() / 8 - 1;

      EOS_ASSERT(my->tail_block_num == my->chain_plugin->chain().last_irreversible_block_num(), plugin_exception,
                 "discontinuity in validation log would be created if enabling validation log; remove existing log via --delete-signed-validation-log");
   }

   wlog("validation log contains blocks ${a} to ${b}, insert @ ${i}", ("a", my->head_block_num)("b", my->tail_block_num)("i", my->next_log_insert_pos));

   app().find_plugin<chain_plugin>()->chain().irreversible_block.connect([&](const block_state_ptr& bsp) {
      my->io_context.post([id=bsp->block->id(),num=bsp->block_num,this]() {
         //swallow duplicate blocks, a known problem in the past with this signal
         if(my->tail_block_num == num)
            return;
         my->tail_block_num = num;
         my->sign_block(id);
      });
   });

   for(signed_validation_plugin_impl::connection& c : my->connections)
      c.reset_and_start_connect_again();

   my->thread = std::thread([this]() {
      fc::set_os_thread_name("sign-valid");
      my->io_context.run();
   });
}

void signed_validation_plugin::plugin_shutdown() {
   if(my->thread.joinable()) {
      my->io_context.post([this]() {
         my->io_context.stop();
      });
      my->thread.join();
   }
}

}

FC_REFLECT(eosio::signed_validation_plugin_impl::log_header_v1, (first_block_num));
FC_REFLECT(eosio::signed_validation_plugin_impl::log_entry, (block)(signature));
FC_REFLECT(eosio::signed_validation_plugin_impl::ws_hello_message, (first_block_num)(last_block_num));
FC_REFLECT(eosio::signed_validation_plugin_impl::ws_streamed_validation_message, (block)(signature));
//FC_REFLECT(eosio::signed_validation_plugin_impl::ws_block_reply_message, (blocks));
FC_REFLECT(eosio::signed_validation_plugin_impl::websocket_from_nodeos_message, (hello)(validated_block)(blocks));
FC_REFLECT(eosio::signed_validation_plugin_impl::ws_request_blocks_message, (start)(end));
FC_REFLECT(eosio::signed_validation_plugin_impl::websocket_to_nodeos_message, (request_blocks));