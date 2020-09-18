#pragma once

#include <map>

// used for testing purposes
namespace eosio::chain_kv {
   class map_adapter {
      public:
         using db_t = std::map<shared_bytes, shared_bytes>;
         using iterator = db_t::iterator;

         map_adapter(const map_adapter&) = default;
         map_adapter(map_adapter&&)      = default;
         map_adapter(db_t& db) : db(&db) {}

         map_adapter& operator=(const map_adapter&) = default;
         map_adapter& operator=(map_adapter&&) = default;

         shared_bytes read(const shared_bytes& key) const {
            auto it = db->find(key);
            if ( it == db->end() )
               return shared_bytes::invalid();
            else
               return it->second;
         }
         inline bool has_parent() const { return false; }
         template <typename T>
         inline bool contains_local(T) const { return true; }
         void write(const shared_bytes& key, const shared_bytes& value) { db->insert_or_assign(key, value); }
         bool contains(const shared_bytes& key) const { return db->count(key) > 0; }
         void erase(const shared_bytes& key) { db->erase(key); }
         void clear() { db->clear(); }

         iterator       find(const shared_bytes& key) { return db->find(key); }
         iterator       begin() { return db->begin(); }
         iterator       end() { return db->end(); }
         iterator       lower_bound(const shared_bytes& key) { return db->lower_bound(key); }
         iterator       upper_bound(const shared_bytes& key) { return db->upper_bound(key); }

         void undo() {}
         void commit() {}
         void flush() {}
      private:
         db_t* db;
   };
} // namespace eosio::session
