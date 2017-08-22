#include <eos/wallet_plugin/wallet_manager.hpp>
#include <eos/utilities/key_conversion.hpp>

namespace eos {
namespace wallet {

constexpr auto file_ext = ".wallet";
constexpr auto password_prefix = "PW";

std::string gen_password() {
   auto key = fc::ecc::private_key::generate();
   return password_prefix + utilities::key_to_wif(key);

}

std::string wallet_manager::create(const std::string& name) {
   std::string password = gen_password();

   auto wallet_filename = dir / (name + file_ext);
   if (fc::exists( dir / wallet_filename)) {
      FC_THROW("Wallet with name: ${n} already exists.", ("n", name));
   }

   wallet_data d;
   auto wallet = make_unique<wallet_api>(d);
   wallet->set_password(password);
   wallet->set_wallet_filename(wallet_filename.string());
   wallet->unlock(password);
   wallet->save_wallet_file();
   wallets.emplace(name, std::move(wallet));

   return password;
}

void wallet_manager::open(const std::string& name) {
   wallet_data d;
   auto wallet = std::make_unique<wallet_api>(d);
   auto wallet_filename = dir / (name + file_ext);
   wallet->set_wallet_filename(wallet_filename.string());
   if (!wallet->load_wallet_file()) {
      FC_THROW("Unable to open file: ${f}", ("f", wallet_filename.string()));
   }
   wallets.emplace(name, std::move(wallet));
}

std::vector<std::string> wallet_manager::list_wallets() const {
   std::vector<std::string> result;
   for (const auto& i : wallets) {
      if (i.second->is_locked()) {
         result.emplace_back(i.first);
      } else {
         result.emplace_back(i.first + " *");
      }
   }
   return result;
}

std::vector<std::string> wallet_manager::list_keys() const {
   std::vector<std::string> result;
   for (const auto& i : wallets) {
      if (!i.second->is_locked()) {
         result.emplace_back(i.first);
         const auto& keys = i.second->list_keys();
         for (const auto& i : keys) {
            result.emplace_back(i.second);
         }
      }
   }
   return result;
}

void wallet_manager::lock_all() {
   for (auto& i : wallets) {
      if (!i.second->is_locked()) {
         i.second->lock();
      }
   }
}

void wallet_manager::lock(const std::string& name) {
   if (wallets.count(name) == 0) {
      FC_THROW("Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (w->is_locked()) {
      return;
   }
   w->lock();
}

void wallet_manager::unlock(const std::string& name, const std::string& password) {
   if (wallets.count(name) == 0) {
      FC_THROW("Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (!w->is_locked()) {
      return;
   }
   w->unlock(password);
}

void wallet_manager::import_key(const std::string& name, const std::string& wif_key) {
   if (wallets.count(name) == 0) {
      FC_THROW("Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (w->is_locked()) {
      FC_THROW("Wallet is locked: ${w}", ("w", name));
   }
   w->import_key(wif_key);
}

chain::SignedTransaction
wallet_manager::sign_transaction(const chain::SignedTransaction& txn, const chain::chain_id_type& id) const {
   chain::SignedTransaction stxn(txn);

   size_t num_sigs = 0;
   for (const auto& i : wallets) {
      if (!i.second->is_locked()) {
         const auto& name = i.first;
         const auto& keys = i.second->list_keys();
         for (const auto& i : keys) {
            const optional<fc::ecc::private_key>& key = utilities::wif_to_key(i.second);
            if (!key) {
               FC_THROW("Invalid private key in wallet ${w}", ("w", name));
            }
            stxn.sign(*key, id);
            ++num_sigs;
         }
      }
   }
   if (num_sigs == 0) {
      FC_THROW("No unlocked wallets with keys, transaction not signed.");
   }

   return stxn;
}

} // namespace wallet
} // namespace eos
