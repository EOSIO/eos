#include <eos/network_plugin/network_plugin.hpp>

#include <thread>
#include <algorithm>

namespace eosio {

class network_plugin_impl {
   public:
      std::unique_ptr<boost::asio::io_service> network_ios;
      std::unique_ptr<boost::asio::strand> strand;
      std::unique_ptr<boost::asio::io_service::work> network_work;
      std::vector<std::thread> thread_pool;
      unsigned int num_threads;

      void shutdown() {
        if(!network_ios)
          return;
        network_ios->stop();
        for(std::thread& t : thread_pool)
          t.join();
        network_ios.reset();
      }

      struct active_connection {
         active_connection(unsigned int cid, std::unique_ptr<connection_interface> c) :
            connection_id(cid), connection(std::move(c)) {}

         unsigned int connection_id;
         std::unique_ptr<connection_interface> connection;
      };
      std::list<active_connection> active_connections;
      unsigned int next_active_connection_id{0};
};

network_plugin::network_plugin()
   :my(new network_plugin_impl) {}

void network_plugin::set_program_options(options_description& cli, options_description& cfg) {
   cfg.add_options()
      ("network-threadpool-size", bpo::value<unsigned int>()->default_value(std::max(std::thread::hardware_concurrency()/4U, 1U)), "Number of threads to use for network operation.")
      ;
}

void network_plugin::plugin_initialize(const variables_map& options) {
   my->num_threads = options["network-threadpool-size"].as<unsigned int>();
   if(!my->num_threads)
      throw std::runtime_error("Must configure at least 1 network-threadpool-size");
   my->network_ios = std::make_unique<boost::asio::io_service>();
   my->strand = std::make_unique<boost::asio::strand>(*my->network_ios);
}

void network_plugin::plugin_startup() {
   my->network_work = std::make_unique<boost::asio::io_service::work>(*my->network_ios);
   for(unsigned int i = 0; i < my->num_threads; ++i)
      my->thread_pool.emplace_back([&ios=*my->network_ios]() {
         ios.run();
      });
}

boost::asio::io_service& network_plugin::network_io_service() {
   return *my->network_ios;
}

void network_plugin::new_connection(std::unique_ptr<connection_interface> connection) {
  //ugh, asio callbacks have to be copyable
  connection_interface* bare_connection = connection.release();
  my->strand->dispatch([this,bare_connection]() {
    unsigned int this_conn_id = my->next_active_connection_id++;

    my->active_connections.emplace_back(this_conn_id, std::unique_ptr<connection_interface>(bare_connection));

    network_plugin_impl::active_connection& added_connection = my->active_connections.back();

    //when the connection indicates failure; remove it from the list. This will implictly
    // destroy the connection. WARNING: use strand.post() here to prevent dtor getting
    // called which signal is still processing.
    added_connection.connection->on_disconnected([this, this_conn_id]() {
       my->strand->post([this, this_conn_id]() {
          my->active_connections.remove_if([this_conn_id](const network_plugin_impl::active_connection& ac) {
             return ac.connection_id == this_conn_id;
          });
       });
    });

    //there is a possiblity that the connection indicated failure before the signal was
    // connected. Check the synchronous indicatior.
    if(added_connection.connection->disconnected()) {
       my->strand->post([this, this_conn_id]() {
          my->active_connections.remove_if([this_conn_id](const network_plugin_impl::active_connection& ac) {
             return ac.connection_id == this_conn_id;
          });
       });
     }
  });
}

void network_plugin::indicate_about_to_shutdown() {
   my->shutdown();
}

void network_plugin::plugin_shutdown() {
   my->shutdown();
}

}
