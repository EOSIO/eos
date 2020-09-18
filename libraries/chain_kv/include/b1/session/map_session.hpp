#pragma once

#include <map>

#include <session/session.hpp>

// used for testing purposes
namespace eosio::session {
   template <>
   class session<std::map<shared_bytes, shared_bytes> {
      public:
         using db_t = std::map<shared_bytes, shared_bytes>;
         using iterator = db_t::iterator;

         session()               = default;
         session(const session&) = default;
         session(session&&)      = default;
         session(db_t& db);

      session& operator=(const session&) = default;
      session& operator=(session&&) = default;

      shared_bytes read(const shared_bytes& key) const {
         auto it = db.find(key);
         if ( it == db.end() )
            return shared_bytes::invalid;
         else
            return it->second;
      }
      void         write(const shared_bytes& key, const shared_bytes& value) { db.insert(key, value); }
      bool         contains(const shared_bytes& key) const { return db.count(key) > 0; }
      void         erase(const shared_bytes& key) { db.erase(key); }
      void         clear() { db.clear(); }

      iterator       find(const shared_bytes& key) { return db.find(key); }
      iterator       begin() { return db.begin(); }
      iterator       end() { return db.end(); }
      iterator       lower_bound(const shared_bytes& key) { return db.lower_bound(key); }
      iterator       upper_bound(const shared_bytes& key) { return db.upper_bound(key); }

      void undo() {}
      void commit() {}
      void flush() {}
      private:
         db_t* db;
   };
} // namespace eosio::session
