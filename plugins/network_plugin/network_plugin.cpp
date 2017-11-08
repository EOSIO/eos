#include <eos/network_plugin/network_plugin.hpp>
#include <boost/asio/steady_timer.hpp>
#include <thread>
#include <algorithm>

namespace eosio {

class network_plugin_impl {
   public:
      std::unique_ptr<boost::asio::io_service> network_ios;
      std::unique_ptr<boost::asio::strand> strand;
      std::unique_ptr<boost::asio::io_service::work> network_work;
      std::unique_ptr<boost::asio::steady_timer> connection_work_timer;
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
            connection_id(cid), connection(std::move(c)), send_context(connection) {}

         unsigned int connection_id;
         std::unique_ptr<connection_interface> connection;
         connection_send_context send_context;
      };
      std::list<active_connection> active_connections;
      unsigned int next_active_connection_id{0};

      static const size_t transaction_buffer_size{64U << 10U};
      static_assert(!(transaction_buffer_size & (transaction_buffer_size-1)), "transaction_buffer_size must be power of two");
      vector<meta_transaction_ptr> _transaction_buffer{transaction_buffer_size};
      size_t _transaction_buffer_head{0};

      //broadcast a validated transaction to everyone
      void on_transaction_validated(meta_transaction_ptr trx) {
         strand->dispatch([this, t=move(trx)]() {
            _transaction_buffer[_transaction_buffer_head % transaction_buffer_size] = move(t);
            asm volatile("" ::: "memory");
            ++_transaction_buffer_head;
         });
      }

      connection_send_work get_work_to_send(connection_send_context& context) {
         //More checks here for higher priority queues; like blocks

         size_t buffer_head = _transaction_buffer_head;
         asm volatile("" ::: "memory");

         if(buffer_head == context.trx_tail)
            return connection_send_nowork();

         //Warning: here be grief. We pull from tail up to head transactions. This could mean up
         //         to transaction_buffer_size-1 transactions. But _transaction_buffer_head could
         //         continue to move while this code here is being executed potentially clobbering
         //         transactions that tail still points to. So... we'll actually only use up to
         //         transaction_buffer_size/2 to give a little wiggle room in allowing the connection
         //         to serialize the transactions before they have an opportunity to be globbered.
         //         This is probably overkill since the serialization happens almost immediately.

         if(buffer_head-context.trx_tail >= transaction_buffer_size/2)
            return connection_send_failure();

         connection_send_transactions send_transactions;
         if(buffer_head%transaction_buffer_size < context.trx_tail%transaction_buffer_size) {
            send_transactions.begin = _transaction_buffer.begin() + context.trx_tail%transaction_buffer_size;
            send_transactions.end   = send_transactions.begin + transaction_buffer_size-context.trx_tail%transaction_buffer_size;
            send_transactions.begin2 = _transaction_buffer.begin();
            send_transactions.end2   = send_transactions.begin2 + buffer_head%transaction_buffer_size;
         }
         else {
            send_transactions.begin = _transaction_buffer.begin()+context.trx_tail%transaction_buffer_size;
            send_transactions.end   = send_transactions.begin+(buffer_head-context.trx_tail);
            send_transactions.begin2 = send_transactions.end2 = _transaction_buffer.begin();
         }

         context.trx_tail = buffer_head;

         return send_transactions;
      }

      void work() {
         ///XXX from now or from exired?
         connection_work_timer->expires_from_now(std::chrono::milliseconds(10));
         connection_work_timer->async_wait(strand->wrap([this](auto ec){
            if(ec)
               throw std::runtime_error("Unexpected timer wait failure");
            for(network_plugin_impl::active_connection& c : active_connections)
               c.connection->begin_processing_network_send_queue(c.send_context);
            work();
         }));
      }
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

      //controller::transaction_validated.connect([this](auto a) {
      //    my->on_transaction_validated(a);
      //});
   my->connection_work_timer = std::make_unique<boost::asio::steady_timer>(*my->network_ios);
   my->work();
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

    ///XXX when cleaning this nonsense up, add this back---
    added_connection.send_context.trx_tail = my->_transaction_buffer_head;

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

connection_send_work network_plugin::get_work_to_send(connection_send_context& context) {
   return my->get_work_to_send(context);
}

void network_plugin::indicate_about_to_shutdown() {
   my->shutdown();
}

void network_plugin::plugin_shutdown() {
   my->shutdown();
}

}
