#pragma once
#include <appbase/application.hpp>
#include <fc/variant.hpp>
#include <eos/types/native.hpp>
#include <eos/chain/types.hpp>
#include <eos/chain/transaction.hpp>

namespace fc { class variant; }

namespace eos {
   using namespace appbase;

   using wallet_ptr = std::shared_ptr<class wallet_plugin_impl>;
   using wallet_const_ptr = std::shared_ptr<const class wallet_plugin_impl>;

namespace wallet_apis {
struct empty{};

class read_only {
   wallet_const_ptr wallet;

public:
   read_only() = delete;
   read_only(const read_only&) = default;
   read_only(read_only&&) = default;
   explicit read_only(wallet_const_ptr&& wallet)
      : wallet(wallet) {}

};

class read_write {
   wallet_ptr wallet;

public:
   read_write() = delete;
   read_write(const read_write&) = default;
   read_write(read_write&&) = default;

   explicit read_write(wallet_ptr& wallet)
      : wallet(wallet) {}

   chain::SignedTransaction sign_transaction(const chain::Transaction& txn) {
      return {};
   }
};

} // namespace wallet_apis

class wallet_plugin : public plugin<wallet_plugin> {
public:
   APPBASE_PLUGIN_REQUIRES()

   wallet_plugin();
   virtual ~wallet_plugin() = default;

   virtual void set_program_options(options_description& cli, options_description& cfg) override {}
   void plugin_initialize(const variables_map& options) {}
   void plugin_startup() {}
   void plugin_shutdown() {}

   wallet_apis::read_only get_read_only_api() const {
      return wallet_apis::read_only(wallet_const_ptr(my));
   }

   wallet_apis::read_write get_read_write_api() {
      return wallet_apis::read_write(my);
   }

private:
   wallet_ptr my;
};

}

FC_REFLECT(eos::wallet_apis::empty, )
