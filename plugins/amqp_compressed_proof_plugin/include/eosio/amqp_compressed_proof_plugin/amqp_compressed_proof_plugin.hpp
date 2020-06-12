#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class compressed_proof_generator {
public:
   compressed_proof_generator(chain::controller& controller);
   compressed_proof_generator(chain::controller& controller, const boost::filesystem::path& reversible_path);
   ~compressed_proof_generator();

   compressed_proof_generator(const compressed_proof_generator&) = delete;
   compressed_proof_generator& operator=(const compressed_proof_generator&) = delete;

   using action_filter_func = std::function<bool(const chain::action& a)>;
   using merkle_proof_result_func = std::function<void(std::vector<char>&&)>;
   void add_result_callback(action_filter_func&& filter, merkle_proof_result_func&& result);
private:
   std::unique_ptr<struct compressed_proof_generator_impl> my;
};

class amqp_compressed_proof_plugin : public appbase::plugin<amqp_compressed_proof_plugin> {
public:
   amqp_compressed_proof_plugin();
   virtual ~amqp_compressed_proof_plugin();

   APPBASE_PLUGIN_REQUIRES((chain_plugin))
   virtual void set_program_options(options_description&, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<struct amqp_compressed_proof_plugin_impl> my;
};

}
