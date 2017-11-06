/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <appbase/application.hpp>
#include <fc/variant.hpp>
#include <eos/types/native.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/transaction.hpp>

namespace fc { class variant; }

namespace eosio {
   using namespace appbase;

   namespace wallet {
      class wallet_manager;
   }
   using namespace wallet;

class wallet_plugin : public plugin<wallet_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES()

   wallet_plugin();
   wallet_plugin(const wallet_plugin&) = delete;
   wallet_plugin(wallet_plugin&&) = delete;
   wallet_plugin& operator=(const wallet_plugin&) = delete;
   wallet_plugin& operator=(wallet_plugin&&) = delete;
   virtual ~wallet_plugin() override = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override;
   void plugin_initialize(const variables_map& options);
   void plugin_startup() {}
   void plugin_shutdown() {}

   // api interface provider
   wallet_manager& get_wallet_manager();

private:
   std::unique_ptr<wallet_manager> wallet_manager_ptr;
};

}

