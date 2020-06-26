#include <eosio/amqp_witness_plugin/amqp_witness_plugin.hpp>
#include <eosio/chain/types.hpp>

#include <eosio/reliable_amqp_publisher/reliable_amqp_publisher.hpp>

namespace eosio {
static appbase::abstract_plugin& _amqp_witness_plugin = app().register_plugin<amqp_witness_plugin>();

struct amqp_witness_plugin_impl {
   signature_provider_plugin::signature_provider_type signature_provider;
   unsigned staleness_limit = 600;
   std::string amqp_server = "amqp://";
   std::string exchange = "witness_signatures";
   std::optional<std::string> routing_key;
   bool delete_previous = false;

   std::unique_ptr<reliable_amqp_publisher> rqueue;
};

amqp_witness_plugin::amqp_witness_plugin() : my(new amqp_witness_plugin_impl()) {}

void amqp_witness_plugin::set_program_options(options_description& cli, options_description& cfg) {
   auto default_priv_key = eosio::chain::private_key_type::regenerate<fc::ecc::private_key_shim>(fc::sha256::hash(std::string("witness")));

   cfg.add_options()
         ("witness-signature-provider", boost::program_options::value<string>()->default_value(
               default_priv_key.get_public_key().to_string() + "=KEY:" + default_priv_key.to_string()),
               app().get_plugin<signature_provider_plugin>().signature_provider_help_text())
         ("witness-staleness-limit", boost::program_options::value<unsigned>()->default_value(my->staleness_limit)->notifier([this](const unsigned& v) {
                  my->staleness_limit = v;
               }), "Blocks older than this many seconds are not signed by the witness plugin")
         ("witness-amqp-server", boost::program_options::value<string>()->default_value(my->amqp_server)->notifier([this](const string& v) {
                  my->amqp_server = v;
               }), "AMQP server to send witness signatures in form of amqp://user:password@host:port/vhost")
         ("witness-amqp-exchange", boost::program_options::value<string>()->default_value(my->exchange)->notifier([this](const string& v) {
                  my->exchange = v;
               }), "Existing AMQP exchange to send witness signature messages to")
         ("witness-amqp-routing-key", boost::program_options::value<string>()->notifier([this](const string& v) {
                  my->routing_key = v;
               }), "AMQP routing key to set on published messages")
         ;

   cli.add_options()
         ("witness-delete-unsent", boost::program_options::bool_switch(&my->delete_previous), "Delete unsent witness signature messages retained from previous connections");
}

void amqp_witness_plugin::plugin_initialize(const variables_map& options) {
   try {
      const auto& [pubkey, provider] = app().get_plugin<signature_provider_plugin>().signature_provider_for_specification(options["witness-signature-provider"].as<std::string>());
      my->signature_provider = provider;
   }
   FC_LOG_AND_RETHROW()
}

void amqp_witness_plugin::plugin_startup() {
   const boost::filesystem::path witness_data_file_path = appbase::app().data_dir() / "amqp" / "witness.bin";

   if(boost::filesystem::exists(witness_data_file_path) && my->delete_previous)
      boost::filesystem::remove(witness_data_file_path);

   my->rqueue = std::make_unique<reliable_amqp_publisher>(my->amqp_server, my->exchange, my->routing_key ? *my->routing_key : "", witness_data_file_path, "eosio.node.witness_v0");

   app().get_plugin<chain_plugin>().chain().irreversible_block.connect([&](const chain::block_state_ptr& bsp) {
      if(bsp->block->timestamp.to_time_point() < fc::time_point::now() - fc::seconds(my->staleness_limit))
         return;

      my->rqueue->post_on_io_context([this, bsp]() {
         try {
            my->rqueue->publish_message(std::make_pair(bsp->header.action_mroot, my->signature_provider(bsp->header.action_mroot)));
         }
         FC_LOG_AND_DROP();
      });
   });
}

void amqp_witness_plugin::plugin_shutdown() {}

}
