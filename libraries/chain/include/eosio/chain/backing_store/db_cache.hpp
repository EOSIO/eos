#pragma once

#include <b1/session/shared_bytes.hpp>

/// \brief Implement a cache storing (key, value) for db apis.
namespace eosio { namespace chain { namespace backing_store {
   /// \brief Insert or update (key, value) into the cache.
   /// \param key The key to be inserted/updated.
   /// \param value The value to be inserted/updated.
   /// \remarks If (key, value) is not in the cache, insert it; otherwise update the value.
   void db_cache_upsert(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value);

   /// \brief Find a key
   /// \param key The key to be searched.
   /// \remarks Return the value associated with the key if it is in the cache, otherwise return null.
   std::optional<eosio::session::shared_bytes> db_cache_find(const eosio::session::shared_bytes& key);

   /// \brief Check if a key is in the cache or not.
   /// \param key The key to be checked.
   /// \return true if the key is in the cache, false if not.
   bool db_cache_contains(const eosio::session::shared_bytes& key);

   /// \brief Remove a (key, value).
   /// \param key The key to be removed.
   /// \remarks If the key is not in cache, the function just returns.
   void db_cache_remove(const eosio::session::shared_bytes& key);

   /// \brief Clear the cache
   /// \remarks This is called when the cache is potentially stale, like when
   ///          db undo is performed and/or a new block starts.
   void db_cache_clear();
}}} // namespace eosio::chain::backing_store
