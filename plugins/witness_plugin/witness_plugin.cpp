#include <eosio/witness_plugin/witness_plugin.hpp>
#include <eosio/chain/types.hpp>
#include <fc/log/logger_config.hpp> //set_os_thread_name()

namespace eosio {
static appbase::abstract_plugin& _witness_plugin = app().register_plugin<witness_plugin>();

struct witness_plugin_impl {
   signature_provider_plugin::signature_provider_type signature_provider;

   struct callback_entry {
      witness_plugin::witness_callback_func func;
      std::weak_ptr<void> weakptr;
   };

   std::list<callback_entry> callbacks;

   boost::asio::io_context ctx;
   boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard{ctx.get_executor()};
   std::thread thread;
};

witness_plugin::witness_plugin() : my(new witness_plugin_impl()) {}

void witness_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto default_priv_key = eosio::chain::private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("witness")));

   cfg.add_options()
         ("witness-signature-provider", boost::program_options::value<string>()->default_value(
               default_priv_key.get_public_key().to_string() + "=KEY:" + default_priv_key.to_string())->notifier([this](const string& v){
                  const auto& [pubkey, provider] = app().get_plugin<signature_provider_plugin>().signature_provider_for_specification(v);
                  my->signature_provider = provider;
               }),
               app().get_plugin<signature_provider_plugin>().signature_provider_help_text())
         ;
}

void witness_plugin::plugin_initialize(const variables_map& options) {}

void witness_plugin::plugin_startup() {
   my->thread = std::thread([this]() {
      fc::set_os_thread_name("witness");
      do {
         try {
            my->ctx.run();
            break;
         } FC_LOG_AND_DROP();
      } while(true);
   });


   app().get_plugin<chain_plugin>().chain().irreversible_block.connect([&](const chain::block_state_ptr& bsp) {
      std::list<std::pair<witness_plugin::witness_callback_func, std::shared_ptr<void>>> locks;
      auto it = my->callbacks.begin();
      while(it != my->callbacks.end()) {
         auto lock = it->weakptr.lock();
         if(!lock)
            it = my->callbacks.erase(it);
         else
            locks.emplace_back(std::make_pair(it++->func, lock));
      }

      my->ctx.post([this, bsp, locks]() {
         try {
            chain::signature_type mroot_sig = my->signature_provider(bsp->header.action_mroot);
            for(const auto& cb : locks)
               cb.first(bsp, mroot_sig);
         } FC_LOG_AND_DROP();
      });
   });
}

void witness_plugin::add_on_witness_sig(witness_callback_func&& func, std::weak_ptr<void> weak_ptr) {
   my->callbacks.emplace_back(witness_plugin_impl::callback_entry{std::move(func), std::move(weak_ptr)});
}

void witness_plugin::plugin_shutdown() {
   my->ctx.stop();
   my->thread.join();
}

}
