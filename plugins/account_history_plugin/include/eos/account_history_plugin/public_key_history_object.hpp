#pragma once

#include <chainbase/chainbase.hpp>
#include <fc/array.hpp>

namespace std {

   template<>
   struct hash<eos::chain::public_key_type>
   {
      size_t operator()( const eos::chain::public_key_type& key ) const
      {
         std::hash<fc::ecc::public_key_data> hash;
         return hash(key.key_data);
      }
   };
}

namespace eos {
using chain::AccountName;
using chain::public_key_type;
using chain::PermissionName;
using namespace boost::multi_index;

class public_key_history_object : public chainbase::object<chain::public_key_history_object_type, public_key_history_object> {
   OBJECT_CTOR(public_key_history_object)

   id_type           id;
   public_key_type   public_key;
   AccountName       account_name;
   PermissionName    permission;
};

struct by_id;
struct by_pub_key;
struct by_account_permission;
using public_key_history_multi_index = chainbase::shared_multi_index_container<
   public_key_history_object,
   indexed_by<
      ordered_unique<tag<by_id>, BOOST_MULTI_INDEX_MEMBER(public_key_history_object, public_key_history_object::id_type, id)>,
      hashed_non_unique<tag<by_pub_key>, BOOST_MULTI_INDEX_MEMBER(public_key_history_object, public_key_type, public_key), std::hash<public_key_type> >,
      hashed_non_unique<tag<by_account_permission>,
         composite_key< public_key_history_object,
            member<public_key_history_object, AccountName,     &public_key_history_object::account_name>,
            member<public_key_history_object, PermissionName,  &public_key_history_object::permission>
         >,
         composite_key_hash<
            std::hash<AccountName>,
            std::hash<PermissionName>
         >
      >
   >
>;

typedef chainbase::generic_index<public_key_history_multi_index> public_key_history_index;

}

CHAINBASE_SET_INDEX_TYPE( eos::public_key_history_object, eos::public_key_history_multi_index )

FC_REFLECT( eos::public_key_history_object, (public_key)(account_name)(permission) )

