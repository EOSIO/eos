/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>

namespace eosio {

using namespace appbase;

class pbft_plugin : public appbase::plugin<pbft_plugin> {
public:
   pbft_plugin();
   virtual ~pbft_plugin();
 
   APPBASE_PLUGIN_REQUIRES()
   virtual void set_program_options(options_description&, options_description& cfg) override;
 
   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

private:
   std::unique_ptr<class pbft_plugin_impl> my;
};

}
