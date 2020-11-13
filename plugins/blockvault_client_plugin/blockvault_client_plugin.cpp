#include "block_vault_impl.hpp"
#include "zlib_compressor.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <eosio/blockvault_client_plugin/blockvault_client_plugin.hpp> // eosio::blockvault_client_plugin
#include <fc/log/log_message.hpp>                                      // FC_LOG_MESSAGE
#include <vector>                                                      // std::vector
#if HAS_PQXX
#include "postgres_backend.hpp"
#endif

namespace eosio {
static appbase::abstract_plugin& _blockvault_client_plugin = app().register_plugin<blockvault_client_plugin>();

using vault_impl = eosio::blockvault::block_vault_impl<eosio::blockvault::zlib_compressor>;
class blockvault_client_plugin_impl : public vault_impl {
 public:
   blockvault_client_plugin_impl(std::unique_ptr<blockvault::backend>&& be)
       : vault_impl(std::move(be)) {}
};

blockvault_client_plugin::blockvault_client_plugin() {}

blockvault_client_plugin::~blockvault_client_plugin() {
}

void blockvault_client_plugin::set_program_options(options_description&, options_description& cfg) {
#ifdef HAS_PQXX
   cfg.add_options()("block-vault-backend", bpo::value<std::string>(),
                     "the url for block vault backend. Currently, only PostgresQL is supported, the format is "
                     "'postgresql://username:password@localhost/company'");
#endif
}

void blockvault_client_plugin::plugin_initialize(const variables_map& options) {
#ifdef HAS_PQXX
   if (options.count("block-vault-backend")) {
      std::string url = options["block-vault-backend"].as<std::string>();
      if (boost::starts_with(url, "postgresql://")) {
         my.reset(new blockvault_client_plugin_impl(std::make_unique<blockvault::postgres_backend>(url)));
      } else if (url.size()) {
         elog("unknown block-vault-backend option, skipping it");
      }
   }
#endif
}

void blockvault_client_plugin::plugin_startup() {
   if (my.get())
      my->start();
}

void blockvault_client_plugin::plugin_shutdown() {
   if (my.get())
      my->stop();
}

eosio::blockvault::block_vault_interface* blockvault_client_plugin::get() { return my.get(); }

} // namespace eosio
