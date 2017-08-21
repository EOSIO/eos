#include <eos/wallet_plugin/wallet_plugin.hpp>
#include <eos/wallet_plugin/wallet_manager.hpp>
#include <boost/filesystem/path.hpp>

namespace fc { class variant; }

namespace eos {

wallet_plugin::wallet_plugin()
  : wallet_manager_ptr(new wallet_manager()) {
}

wallet_manager& wallet_plugin::get_wallet_manager() {
   return *wallet_manager_ptr;
}

void wallet_plugin::set_program_options(options_description& cli, options_description& cfg) {
   cli.add_options()
         ("wallet-dir", bpo::value<boost::filesystem::path>()->default_value("."),
          "The path of the wallet files (absolute path or relative to application data dir)")
         ;
}

void wallet_plugin::plugin_initialize(const variables_map& options) {
   ilog("initializing wallet plugin");

   if (options.count("wallet-dir")) {
      auto dir = options.at("wallet-dir").as<boost::filesystem::path>();
      if (dir.is_relative())
         wallet_manager_ptr->set_dir(app().data_dir() / dir);
      else
         wallet_manager_ptr->set_dir(dir);
   }
}
} // namespace eos
