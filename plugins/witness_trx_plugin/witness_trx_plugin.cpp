#include <eosio/witness_trx_plugin/witness_trx_plugin.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <fc/io/cfile.hpp>
#include <fc/log/logger_config.hpp> //set_os_thread_name()

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

using namespace boost::multi_index;

namespace eosio {
static appbase::abstract_plugin& _witness_trx_plugin = app().register_plugin<witness_trx_plugin>();

struct witness_trx_plugin_impl {
   name witness_account = N(witness);
   signature_provider_plugin::signature_provider_type signature_provider;
   bool delete_previous = false;

   chain_plugin& chainplug = app().get_plugin<eosio::chain_plugin>();

   boost::filesystem::path trx_witness_path;

   chain::time_point last_known_head_time;
   chain::block_id_type lib_block_id;
   std::optional<chain::chain_id_type> chain_id;
   std::list<std::vector<char>> sig_action_data_waiting_on_catch_up;

   std::thread thread;
   boost::asio::io_context ctx;
   boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard{ctx.get_executor()};
   boost::asio::io_context::strand from_witness_plugin_strand{ctx};

   struct outstanding_witness_transaction {
      outstanding_witness_transaction(const chain::digest_type& trx_id, const chain::time_point& trx_expires, std::list<std::vector<char>>&& action_data) :
        trx_id(trx_id), trx_expires(trx_expires), action_data(std::move(action_data)) {}
      chain::digest_type trx_id;
      chain::time_point trx_expires;
      std::list<std::vector<char>> action_data;
   };

   struct by_trx_id;
   struct by_trx_expiration;

   typedef multi_index_container<
      outstanding_witness_transaction,
      indexed_by<
         hashed_unique<tag<by_trx_id>, key<&outstanding_witness_transaction::trx_id>, std::hash<chain::transaction_id_type>>,
         ordered_non_unique<tag<by_trx_expiration>, key<&outstanding_witness_transaction::trx_expires>>
      >
   > outstanding_witness_transaction_index_type;
   outstanding_witness_transaction_index_type outstanding_witness_transaction_index;

   bool caught_up() const {
      return last_known_head_time >= fc::time_point::now() - fc::seconds(5);
   }

   void on_accepted_block(const chain::block_state_ptr& bsp) {
      ctx.post([this, this_block_time=bsp->block->timestamp]() {
         if(this_block_time > last_known_head_time)
            last_known_head_time = this_block_time;

         if(!sig_action_data_waiting_on_catch_up.empty() && caught_up()) {
            while(!sig_action_data_waiting_on_catch_up.empty()) {
               std::list<std::vector<char>> batch;
               constexpr size_t max_batch_size = 1000u;
               auto it = sig_action_data_waiting_on_catch_up.begin();
               std::advance(it, std::min(max_batch_size, sig_action_data_waiting_on_catch_up.size()));
               batch.splice(batch.begin(), sig_action_data_waiting_on_catch_up, sig_action_data_waiting_on_catch_up.begin(), it);
               submit_witness_trx(std::move(batch));
            }
         }

         auto it = outstanding_witness_transaction_index.get<by_trx_expiration>().begin();
         while(it != outstanding_witness_transaction_index.get<by_trx_expiration>().end() && it->trx_expires < chain::time_point::now()) {
            wlog("Witness transaction not confirmed by producers; resubmitting");
            outstanding_witness_transaction_index.get<by_trx_expiration>().modify(it, [this](witness_trx_plugin_impl::outstanding_witness_transaction& row) {
               sig_action_data_waiting_on_catch_up.splice(sig_action_data_waiting_on_catch_up.end(), row.action_data);
            });
            it = outstanding_witness_transaction_index.get<by_trx_expiration>().erase(it);
         }
      });
   }

   void on_irreversible_block(const chain::block_state_ptr& bsp) {
      ctx.post([this, bsp]() {
         lib_block_id = bsp->id;

         for(const chain::transaction_receipt& trx : bsp->block->transactions) {
            if(trx.status != chain::transaction_receipt_header::executed)
               continue;
            if(!trx.trx.contains<chain::packed_transaction>())
               continue;
            if(auto it = outstanding_witness_transaction_index.find(trx.trx.get<chain::packed_transaction>().id()); it != outstanding_witness_transaction_index.end())
               outstanding_witness_transaction_index.erase(it);
         }
      });
   }

   void submit_witness_trx(std::list<std::vector<char>>&& action_datas) {
      chain::signed_transaction trx;
      for(const auto& action_data : action_datas)
         trx.actions.emplace_back(vector<chain::permission_level>{{witness_account,N(active)}}, witness_account, N(witness), action_data);
      trx.set_reference_block(lib_block_id);
      trx.expiration = last_known_head_time + fc::seconds(30);

      try {
         trx.signatures.emplace_back(signature_provider(trx.sig_digest(*chain_id, trx.context_free_data)));
         outstanding_witness_transaction_index.emplace(trx.id(), trx.expiration + fc::seconds(600), std::move(action_datas));
      }
      catch(const fc::exception& e) {
         wlog("Failed to sign witness transaction. Will retry shortly. ${w}", ("w", e));
         outstanding_witness_transaction_index.emplace(trx.id(), chain::time_point::now() + fc::seconds(10), std::move(action_datas));
         return;
      }

      app().post(priority::medium_low, [this, t=std::move(trx)]() {
         chainplug.accept_transaction(std::make_shared<chain::packed_transaction>(chain::signed_transaction(t), true), [](const fc::static_variant<fc::exception_ptr, chain::transaction_trace_ptr>& result) {
            if(result.contains<chain::transaction_trace_ptr>())
               return;
            if(result.get<fc::exception_ptr>()->code() == chain::tx_duplicate::code_value)
               return;
            wlog("Failed to submit witness trx. Will retry later ${e}", ("e", result.get<fc::exception_ptr>()));
         });
      });
   }

