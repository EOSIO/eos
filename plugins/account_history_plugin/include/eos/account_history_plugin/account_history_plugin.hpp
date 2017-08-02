#pragma once
#include <appbase/application.hpp>
#include <eos/chain/chain_controller.hpp>

#include <eos/chain_plugin/chain_plugin.hpp>

namespace fc { class variant; }

namespace eos {
   using chain::chain_controller;
   using chain::transaction_id_type;
   using std::shared_ptr;
   using namespace appbase;
   using chain::Name;
   using fc::optional;
   using chain::uint128_t;

   typedef shared_ptr<class account_history_plugin_impl> account_history_ptr;
   typedef shared_ptr<const class account_history_plugin_impl> account_history_const_ptr;

namespace account_history_apis {
struct empty{};

class read_only {
   account_history_const_ptr account_history;

public:
   read_only(account_history_const_ptr&& account_history)
      : account_history(account_history) {}

   struct get_transaction_params {
      chain::transaction_id_type  transaction_id;
   };
   struct get_transaction_results {
      fc::variant    transaction;
   };

   get_transaction_results get_transaction(const get_transaction_params& params) const;

};

class read_write {
   account_history_ptr account_history;

public:
   read_write(account_history_ptr account_history) : account_history(account_history) {}
};
} // namespace account_history_apis

class account_history_plugin : public plugin<account_history_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES((chain_plugin))

   account_history_plugin();
   virtual ~account_history_plugin();

   virtual void set_program_options(options_description& cli, options_description& cfg) override;

   void plugin_initialize(const variables_map& options);
   void plugin_startup();
   void plugin_shutdown();

   account_history_apis::read_only get_read_only_api() const { return account_history_apis::read_only(account_history_const_ptr(my)); }
   account_history_apis::read_write get_read_write_api() { return account_history_apis::read_write(my); }

private:
   account_history_ptr my;
};

}

FC_REFLECT(eos::account_history_apis::empty, )
FC_REFLECT(eos::account_history_apis::read_only::get_transaction_params, (transaction_id) )
FC_REFLECT(eos::account_history_apis::read_only::get_transaction_results, (transaction) )
