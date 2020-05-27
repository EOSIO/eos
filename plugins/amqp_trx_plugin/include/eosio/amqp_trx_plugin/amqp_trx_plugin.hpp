#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>

namespace eosio {

// consume message types
using transaction_msg = fc::static_variant<chain::packed_transaction_v0, chain::packed_transaction>;

// publish message types
struct transaction_trace_exception {
   int64_t error_code; ///< fc::exception code()
   std::string error_message;
};
using transaction_trace_msg = fc::static_variant<transaction_trace_exception, chain::transaction_trace>;

class amqp_trx_plugin : public appbase::plugin<amqp_trx_plugin> {

 public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   amqp_trx_plugin();
   virtual ~amqp_trx_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();
   void handle_sighup() override;

 private:
   std::shared_ptr<struct amqp_trx_plugin_impl> my;
};

} // namespace eosio

FC_REFLECT( eosio::transaction_trace_exception, (error_code)(error_message) );
