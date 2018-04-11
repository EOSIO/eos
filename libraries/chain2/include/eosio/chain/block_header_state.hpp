#pragma once 
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/incremental_merkle.hpp>

namespace eosio { namespace chain {

/**
 *  @struct block_header_state
 *  @brief defines the minimum state necessary to validate transaction headers
 */
struct block_header_state {
    block_id_type                   id;
    uint32_t                        block_num = 0;
    signed_block_header             header;
    uint32_t                        dpos_last_irreversible_blocknum = 0;
    uint32_t                        pending_schedule_lib_num = 0; /// last irr block num
    digest_type                     pending_schedule_hash;
    producer_schedule_type          pending_schedule;
    producer_schedule_type          active_schedule;
    incremental_merkle              blockroot_merkle;
    flat_map<account_name,uint32_t> producer_to_last_produced;
    
    block_header_state   next( const signed_block_header& h )const;

    uint32_t             calc_dpos_last_irreversible()const;
    bool                 is_active_producer( account_name n )const;
    producer_key         scheduled_producer( block_timestamp_type t )const;
};



} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::block_header_state, 
            (id)(block_num)(header)(dpos_last_irreversible_blocknum)
            (pending_schedule_lib_num)(pending_schedule_hash)
            (pending_schedule)(active_schedule)(blockroot_merkle)
            (producer_to_last_produced) )


