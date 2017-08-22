#pragma once
#include <eos/chain/transaction.hpp>
#include <eos/wallet_plugin/wallet.hpp>
#include <boost/filesystem/path.hpp>

namespace fc { class variant; }

namespace eos {
namespace wallet {

/// Provides associate of wallet name to wallet and manages the interaction with each wallet.
///
/// The name of the wallet is also used as part of the file name by wallet_api. See wallet_manager#create.
class wallet_manager {
public:
   wallet_manager() = default;
   wallet_manager(const wallet_manager&) = delete;
   wallet_manager(wallet_manager&&) = delete;
   wallet_manager& operator=(const wallet_manager&) = delete;
   wallet_manager& operator=(wallet_manager&&) = delete;
   ~wallet_manager() = default;

   /// Set the path for location of wallet files.
   /// @param p path to override default ./ location of wallet files.
   void set_dir(const boost::filesystem::path& p) { dir = p; }

   /// Sign transaction with all the keys from all the unlocked wallets.
   /// @param txn the transaction to sign.
   /// @param id the chain_id to sign transaction with.
   /// @return txn signed
   /// @throws fc::exception if no unlocked wallets
   chain::SignedTransaction sign_transaction(const chain::SignedTransaction& txn, const chain::chain_id_type& id) const;

   /// Create a new wallet.
   /// A new wallet is created in file dir/{name}.wallet see wallet_manager#set_dir.
   /// The new wallet is unlocked after creation.
   /// @param name of the wallet and name of the file without ext .wallet.
   /// @return Plaintext password that is needed to unlock wallet. Caller is responsible for saving password otherwise
   ///         they will not be able to unlock their wallet. Note user supplied passwords are not supported.
   /// @throws fc::exception if wallet with name already exists (or filename already exists)
   std::string create(const std::string& name);

   /// Open an existing wallet file dir/{name}.wallet.
   /// Note this does not unlock the wallet, see wallet_manager#unlock.
   /// @param name of the wallet file (minus ext .wallet) to open.
   /// @throws fc::exception if unable to find/open the wallet file.
   void open(const std::string& name);

   /// @return A list of wallet names with " *" appended if the wallet is unlocked.
   std::vector<std::string> list_wallets() const;

   /// @return A list of unlocked wallet names and their private keys.
   ///         Wallet name entry followed by private key entries.
   std::vector<std::string> list_keys() const;

   /// Locks all the unlocked wallets.
   void lock_all();

   /// Lock the specified wallet.
   /// No-op if wallet already locked.
   /// @param name the name of the wallet to lock.
   /// @throws fc::exception if wallet with name not found.
   void lock(const std::string& name);

   /// Unlock the specified wallet.
   /// The wallet remains unlocked until wallet_manager#lock is called or program exit.
   /// @param name the name of the wallet to lock.
   /// @param password the plaintext password returned from wallet_manager#create.
   /// @throws fc::exception if wallet not found or invalid password.
   void unlock(const std::string& name, const std::string& password);

   /// Import private key into specified wallet.
   /// Imports a WIF Private Key into specified wallet.
   /// Wallet must be opened and unlocked.
   /// @param name the name of the wallet to import into.
   /// @param wif_key the WIF Private Key to import, e.g. 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
   /// @throws fc::exception if wallet not found or locked.
   void import_key(const std::string& name, const std::string& wif_key);

private:
   std::map<std::string, std::unique_ptr<wallet_api>> wallets;
   boost::filesystem::path dir = ".";
};

} // namespace wallet
} // namespace eos


