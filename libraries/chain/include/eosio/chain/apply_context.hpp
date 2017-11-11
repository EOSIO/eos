/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/record_functions.hpp>

namespace chainbase { class database; }

namespace eosio { namespace chain {

class chain_controller;

class apply_context {
   public:
      apply_context(chain_controller& con,
                    chainbase::database& db,
                    const transaction& t,
                    const action& a,
                    const account_name& recv )
      :controller(con), db(db), trx(t), act(a), receiver(recv), mutable_controller(con),
       mutable_db(db), used_authorizations(act.authorization.size(), false){}

      template <typename ObjectType>
      int32_t store_record( name scope, name code, name table, typename ObjectType::key_type* keys, char* value, uint32_t valuelen ) {
         require_scope( scope );

         auto tuple = find_tuple<ObjectType>::get(scope, code, table, keys);
         const auto* obj = db.find<ObjectType, by_scope_primary>(tuple);

         if( obj ) {
            mutable_db.modify( *obj, [&]( auto& o ) {
               o.value.assign(value, valuelen);
            });
            return 0;
         } else {
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
      int32_t update_record( name scope, name code, name table, typename ObjectType::key_type *keys, char* value, uint32_t valuelen ) {
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
      int32_t remove_record( name scope, name code, name table, typename ObjectType::key_type* keys, char* value, uint32_t valuelen ) {
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
      int32_t load_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
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
      int32_t front_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
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
      int32_t back_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
         require_scope( scope );

         const auto& idx = db.get_index<IndexType, Scope>();
         auto tuple = boost::make_tuple( account_name(scope), account_name(code), account_name(uint64_t(table)+1) );
         auto itr = idx.lower_bound(tuple);

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
      int32_t next_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
         require_scope( scope );

         const auto& pidx = db.get_index<IndexType, by_scope_primary>();
         
         auto tuple = next_record_tuple<typename IndexType::value_type>::get(scope, code, table, keys);
         auto pitr = pidx.find(tuple);

         if(pitr == pidx.end())
           return -1;

         const auto& fidx = db.get_index<IndexType>();
         auto itr = fidx.indicies().template project<Scope>(pitr);

         const auto& idx = db.get_index<IndexType, Scope>();

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
      int32_t previous_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
         require_scope( scope );

         const auto& pidx = db.get_index<IndexType, by_scope_primary>();
         
         auto tuple = next_record_tuple<typename IndexType::value_type>::get(scope, code, table, keys);
         auto pitr = pidx.find(tuple);

         if(pitr == pidx.end())
           return -1;

         const auto& fidx = db.get_index<IndexType>();
         auto itr = fidx.indicies().template project<Scope>(pitr);

         const auto& idx = db.get_index<IndexType, Scope>();
         
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
      int32_t lower_bound_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
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
      int32_t upper_bound_record( name scope, name code, name table, typename IndexType::value_type::key_type* keys, char* value, uint32_t valuelen ) {
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
      void require_authorization(const account_name& account){};
      void require_authorization(const account_name& account, const permission_name& permission){};
      void require_scope(const account_name& account)const{};
      void require_recipient(const account_name& account){};

      bool                     all_authorizations_used()const;
      vector<permission_level> unused_authorizations()const;

      void get_active_producers(account_name* producers, uint32_t len);

      const chain_controller&       controller;
      const chainbase::database&    db;  ///< database where state is stored
      const transaction&            trx; ///< used to gather the valid read/write scopes
      const action&                 act; ///< message being applied
      account_name                  receiver; ///< the code that is currently running

      chain_controller&             mutable_controller;
      chainbase::database&          mutable_db;

      std::deque<account_name>      notified;
      std::vector<action>           inline_actions; ///< queued inline messages
      std::vector<transaction>      deferred_transactions; ///< deferred txs /// TODO specify when

      ///< Parallel to act.authorization; tracks which permissions have been used while processing the message
      vector<bool> used_authorizations;

      ///< pending transaction construction
     /*
      typedef uint32_t pending_transaction_handle;
      struct pending_transaction : public transaction {
         typedef uint32_t handle_type;
         
         pending_transaction(const handle_type& _handle, const apply_context& _context, const uint16_t& block_num, const uint32_t& block_ref, const time_point& expiration )
            : transaction(block_num, block_ref, expiration, vector<account_name>(),  vector<account_name>(), vector<action>())
            , handle(_handle)
            , context(_context) {}
         
         
         handle_type handle;
         const apply_context& context;

         void check_size() const;
      };

      pending_transaction::handle_type next_pending_transaction_serial;
      vector<pending_transaction> pending_transactions;

      pending_transaction& get_pending_transaction(pending_transaction::handle_type handle);
      pending_transaction& create_pending_transaction();
      void release_pending_transaction(pending_transaction::handle_type handle);

      ///< pending message construction
      typedef uint32_t pending_message_handle;
      struct pending_message : public action {
         typedef uint32_t handle_type;
         
         pending_message(const handle_type& _handle, const account_name& code, const action_name& type, const vector<char>& data)
            : action(code, type, vector<permission_level>(), data)
            , handle(_handle) {}

         handle_type handle;
      };

      pending_transaction::handle_type next_pending_message_serial;
      vector<pending_message> pending_messages;

      pending_message& get_pending_message(pending_message::handle_type handle);
      pending_message& create_pending_message(const account_name& code, const action_name& type, const vector<char>& data);
      void release_pending_message(pending_message::handle_type handle);
      */
};

using apply_handler = std::function<void(apply_context&)>;

} } // namespace eosio::chain
