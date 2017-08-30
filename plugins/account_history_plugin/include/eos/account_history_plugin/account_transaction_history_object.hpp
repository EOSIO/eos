#pragma once

#include <chainbase/chainbase.hpp>
#include <eos/chain/types.hpp>

namespace std {

   template<>
   struct hash<eos::chain::AccountName>
   {
    size_t operator()( const eos::chain::AccountName& name )const
    {
        return (uint64_t)name;
    }
   };
}

namespace eos {
using chain::AccountName;
using chain::shared_vector;
using chain::transaction_id_type;
using namespace boost::multi_index;

class account_transaction_history_object : public chainbase::object<chain::account_transaction_history_object_type, account_transaction_history_object> {
   OBJECT_CTOR(account_transaction_history_object)

   id_type                            id;
   AccountName                        account_name;
   transaction_id_type                transaction_id;
};

struct by_id;
struct by_account_name;
struct by_account_name_trx_id;
using account_transaction_history_multi_index = chainbase::shared_multi_index_container<
   account_transaction_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(account_transaction_history_object, account_transaction_history_object::id_type, id)>,
      hashed_non_unique<tag<by_account_name>, BOOST_MULTI_INDEX_MEMBER(account_transaction_history_object, AccountName, account_name), std::hash<AccountName>>,
      hashed_unique<tag<by_account_name_trx_id>,
         composite_key< account_transaction_history_object,
            member<account_transaction_history_object, AccountName, &account_transaction_history_object::account_name>,
            member<account_transaction_history_object, transaction_id_type, &account_transaction_history_object::transaction_id>
         >,
         composite_key_hash< std::hash<AccountName>, std::hash<transaction_id_type> >
      >
   >
>;

typedef chainbase::generic_index<account_transaction_history_multi_index> account_transaction_history_index;

}

CHAINBASE_SET_INDEX_TYPE( eos::account_transaction_history_object, eos::account_transaction_history_multi_index )

FC_REFLECT( eos::account_transaction_history_object, (account_name)(transaction_id) )

