#pragma once
#include <eosio/blockchain/database.hpp>

namespace eosio { namespace blockchain {

struct global_state_object : public object< N(globalstate), global_state_object > {

   template<typename Constructor, typename Allocator>
   global_state_object( Constructor&& c, Allocator&& ){
      c(*this);
   }

   global_state_object::id_type   id;
   block_num_type                 head_block_num = 0;
   block_id_type                  head_block_id;
   digest_type                    block_root;
   time_point                     head_block_time;
};

typedef shared_multi_index_container< global_state_object,
   indexed_by<
      ordered_unique< tag<by_id>, member<global_state_object, global_state_object::id_type, &global_state_object::id > >
   >
> global_state_index;


struct active_producers_object : public object< N(producers), active_producers_object > {

   template<typename Constructor, typename Allocator>
   active_producers_object( Constructor&& c, Allocator&& a ):producers(a){
      c(*this);
   }

   active_producers_object::id_type   id;
   shared_vector<account_name>        producers;
};

typedef shared_multi_index_container< active_producers_object,
   indexed_by<
      ordered_unique< tag<by_id>, member<active_producers_object, active_producers_object::id_type, &active_producers_object::id > >
   >
> active_producers_index;

} } /// eosio::blockchain

EOSIO_SET_INDEX_TYPE( eosio::blockchain::global_state_object, eosio::blockchain::global_state_index );
EOSIO_SET_INDEX_TYPE( eosio::blockchain::active_producers_object, eosio::blockchain::active_producers_index );
