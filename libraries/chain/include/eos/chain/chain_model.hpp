#pragma once

#include <chainbase/chainbase.hpp>
#include <eos/types/types.hpp>

namespace eos { namespace chain {

class account_object;
class producer_object;
using types::AccountName;

/**
 * @brief The chain_model class provides read-only access to blockchain state
 *
 * The raw chainbase::database API is a lower level of abstraction, dealing with indexes and revisions and undo
 * semantics, than clients to the blockchain ought to consume. This class wraps the database with a higher-level,
 * read-only API.
 *
 * @note This class has reference-semantics only. This reference does not imply ownership of the database, and will
 * dangle if it outlives the underlying database.
 */
class chain_model {
   const chainbase::database& db;

public:
   chain_model(const chainbase::database& db) : db(db) {}

   const account_object& get_account(const AccountName& name) const;
   const producer_object& get_producer(const AccountName& name) const;

   template<typename ObjectType, typename IndexedByType, typename CompatibleKey>
   const ObjectType* find(CompatibleKey&& key) const {
      return db.find<ObjectType, IndexedByType, CompatibleKey>(std::forward<CompatibleKey>(key));
   }

   template<typename ObjectType>
   const ObjectType* find(chainbase::oid<ObjectType> key = chainbase::oid<ObjectType>()) const { return db.find(key); }

   template<typename ObjectType, typename IndexedByType, typename CompatibleKey>
   const ObjectType& get(CompatibleKey&& key) const {
      return db.get<ObjectType, IndexedByType, CompatibleKey>(std::forward<CompatibleKey>(key));
   }

   template<typename ObjectType>
   const ObjectType& get(const chainbase::oid<ObjectType>& key = chainbase::oid<ObjectType>()) const {
      return db.get(key);
   }
};

} } // namespace eos::chain

