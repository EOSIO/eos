/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#pragma once

#include <eosio/chain_plugin/chain_plugin.hpp>

#include <appbase/application.hpp>

namespace eosio {

class validator_plugin : public appbase::plugin<validator_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   validator_plugin();
   virtual ~validator_plugin();

   virtual void set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;

   virtual void plugin_initialize(const boost::program_options::variables_map& options);
   virtual void plugin_startup();
   virtual void plugin_shutdown();

private:
   std::unique_ptr<class validator_plugin_impl> my;
};

} //eosio
