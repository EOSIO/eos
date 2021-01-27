#pragma once
#include <eosio/chain_plugin/chain_plugin.hpp>
#include <appbase/application.hpp>

namespace eosio {

// publish message types
struct transaction_trace_exception {
   int64_t error_code; ///< fc::exception code()
   std::string error_message;
};
using transaction_trace_msg = std::variant<transaction_trace_exception, chain::transaction_trace>;

class amqp_trace_plugin : public appbase::plugin<amqp_trace_plugin> {

 public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   amqp_trace_plugin();
   virtual ~amqp_trace_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();
   void handle_sighup() override;

   // can be called from any thread
   void publish_error( std::string transaction_id, int64_t error_code, std::string error_message );

 private:
   std::shared_ptr<struct amqp_trace_plugin_impl> my;
};

} // namespace eosio

FC_REFLECT( eosio::transaction_trace_exception, (error_code)(error_message) );
