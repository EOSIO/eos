#include <eosio/chain/backing_store/db_cache.hpp>

namespace eosio { namespace chain { namespace backing_store {

   //std::map<bytes, bytes> db_cache_store;
   ////std::unordered_map<eosio::session::shared_bytes, bytes> db_cache_store;
   static std::unordered_map<eosio::session::shared_bytes, eosio::session::shared_bytes> db_cache_store;

   ////void db_cache_insert(const eosio::session::shared_bytes& key, const char* data, std::size_t size) {
    void db_cache_upsert(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value) {
      //bytes k(key.size());
      //std::memcpy(k.data(), key.data(), key.size());

      ////bytes b(size);
      ////std::memcpy(b.data(), data, size);
      //db_cache_store[k] = b;
      ////db_cache_store[key] = b;
      db_cache_store[key] = value;
   }

   ////std::optional<bytes> db_cache_find(const eosio::session::shared_bytes& key) {
    std::optional<eosio::session::shared_bytes> db_cache_find(const eosio::session::shared_bytes& key) {
	   /*
      bytes k(key.size());
      std::memcpy(k.data(), key.data(), key.size());

      auto it = db_cache_store.find(k);
      */
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
	   /*
      bytes k(key.size());
      std::memcpy(k.data(), key.data(), key.size()); 
      db_cache_store.erase(k);
      */
      db_cache_store.erase(key);
   }

    void db_cache_clear() {
	   //ilog("====db_cache_store.size: ${s}", ("s", db_cache_store.size()));
      db_cache_store.clear();
   }
}}} // namespace eosio::chain::backing_store
