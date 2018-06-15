/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/wallet_plugin/wallet_api.hpp>
#include <boost/filesystem/path.hpp>
#include <chrono>

namespace fc { class variant; }

namespace eosio {
namespace wallet {

/// Provides associate of wallet name to wallet and manages the interaction with each wallet.
///
/// The name of the wallet is also used as part of the file name by soft_wallet. See wallet_manager::create.
/// No const methods because timeout may cause lock_all() to be called.
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

   /// Set the timeout for locking all wallets.
   /// If set then after t seconds of inactivity then lock_all().
   /// Activity is defined as any wallet_manager method call below.
   void set_timeout(const std::chrono::seconds& t);

   /// @see wallet_manager::set_timeout(const std::chrono::seconds& t)
   /// @param secs The timeout in seconds.
   void set_timeout(int64_t secs) { set_timeout(std::chrono::seconds(secs)); }
      
   /// Sign transaction with the private keys specified via their public keys.
   /// Use chain_controller::get_required_keys to determine which keys are needed for txn.
   /// @param txn the transaction to sign.
   /// @param keys the public keys of the corresponding private keys to sign the transaction with
   /// @param id the chain_id to sign transaction with.
   /// @return txn signed
   /// @throws fc::exception if corresponding private keys not found in unlocked wallets
   chain::signed_transaction sign_transaction(const chain::signed_transaction& txn, const flat_set<public_key_type>& keys,
                                             const chain::chain_id_type& id);


   /// Sign digest with the private keys specified via their public keys.
   /// @param digest the digest to sign.
   /// @param key the public key of the corresponding private key to sign the digest with
   /// @return signature over the digest
   /// @throws fc::exception if corresponding private keys not found in unlocked wallets
   chain::signature_type sign_digest(const chain::digest_type& digest, const public_key_type& key);

   /// Create a new wallet.
   /// A new wallet is created in file dir/{name}.wallet see set_dir.
   /// The new wallet is unlocked after creation.
   /// @param name of the wallet and name of the file without ext .wallet.
   /// @return Plaintext password that is needed to unlock wallet. Caller is responsible for saving password otherwise
   ///         they will not be able to unlock their wallet. Note user supplied passwords are not supported.
   /// @throws fc::exception if wallet with name already exists (or filename already exists)
   std::string create(const std::string& name);

   /// Open an existing wallet file dir/{name}.wallet.
   /// Note this does not unlock the wallet, see wallet_manager::unlock.
   /// @param name of the wallet file (minus ext .wallet) to open.
   /// @throws fc::exception if unable to find/open the wallet file.
   void open(const std::string& name);

   /// @return A list of wallet names with " *" appended if the wallet is unlocked.
   std::vector<std::string> list_wallets();

   /// @return A list of private keys from a wallet provided password is correct to said wallet
   map<public_key_type,private_key_type> list_keys(const string& name, const string& pw);

   /// @return A set of public keys from all unlocked wallets, use with chain_controller::get_required_keys.
   flat_set<public_key_type> get_public_keys();

   /// Locks all the unlocked wallets.
   void lock_all();

   /// Lock the specified wallet.
   /// No-op if wallet already locked.
   /// @param name the name of the wallet to lock.
   /// @throws fc::exception if wallet with name not found.
   void lock(const std::string& name);

   /// Unlock the specified wallet.
   /// The wallet remains unlocked until ::lock is called or program exit.
   /// @param name the name of the wallet to lock.
   /// @param password the plaintext password returned from ::create.
   /// @throws fc::exception if wallet not found or invalid password or already unlocked.
   void unlock(const std::string& name, const std::string& password);

   /// Import private key into specified wallet.
   /// Imports a WIF Private Key into specified wallet.
   /// Wallet must be opened and unlocked.
   /// @param name the name of the wallet to import into.
   /// @param wif_key the WIF Private Key to import, e.g. 5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3
   /// @throws fc::exception if wallet not found or locked.
   void import_key(const std::string& name, const std::string& wif_key);

   /// Removes a key from the specified wallet.
   /// Wallet must be opened and unlocked.
   /// @param name the name of the wallet to remove the key from.
   /// @param password the plaintext password returned from ::create.
   /// @param key the Public Key to remove, e.g. EOS6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV
   /// @throws fc::exception if wallet not found or locked or key is not removed.
   void remove_key(const std::string& name, const std::string& password, const std::string& key);

   /// Creates a key within the specified wallet.
   /// Wallet must be opened and unlocked
   /// @param name of the wallet to create key in
   /// @param type of key to create
   /// @throws fc::exception if wallet not found or locked, or if the wallet cannot create said type of key
   /// @return The public key of the created key
   string create_key(const std::string& name, const std::string& key_type);

private:
   /// Verify timeout has not occurred and reset timeout if not.
   /// Calls lock_all() if timeout has passed.
   void check_timeout();

private:
   using timepoint_t = std::chrono::time_point<std::chrono::system_clock>;
   std::map<std::string, std::unique_ptr<wallet_api>> wallets;
   std::chrono::seconds timeout = std::chrono::seconds::max(); ///< how long to wait before calling lock_all()
   mutable timepoint_t timeout_time = timepoint_t::max(); ///< when to call lock_all()
   boost::filesystem::path dir = ".";
};

} // namespace wallet
} // namespace eosio


