#pragma once

#include <eos/chain/message.hpp>
#include <eos/chain/transaction.hpp>
#include <eos/types/types.hpp>
#include <eos/chain/record_functions.hpp>

namespace chainbase { class database; }

namespace eos { namespace chain {

class chain_controller;

class apply_context {
public:
   apply_context(chain_controller& con,
                 chainbase::database& db,
                 const chain::Transaction& t,
                 const chain::Message& m,
                 const types::AccountName& code)
      : controller(con), db(db), trx(t), msg(m), code(code), mutable_controller(con),
        mutable_db(db), used_authorizations(msg.authorization.size(), false){}

   template <typename ObjectType>
   int32_t store_record( Name scope, Name code, Name table, typename ObjectType::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      auto tuple = find_tuple<ObjectType>::get(scope, code, table, keys);
      const auto* obj = db.find<ObjectType, by_scope_primary>(tuple);

      if( obj ) {
         //wlog( "modify" );
         mutable_db.modify( *obj, [&]( auto& o ) {
            o.value.assign(value, valuelen);
         });
         return 0;
      } else {
         //wlog( "new" );
         mutable_db.create<ObjectType>( [&](auto& o) {
            o.scope = scope;
            o.code  = code;
            o.table = table;
            key_helper<ObjectType>::set(o, keys);
            o.value.insert( 0, value, valuelen );
         });
         return 1;
      }
   }

   template <typename ObjectType>
   int32_t update_record( Name scope, Name code, Name table, typename ObjectType::key_type *keys, char* value, uint32_t valuelen ) {
      require_scope( scope );
      
      auto tuple = find_tuple<ObjectType>::get(scope, code, table, keys);
      const auto* obj = db.find<ObjectType, by_scope_primary>(tuple);

      if( !obj ) {
         return 0;
      }

      mutable_db.modify( *obj, [&]( auto& o ) {
         if( valuelen > o.value.size() ) {
            o.value.resize(valuelen);
         }
         memcpy(o.value.data(), value, valuelen);
      });

      return 1;
   }

   template <typename ObjectType>
   int32_t remove_record( Name scope, Name code, Name table, typename ObjectType::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      auto tuple = find_tuple<ObjectType>::get(scope, code, table, keys);
      const auto* obj = db.find<ObjectType, by_scope_primary>(tuple);
      if( obj ) {
         mutable_db.remove( *obj );
         return 1;
      }
      return 0;
   }

   template <typename IndexType, typename Scope>
   int32_t load_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = load_record_tuple<typename IndexType::value_type, Scope>::get(scope, code, table, keys);
      auto itr = idx.lower_bound(tuple);

      if( itr == idx.end() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ||
          !load_record_compare<typename IndexType::value_type, Scope>::compare(*itr, keys)) return -1;

       key_helper<typename IndexType::value_type>::set(keys, *itr);

       auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
       if( copylen ) {
          itr->value.copy(value, copylen);
       }
       return copylen;
   }

   template <typename IndexType, typename Scope>
   int32_t front_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = front_record_tuple<typename IndexType::value_type>::get(scope, code, table);

      auto itr = idx.lower_bound( tuple );
      if( itr == idx.end() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ) return -1;

      key_helper<typename IndexType::value_type>::set(keys, *itr);

      auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
      if( copylen ) {
         itr->value.copy(value, copylen);
      }
      return copylen;
   }

   template <typename IndexType, typename Scope>
   int32_t back_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = back_record_tuple<typename IndexType::value_type>::get(scope, code, table);
      auto itr = idx.upper_bound(tuple);

      if( std::distance(idx.begin(), itr) == 0 ) return -1;

      --itr;

      if( itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ) return -1;

      key_helper<typename IndexType::value_type>::set(keys, *itr);

      auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
      if( copylen ) {
         itr->value.copy(value, copylen);
      }
      return copylen;
   }

   template <typename IndexType, typename Scope>
   int32_t next_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = next_record_tuple<typename IndexType::value_type, Scope>::get(scope, code, table, keys);

      auto itr = idx.lower_bound(tuple);

      if( itr == idx.end() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ||
          !key_helper<typename IndexType::value_type>::compare(*itr, keys) ) { 
        return -1;
      }

      ++itr;

      if( itr == idx.end() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ) { 
        return -1;
      }

      key_helper<typename IndexType::value_type>::set(keys, *itr);
      
      auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
      if( copylen ) {
         itr->value.copy(value, copylen);
      }
      return copylen;
   }

   template <typename IndexType, typename Scope>
   int32_t previous_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = next_record_tuple<typename IndexType::value_type, Scope>::get(scope, code, table, keys);
      auto itr = idx.lower_bound(tuple);
      
      if( itr == idx.end() ||
          itr == idx.begin() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ||
          !key_helper<typename IndexType::value_type>::compare(*itr, keys) ) return -1;

      --itr;

      if( itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ) return -1;

      key_helper<typename IndexType::value_type>::set(keys, *itr);

      auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
      if( copylen ) {
         itr->value.copy(value, copylen);
      }
      return copylen;
   }

   template <typename IndexType, typename Scope>
   int32_t lower_bound_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = lower_bound_tuple<typename IndexType::value_type, Scope>::get(scope, code, table, keys);
      auto itr = idx.lower_bound(tuple);

      if( itr == idx.end() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table) return -1;

      key_helper<typename IndexType::value_type>::set(keys, *itr);

      auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
      if( copylen ) {
         itr->value.copy(value, copylen);
      }
      return copylen;
   }

   template <typename IndexType, typename Scope>
   int32_t upper_bound_record( Name scope, Name code, Name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
      require_scope( scope );

      const auto& idx = db.get_index<IndexType, Scope>();
      auto tuple = upper_bound_tuple<typename IndexType::value_type, Scope>::get(scope, code, table, keys);
      auto itr = idx.upper_bound(tuple);

      if( itr == idx.end() ||
          itr->scope != scope ||
          itr->code  != code  ||
          itr->table != table ) return -1;

      key_helper<typename IndexType::value_type>::set(keys, *itr);

      auto copylen =  std::min<size_t>(itr->value.size(),valuelen);
      if( copylen ) {
         itr->value.copy(value, copylen);
      }
      return copylen;
   }

   /**
    * @brief Require @ref account to have approved of this message
    * @param account The account whose approval is required
    *
    * This method will check that @ref account is listed in the message's declared authorizations, and marks the
    * authorization as used. Note that all authorizations on a message must be used, or the message is invalid.
    *
    * @throws tx_missing_auth If no sufficient permission was found
    */
   void require_authorization(const types::AccountName& account);
   void require_scope(const types::AccountName& account)const;
   void require_recipient(const types::AccountName& account);

   bool all_authorizations_used() const;
   vector<types::AccountPermission> unused_authorizations() const;

   const chain_controller&      controller;
   const chainbase::database&   db;  ///< database where state is stored
   const chain::Transaction&    trx; ///< used to gather the valid read/write scopes
   const chain::Message&        msg; ///< message being applied
   types::AccountName           code; ///< the code that is currently running

   chain_controller&    mutable_controller;
   chainbase::database& mutable_db;

   std::deque<AccountName>          notified;
   std::deque<ProcessedTransaction> sync_transactions; ///< sync calls made
   std::deque<GeneratedTransaction> async_transactions; ///< async calls requested

   ///< Parallel to msg.authorization; tracks which permissions have been used while processing the message
   vector<bool> used_authorizations;
};

using apply_handler = std::function<void(apply_context&)>;

} } // namespace eos::chain
