/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <chainbase/chainbase.hpp>
#include <eosio/chain/types.hpp>

namespace std {

   template<>
   struct hash<eosio::chain::account_name>
   {
    size_t operator()( const eosio::chain::account_name& name )const
    {
        return (uint64_t)name;
    }
   };
}

namespace eosio {
using chain::account_name;
using chain::shared_vector;
using chain::transaction_id_type;
using namespace boost::multi_index;

class account_transaction_history_object : public chainbase::object<chain::account_transaction_history_object_type, account_transaction_history_object> {
   OBJECT_CTOR(account_transaction_history_object)

   id_type                            id;
   account_name                       account_name;
   transaction_id_type                transaction_id;
};

struct by_id;
struct by_account_name;
struct by_account_name_trx_id;
using account_transaction_history_multi_index = chainbase::shared_multi_index_container<
   account_transaction_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_transaction_history_object, account_transaction_history_object::id_type, id)>,
      hashed_non_unique<tag<by_account_name>, BOOST_MULTI_INDEX_MEMBER(account_transaction_history_object, account_name, account_name), std::hash<account_name>>,
      hashed_unique<tag<by_account_name_trx_id>,
         composite_key< account_transaction_history_object,
            member<account_transaction_history_object, account_name, &account_transaction_history_object::account_name>,
            member<account_transaction_history_object, transaction_id_type, &account_transaction_history_object::transaction_id>
         >,
         composite_key_hash< std::hash<account_name>, std::hash<transaction_id_type> >
      >
   >
>;

typedef chainbase::generic_index<account_transaction_history_multi_index> account_transaction_history_index;

}

CHAINBASE_SET_INDEX_TYPE( eosio::account_transaction_history_object, eosio::account_transaction_history_multi_index )

FC_REFLECT( eosio::account_transaction_history_object, (account_name)(transaction_id) )

