#pragma once
#include <eos/chain/transaction.hpp>
#include <eos/wallet_plugin/wallet.hpp>
#include <boost/filesystem/path.hpp>

namespace fc { class variant; }

namespace eos {
namespace wallet {

class wallet_manager {
public:
   wallet_manager() = default;
   wallet_manager(const wallet_manager&) = delete;
   wallet_manager(wallet_manager&&) = delete;
   ~wallet_manager() = default;

   void set_dir(const boost::filesystem::path& p) { dir = p; }

   chain::SignedTransaction sign_transaction(const chain::SignedTransaction& txn) const;

   std::string create(const std::string& name);
   void open(const std::string& name);
   std::vector<std::string> list_wallets() const;
   std::vector<std::string> list_keys() const;
   void lock_all();
   void lock(const std::string& name);
   void unlock(const std::string& name, const std::string& password);
   void import_key(const std::string& name, const std::string& wif_key);

private:
   std::map<std::string, std::unique_ptr<wallet_api>> wallets;
   boost::filesystem::path dir = ".";
};

}
}


