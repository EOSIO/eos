#include <boost/algorithm/string/predicate.hpp>
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp>
#include <eosio/blockvault_client_plugin/detail/blockvault_impl.hpp>
#include <eosio/blockvault_client_plugin/detail/zlib_compressor.hpp>
#include <fc/log/log_message.hpp>
#if HAS_PQXX
#include <eosio/blockvault_client_plugin/detail/postgres_backend.hpp>
#endif

namespace eosio {

static appbase::abstract_plugin& _blockvault_client_plugin = app().register_plugin<blockvault_client_plugin>();

using vault_impl = eosio::blockvault::block_vault_impl<eosio::blockvault::zlib_compressor>;
class blockvault_client_plugin_impl : public vault_impl {
   public:
      blockvault_client_plugin_impl(std::unique_ptr<blockvault::backend>&& be)
      : vault_impl(std::move(be))
      {}
};

blockvault_client_plugin::blockvault_client_plugin() {}

blockvault_client_plugin::~blockvault_client_plugin() {}

void blockvault_client_plugin::set_program_options(options_description&, options_description& cfg) {
#ifdef HAS_PQXX
   cfg.add_options()
         ("block-vault-backend", bpo::value<std::string>(),
          "the uri for block vault backend. Currently, only PostgresQL is supported, the format is "
          "'postgresql://username:password@localhost/company'");
#endif
}

void blockvault_client_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing blockvault_client plugin");
#ifdef HAS_PQXX
   try {
      if (options.count("block-vault-backend")) {
         std::string uri = options["block-vault-backend"].as<std::string>();
         if (boost::starts_with(uri, "postgresql://") || boost::starts_with(uri, "postgres://")) {
            my.reset(new blockvault_client_plugin_impl(std::make_unique<blockvault::postgres_backend>(uri)));
            ilog("blockvault_client plugin started");
         } else if (uri.size()) {
            elog("unknown block-vault-backend option, skipping it");
         }
      }
   }
   FC_RETHROW_EXCEPTIONS ( error, "blockvault_client plugin initialization error")
#endif
}

void blockvault_client_plugin::plugin_startup() {
   if (my.get()) {
      my->start();
   }
}

void blockvault_client_plugin::plugin_shutdown() {
   if (my.get()) {
      my->stop();
   }
}

eosio::blockvault::block_vault_interface* blockvault_client_plugin::get() {
   return my.get();
}

} // namespace eosio
