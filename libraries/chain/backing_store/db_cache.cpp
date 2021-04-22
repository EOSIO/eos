#include <eosio/chain/backing_store/db_cache.hpp>

namespace eosio { namespace chain { namespace backing_store {

   static std::unordered_map<eosio::session::shared_bytes, eosio::session::shared_bytes> db_cache_store;

   // Use map so updated order is the same as the keys are stored
   static std::map<eosio::session::shared_bytes, eosio::session::shared_bytes> db_cache_updated;

   void db_cache_insert(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value) {
      db_cache_store[key] = value;
   }

   void db_cache_update(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value) {
      db_cache_store[key] = value;
      db_cache_updated[key] = value;
   }

   std::optional<eosio::session::shared_bytes> db_cache_find(const eosio::session::shared_bytes& key) {
      auto it = db_cache_store.find(key);
      if ( it != std::end(db_cache_store) ) {
         return it->second;
      } else {
         return {};
      }
   }

   bool db_cache_contains(const eosio::session::shared_bytes& key) {
      auto it = db_cache_store.find(key);
      if ( it != std::end(db_cache_store) ) {
         return true;
      } else {
	 return false;
      }
   }

   void db_cache_remove(const eosio::session::shared_bytes& key) {
      db_cache_store.erase(key);
      db_cache_updated.erase(key);
   }

   void db_cache_clear() {
      db_cache_store.clear();
   }

   void db_cache_commit(session_variant_type& current_session) {
      // batch write
      current_session.write(db_cache_updated);
      db_cache_updated.clear();
   }
}}} // namespace eosio::chain::backing_store
