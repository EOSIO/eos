/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#include <eosio/wallet_plugin/wallet_manager.hpp>
#include <eosio/wallet_plugin/wallet.hpp>
#include <eosio/wallet_plugin/se_wallet.hpp>
#include <eosio/chain/exceptions.hpp>
#include <boost/algorithm/string.hpp>
namespace eosio {
namespace wallet {

constexpr auto file_ext = ".wallet";
constexpr auto password_prefix = "PW";

std::string gen_password() {
   auto key = private_key_type::generate();
   return password_prefix + string(key);

}

bool valid_filename(const string& name) {
   if (name.empty()) return false;
   if (std::find_if(name.begin(), name.end(), !boost::algorithm::is_alnum() && !boost::algorithm::is_any_of("._-")) != name.end()) return false;
   return boost::filesystem::path(name).filename().string() == name;
}

wallet_manager::wallet_manager() {
#ifdef __APPLE__
   try {
      wallets.emplace("SecureEnclave", std::make_unique<se_wallet>());
   } catch(fc::exception& ) {}
#endif
}

wallet_manager::~wallet_manager() {
   //not really required, but may spook users
   if(wallet_dir_lock)
      boost::filesystem::remove(lock_path);
}

void wallet_manager::set_timeout(const std::chrono::seconds& t) {
   timeout = t;
   auto now = std::chrono::system_clock::now();
   timeout_time = now + timeout;
   EOS_ASSERT(timeout_time >= now && timeout_time.time_since_epoch().count() > 0, invalid_lock_timeout_exception, "Overflow on timeout_time, specified ${t}, now ${now}, timeout_time ${timeout_time}",
             ("t", t.count())("now", now.time_since_epoch().count())("timeout_time", timeout_time.time_since_epoch().count()));
}

void wallet_manager::check_timeout() {
   if (timeout_time != timepoint_t::max()) {
      const auto& now = std::chrono::system_clock::now();
      if (now >= timeout_time) {
         lock_all();
      }
      timeout_time = now + timeout;
   }
}

std::string wallet_manager::create(const std::string& name) {
   check_timeout();

   EOS_ASSERT(valid_filename(name), wallet_exception, "Invalid filename, path not allowed in wallet name ${n}", ("n", name));

   auto wallet_filename = dir / (name + file_ext);

   if (fc::exists(wallet_filename)) {
      EOS_THROW(chain::wallet_exist_exception, "Wallet with name: '${n}' already exists at ${path}", ("n", name)("path",fc::path(wallet_filename)));
   }

   std::string password = gen_password();
   wallet_data d;
   auto wallet = make_unique<soft_wallet>(d);
   wallet->set_password(password);
   wallet->set_wallet_filename(wallet_filename.string());
   wallet->unlock(password);
   wallet->lock();
   wallet->unlock(password);

   // Explicitly save the wallet file here, to ensure it now exists.
   wallet->save_wallet_file();

   // If we have name in our map then remove it since we want the emplace below to replace.
   // This can happen if the wallet file is removed while eos-walletd is running.
   auto it = wallets.find(name);
   if (it != wallets.end()) {
      wallets.erase(it);
   }
   wallets.emplace(name, std::move(wallet));

   return password;
}

void wallet_manager::open(const std::string& name) {
   check_timeout();

   EOS_ASSERT(valid_filename(name), wallet_exception, "Invalid filename, path not allowed in wallet name ${n}", ("n", name));

   wallet_data d;
   auto wallet = std::make_unique<soft_wallet>(d);
   auto wallet_filename = dir / (name + file_ext);
   wallet->set_wallet_filename(wallet_filename.string());
   if (!wallet->load_wallet_file()) {
      EOS_THROW(chain::wallet_nonexistent_exception, "Unable to open file: ${f}", ("f", wallet_filename.string()));
   }

   // If we have name in our map then remove it since we want the emplace below to replace.
   // This can happen if the wallet file is added while eos-walletd is running.
   auto it = wallets.find(name);
   if (it != wallets.end()) {
      wallets.erase(it);
   }
   wallets.emplace(name, std::move(wallet));
}

std::vector<std::string> wallet_manager::list_wallets() {
   check_timeout();
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

map<public_key_type,private_key_type> wallet_manager::list_keys(const string& name, const string& pw) {
   check_timeout();

   if (wallets.count(name) == 0)
      EOS_THROW(chain::wallet_nonexistent_exception, "Wallet not found: ${w}", ("w", name));
   auto& w = wallets.at(name);
   if (w->is_locked())
      EOS_THROW(chain::wallet_locked_exception, "Wallet is locked: ${w}", ("w", name));
   w->check_password(pw); //throws if bad password
   return w->list_keys();
}

flat_set<public_key_type> wallet_manager::get_public_keys() {
   check_timeout();
   EOS_ASSERT(!wallets.empty(), wallet_not_available_exception, "You don't have any wallet!");
   flat_set<public_key_type> result;
   bool is_all_wallet_locked = true;
   for (const auto& i : wallets) {
      if (!i.second->is_locked()) {
         result.merge(i.second->list_public_keys());
      }
      is_all_wallet_locked &= i.second->is_locked();
   }
   EOS_ASSERT(!is_all_wallet_locked, wallet_locked_exception, "You don't have any unlocked wallet!");
   return result;
}


void wallet_manager::lock_all() {
   // no call to check_timeout since we are locking all anyway
   for (auto& i : wallets) {
      if (!i.second->is_locked()) {
         i.second->lock();
      }
   }
}

void wallet_manager::lock(const std::string& name) {
   check_timeout();
   if (wallets.count(name) == 0) {
      EOS_THROW(chain::wallet_nonexistent_exception, "Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (w->is_locked()) {
      return;
   }
   w->lock();
}

void wallet_manager::unlock(const std::string& name, const std::string& password) {
   check_timeout();
   if (wallets.count(name) == 0) {
      open( name );
   }
   auto& w = wallets.at(name);
   if (!w->is_locked()) {
      EOS_THROW(chain::wallet_unlocked_exception, "Wallet is already unlocked: ${w}", ("w", name));
      return;
   }
   w->unlock(password);
}

void wallet_manager::import_key(const std::string& name, const std::string& wif_key) {
   check_timeout();
   if (wallets.count(name) == 0) {
      EOS_THROW(chain::wallet_nonexistent_exception, "Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (w->is_locked()) {
      EOS_THROW(chain::wallet_locked_exception, "Wallet is locked: ${w}", ("w", name));
   }
   w->import_key(wif_key);
}

void wallet_manager::remove_key(const std::string& name, const std::string& password, const std::string& key) {
   check_timeout();
   if (wallets.count(name) == 0) {
      EOS_THROW(chain::wallet_nonexistent_exception, "Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (w->is_locked()) {
      EOS_THROW(chain::wallet_locked_exception, "Wallet is locked: ${w}", ("w", name));
   }
   w->check_password(password); //throws if bad password
   w->remove_key(key);
}

string wallet_manager::create_key(const std::string& name, const std::string& key_type) {
   check_timeout();
   if (wallets.count(name) == 0) {
      EOS_THROW(chain::wallet_nonexistent_exception, "Wallet not found: ${w}", ("w", name));
   }
   auto& w = wallets.at(name);
   if (w->is_locked()) {
      EOS_THROW(chain::wallet_locked_exception, "Wallet is locked: ${w}", ("w", name));
   }

   string upper_key_type = boost::to_upper_copy<std::string>(key_type);
   return w->create_key(upper_key_type);
}

chain::signed_transaction
wallet_manager::sign_transaction(const chain::signed_transaction& txn, const flat_set<public_key_type>& keys, const chain::chain_id_type& id) {
   check_timeout();
   chain::signed_transaction stxn(txn);

   for (const auto& pk : keys) {
      bool found = false;
      for (const auto& i : wallets) {
         if (!i.second->is_locked()) {
            optional<signature_type> sig = i.second->try_sign_digest(stxn.sig_digest(id, stxn.context_free_data), pk);
            if (sig) {
               stxn.signatures.push_back(*sig);
               found = true;
               break; // inner for
            }
         }
      }
      if (!found) {
         EOS_THROW(chain::wallet_missing_pub_key_exception, "Public key not found in unlocked wallets ${k}", ("k", pk));
      }
   }

   return stxn;
}

chain::signature_type
wallet_manager::sign_digest(const chain::digest_type& digest, const public_key_type& key) {
   check_timeout();

   try {
      for (const auto& i : wallets) {
         if (!i.second->is_locked()) {
            optional<signature_type> sig = i.second->try_sign_digest(digest, key);
            if (sig)
               return *sig;
         }
      }
   } FC_LOG_AND_RETHROW();

   EOS_THROW(chain::wallet_missing_pub_key_exception, "Public key not found in unlocked wallets ${k}", ("k", key));
}

void wallet_manager::own_and_use_wallet(const string& name, std::unique_ptr<wallet_api>&& wallet) {
   if(wallets.find(name) != wallets.end())
      FC_THROW("tried to use wallet name the already existed");
   wallets.emplace(name, std::move(wallet));
}

void wallet_manager::initialize_lock() {
   //This is technically somewhat racy in here -- if multiple keosd are in this function at once.
   //I've considered that an acceptable tradeoff to maintain cross-platform boost constructs here
   lock_path = dir / "wallet.lock";
   {
      std::ofstream x(lock_path.string());
      EOS_ASSERT(!x.fail(), wallet_exception, "Failed to open wallet lock file at ${f}", ("f", lock_path.string()));
   }
   wallet_dir_lock = std::make_unique<boost::interprocess::file_lock>(lock_path.string().c_str());
   if(!wallet_dir_lock->try_lock()) {
      wallet_dir_lock.reset();
      EOS_THROW(wallet_exception, "Failed to lock access to wallet directory; is another keosd running?");
   }
}

} // namespace wallet
} // namespace eosio
