#pragma once
#include <appbase/application.hpp>
#include <fc/variant.hpp>
#include <eos/types/native.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/transaction.hpp>

namespace fc { class variant; }

namespace eos {
   using namespace appbase;

   class wallet_plugin_api_impl;

   using wallet_ptr = std::shared_ptr<class wallet_plugin_impl>;
   using wallet_const_ptr = std::shared_ptr<const class wallet_plugin_impl>;

class wallet_plugin : public plugin<wallet_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES()

   wallet_plugin();
   wallet_plugin(const wallet_plugin&) = delete;
   wallet_plugin(wallet_plugin&&) = delete;
   virtual ~wallet_plugin() = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override {}
   void plugin_initialize(const variables_map& options) {}
   void plugin_startup() {}
   void plugin_shutdown() {}

   // api interface
   wallet_plugin_api_impl& get

   chain::SignedTransaction sign_transaction(const chain::Transaction& txn) { return {}; }

private:
};

}

