#pragma once
#include <appbase/application.hpp>

#include <eos/chain_plugin/chain_plugin.hpp>

#include <eos/chain/authority.hpp>
#include <eos/chain/types.hpp>

namespace eos {
using namespace appbase;
using chain::account_name;
using chain::Authority;

struct Transfer {
   account_name to;
   uint64_t     amount = 0;
   string       memo;
};

struct CreateAccount {
   account_name  new_account;
   Authority     owner;
   Authority     active;
   uint64_t      initial_balance = 0;
};

/**
 * @brief This class is a native C++ implementation of the system contract.
 */
class native_system_contract_plugin : public appbase::plugin<native_system_contract_plugin> {
public:
   native_system_contract_plugin();
   virtual ~native_system_contract_plugin();

   APPBASE_PLUGIN_REQUIRES((chain_plugin))
   virtual void set_program_options(options_description&, options_description&) override {}

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   /**
    * @brief Install the system contract implementation on the provided database
    *
    * Installs the native system contract on the provided database. This method is static, and may be used without a
    * native_system_contract_plugin or even AppBase. All that is required is a database to install the implementation
    * on.
    */
   static void install(chain::database& db);

private:
   std::unique_ptr<class native_system_contract_plugin_impl> my;
};

}

FC_REFLECT(eos::Transfer, (to)(amount)(memo))
FC_REFLECT(eos::CreateAccount, (new_account)(owner)(active)(initial_balance))
