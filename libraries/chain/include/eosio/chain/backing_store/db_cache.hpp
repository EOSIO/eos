#pragma once

#include <b1/session/shared_bytes.hpp>
#include <b1/session/session.hpp>
#include <b1/session/session_variant.hpp>
#include <b1/session/rocks_session.hpp>

/// \brief Implement a cache storing (key, value) for db apis.
namespace eosio { namespace chain { namespace backing_store {
   void db_cache_insert(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value);

   void db_cache_update(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value);

   /// \brief Insert (key, value) into the cache.
   /// \param key The key to be inserted.
   /// \param value The value to be inserted.
   /// \remarks The key was not in the cache.
   void db_cache_insert(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value);

   /// \brief Update (key, value) in the cache.
   /// \param key The key associated with the value to be updated.
   /// \param value The value to be updated.
   /// \remarks The key was in the cache.
   void db_cache_update(const eosio::session::shared_bytes& key, const eosio::session::shared_bytes& value);

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

   using session_type = eosio::session::session<eosio::session::session<eosio::session::rocksdb_t>>;
   using session_variant_type = eosio::session::session_variant<session_type::parent_type, session_type>;
   /// \brief Commit current list of updated values to database.
   /// \param current_session The session.
   void db_cache_commit(session_variant_type& current_session);
}}} // namespace eosio::chain::backing_store
