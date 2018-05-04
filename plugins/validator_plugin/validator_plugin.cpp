/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio/validator_plugin/validator_plugin.hpp>
#include <eosio/chain/plugin_interface.hpp>
#include <fc/scoped_exit.hpp>

using std::string;
using std::vector;

namespace eosio {

static appbase::abstract_plugin& _producer_plugin = app().register_plugin<validator_plugin>();

using namespace eosio::chain;
using namespace eosio::chain::plugin_interface;

class validator_plugin_impl {
   public:
      validator_plugin_impl()
      {}

      boost::program_options::variables_map _options;

      int32_t                                                   _max_deferred_transaction_time_ms;
      int32_t                                                   _max_pending_transaction_time_ms;

      channels::incoming_block::channel_type::handle            _incoming_block_subscription;
      channels::incoming_transaction::channel_type::handle      _incoming_transaction_subscription;

      methods::incoming_block_sync::method_type::handle         _incoming_block_sync_provider;
      methods::incoming_transaction_sync::method_type::handle   _incoming_transaction_sync_provider;
      methods::start_coordinator::method_type::handle           _start_coordinator_provider;

      void start_block();

      void on_incoming_block(const signed_block_ptr& block) {
         chain::controller& chain = app().get_plugin<chain_plugin>().chain();
         // abort the pending block
         chain.abort_block();

         // exceptions throw out, make sure we restart our loop
         auto ensure = fc::make_scoped_exit([this](){
            // restart a block
            start_block();
         });

         // push the new block
         chain.push_block(block);
      }

      transaction_trace_ptr on_incoming_transaction(const packed_transaction_ptr& trx) {
         chain::controller& chain = app().get_plugin<chain_plugin>().chain();
         return chain.sync_push(std::make_shared<transaction_metadata>(*trx), fc::time_point::now() + fc::milliseconds(_max_pending_transaction_time_ms));
      }
};

validator_plugin::validator_plugin()
   : my(new validator_plugin_impl()){
   }

validator_plugin::~validator_plugin() {}

void validator_plugin::set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   config_file_options.add_options()
         ("validator-max-pending-transaction-time", bpo::value<int32_t>()->default_value(30),
          "Limits the maximum time (in milliseconds) that is allowed a pushed transaction's code to execute before being considered invalid")
         ("validator-max-deferred-transaction-time", bpo::value<int32_t>()->default_value(20),
          "Limits the maximum time (in milliseconds) that is allowed a to push deferred transactions at the start of a block")
         ;
}

void validator_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   my->_max_deferred_transaction_time_ms = options.at("validator-max-deferred-transaction-time").as<int32_t>();
   my->_max_pending_transaction_time_ms = options.at("validator-max-pending-transaction-time").as<int32_t>();


   my->_incoming_block_subscription = app().get_channel<channels::incoming_block>().subscribe([this](const signed_block_ptr& block){
      try {
         my->on_incoming_block(block);
      } FC_LOG_AND_DROP();
   });

   my->_incoming_transaction_subscription = app().get_channel<channels::incoming_transaction>().subscribe([this](const packed_transaction_ptr& trx){
      try {
         my->on_incoming_transaction(trx);
      } FC_LOG_AND_DROP();
   });

   // this is a low priority default plugin
   static const int my_priority = 1000;

   my->_incoming_block_sync_provider = app().get_method<methods::incoming_block_sync>().register_provider([this](const signed_block_ptr& block){
      my->on_incoming_block(block);
   }, my_priority);

   my->_incoming_transaction_sync_provider = app().get_method<methods::incoming_transaction_sync>().register_provider([this](const packed_transaction_ptr& trx) -> transaction_trace_ptr {
      return my->on_incoming_transaction(trx);
   }, my_priority);


   my->_start_coordinator_provider = app().get_method<methods::start_coordinator>().register_provider([this]() {
      ilog("Launching validating node.");
      my->start_block();
   }, my_priority);


} FC_LOG_AND_RETHROW() }

void validator_plugin::plugin_startup()
{
}

void validator_plugin::plugin_shutdown() {

}

void validator_plugin_impl::start_block() {
   chain::controller& chain = app().get_plugin<chain_plugin>().chain();
   chain.abort_block();
   chain.start_block();
   // TODO:  BIG BAD WARNING, THIS WILL HAPPILY BLOW PAST DEADLINES BUT CONTROLLER IS NOT YET SAFE FOR DEADLINE USAGE
   try {
      while (chain.push_next_unapplied_transaction(fc::time_point::maximum()));
   } FC_LOG_AND_DROP();

   try {
      while (chain.push_next_scheduled_transaction(fc::time_point::maximum()));
   } FC_LOG_AND_DROP();

}

} // namespace eosio
