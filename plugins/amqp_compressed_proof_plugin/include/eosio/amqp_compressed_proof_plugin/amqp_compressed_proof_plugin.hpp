#pragma once
#include <appbase/application.hpp>

#include <eosio/chain_plugin/chain_plugin.hpp>

namespace eosio {

using namespace appbase;

class compressed_proof_generator {
public:
   using action_filter_func = std::function<bool(const chain::action& a)>;
   using merkle_proof_result_func = std::function<void(std::vector<char>&&)>;
   using result_callback_funcs = std::pair<action_filter_func, merkle_proof_result_func>;

   compressed_proof_generator(chain::controller& controller, std::vector<result_callback_funcs>&& callbacks);
   compressed_proof_generator(chain::controller& controller, std::vector<result_callback_funcs>&& callbacks, const boost::filesystem::path& reversible_path);
   ~compressed_proof_generator();

   compressed_proof_generator(const compressed_proof_generator&) = delete;
   compressed_proof_generator& operator=(const compressed_proof_generator&) = delete;

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
