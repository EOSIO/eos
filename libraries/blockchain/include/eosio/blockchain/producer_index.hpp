#pragma once
namespace eosio { namespace blockchain {

struct producer_object : public object< N(producer), producer_object > {
   template<typename Constructor, typename Allocator>
   producer_object( Constructor&& c, Allocator&& ){
      c(*this);
   }

   producer_object::id_type   id;
   account_name               producer;
   public_key_type            key;
};


struct by_name;
typedef shared_multi_index_container< producer_object,
   indexed_by<
      ordered_unique< tag<by_id>, member<producer_object, producer_object::id_type, &producer_object::id > >,
      ordered_unique< tag<by_name>, member<producer_object, account_name, &producer_object::producer > >
   >
> producer_index;

} } /// eosio::blockchain

EOSIO_SET_INDEX_TYPE( eosio::blockchain::producer_object, eosio::blockchain::producer_index );