   ~witness_trx_plugin_impl() {
      if(thread.joinable()) {
         //drain any remaining items being delivered from witness_plugin
         std::promise<void> shutdown_promise;
         boost::asio::post(from_witness_plugin_strand, [&]() {
            shutdown_promise.set_value();
         });
         shutdown_promise.get_future().wait();

         ctx.stop();
         thread.join();
      }

      //splice() all the action datas from the outstanding_witness_transaction_index items over to the sig_action_data_waiting_on_catch_up list
      // to have a single location to write out everything
      //This may cause actions in outstanding_witness_transaction_index to be submitted again in a new trx on restart; but that's okay there
      // is no current requirement that witness sigs be presented once and only once
      if(sig_action_data_waiting_on_catch_up.empty() && outstanding_witness_transaction_index.empty())
         return;

      for(auto it = outstanding_witness_transaction_index.begin(); it != outstanding_witness_transaction_index.end(); ++it) {
         outstanding_witness_transaction_index.modify(it, [this](auto& row) {
            sig_action_data_waiting_on_catch_up.splice(sig_action_data_waiting_on_catch_up.begin(), row.action_data);
         });
      }

      try {
         fc::datastream<fc::cfile> file;
         file.set_file_path(trx_witness_path);
         file.open("wb");
         fc::raw::pack(file, sig_action_data_waiting_on_catch_up);
      } FC_LOG_AND_DROP();
   }
};

witness_trx_plugin::witness_trx_plugin() : my(new witness_trx_plugin_impl()) {}

void witness_trx_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto default_priv_key = eosio::chain::private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("witness account")));

   cfg.add_options()
         ("witness-account", boost::program_options::value<string>()->default_value(my->witness_account.to_string())->notifier([this](const string& v) {
                  my->witness_account = eosio::name(v);
               }), "EOSIO account to submit witness transactions to")
         ("witness-trx-signature-provider", boost::program_options::value<string>()->default_value(
               default_priv_key.get_public_key().to_string() + "=KEY:" + default_priv_key.to_string())->notifier([this](const string& v){
                  const auto& [pubkey, provider] = app().get_plugin<signature_provider_plugin>().signature_provider_for_specification(v);
                  my->signature_provider = provider;
               }),
               app().get_plugin<signature_provider_plugin>().signature_provider_help_text())
         ;

   cli.add_options()
         ("witness-trx-delete-unsent", boost::program_options::bool_switch(&my->delete_previous), "Delete unsent witness signature transactions");
}

void witness_trx_plugin::plugin_initialize(const variables_map& options) {
   my->trx_witness_path = appbase::app().data_dir() / "trx_witness.bin";

   const auto chain_read_mode = my->chainplug.chain().get_read_mode();
   EOS_ASSERT(chain_read_mode == chain::db_read_mode::HEAD || chain_read_mode == chain::db_read_mode::SPECULATIVE, chain::plugin_config_exception,
              "witness_trx_plugin requires head or speculative chain mode");
}

void witness_trx_plugin::plugin_startup() {
   app().get_plugin<witness_plugin>().add_on_witness_sig([my=my.get()](const chain::block_state_ptr& bsp, const chain::signature_type& sig) {
      boost::asio::post(my->from_witness_plugin_strand, [my, bsp, sig]() {
         std::vector<char> witness_action_data = fc::raw::pack(std::make_pair(bsp->header.action_mroot, sig));

         if(my->caught_up())
            my->submit_witness_trx({witness_action_data});
         else
            my->sig_action_data_waiting_on_catch_up.emplace_back(std::move(witness_action_data));
      });
   }, std::weak_ptr<witness_trx_plugin_impl>(my));

   my->chainplug.chain().accepted_block.connect([this](const chain::block_state_ptr& bsp) {
      my->on_accepted_block(bsp);
   });

   my->chainplug.chain().irreversible_block.connect([this](const chain::block_state_ptr& bsp) {
      my->on_irreversible_block(bsp);
   });

   my->last_known_head_time = my->chainplug.chain().head_block_time();
   my->lib_block_id = my->chainplug.chain().last_irreversible_block_id();
   my->chain_id = my->chainplug.chain().get_chain_id();

   if(boost::filesystem::exists(my->trx_witness_path) && !my->delete_previous) {
      try {
         fc::datastream<fc::cfile> file;
         file.set_file_path(my->trx_witness_path);
         file.open("rb");
         fc::raw::unpack(file, my->sig_action_data_waiting_on_catch_up);
      } FC_RETHROW_EXCEPTIONS(error, "Failed to load outstanding witness transactions from ${f}", ("f", (fc::path)my->trx_witness_path));
   }
   if(true) {
      boost::filesystem::ofstream o(my->trx_witness_path);
      FC_ASSERT(o.good(), "Failed to create outstanding witness transaction file at ${f}", ("f", (fc::path)my->trx_witness_path));
   }
   boost::system::error_code ec;
   boost::filesystem::remove(my->trx_witness_path, ec);

   my->thread = std::thread([this]() {
      fc::set_os_thread_name("witnesstrx");
      do {
         try {
            my->ctx.run();
            break;
         } FC_LOG_AND_DROP();
      } while(true);
   });
}

void witness_trx_plugin::plugin_shutdown() {}

}
