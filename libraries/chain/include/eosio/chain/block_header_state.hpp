#pragma once
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/incremental_merkle.hpp>

namespace eosio { namespace chain {

/**
 *  @struct block_header_state
 *  @brief defines the minimum state necessary to validate transaction headers
 */
struct block_header_state {
    block_id_type                     id;
    uint32_t                          block_num = 0;
    signed_block_header               header;
    uint32_t                          dpos_proposed_irreversible_blocknum = 0;
    uint32_t                          dpos_irreversible_blocknum = 0;
    uint32_t                          bft_irreversible_blocknum = 0;
    uint32_t                          pending_schedule_lib_num = 0; /// last irr block num
    digest_type                       pending_schedule_hash;
    producer_schedule_type            pending_schedule;
    producer_schedule_type            active_schedule;
    incremental_merkle                blockroot_merkle;
    flat_map<account_name,uint32_t>   producer_to_last_produced;
    flat_map<account_name,uint32_t>   producer_to_last_implied_irb;
    public_key_type                   block_signing_key;
    vector<uint8_t>                   confirm_count;
    vector<header_confirmation>       confirmations;

    block_header_state   next( const signed_block_header& h, bool trust = false )const;
    block_header_state   generate_next( block_timestamp_type when )const;

    void set_new_producers( producer_schedule_type next_pending );
    void set_confirmed( uint16_t num_prev_blocks );
    void add_confirmation( const header_confirmation& c );
    bool maybe_promote_pending();


    bool                 has_pending_producers()const { return pending_schedule.producers.size(); }
    uint32_t             calc_dpos_last_irreversible()const;
    bool                 is_active_producer( account_name n )const;

    /*
    block_timestamp_type get_slot_time( uint32_t slot_num )const;
    uint32_t             get_slot_at_time( block_timestamp_type t )const;
    producer_key         get_scheduled_producer( uint32_t slot_num )const;
    uint32_t             producer_participation_rate()const;
    */

    producer_key         get_scheduled_producer( block_timestamp_type t )const;
    const block_id_type& prev()const { return header.previous; }
    digest_type          sig_digest()const;
    void                 sign( const std::function<signature_type(const digest_type&)>& signer, bool trust = false );
    public_key_type      signee()const;
};



} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::block_header_state,
            (id)(block_num)(header)(dpos_proposed_irreversible_blocknum)(dpos_irreversible_blocknum)(bft_irreversible_blocknum)
            (pending_schedule_lib_num)(pending_schedule_hash)
            (pending_schedule)(active_schedule)(blockroot_merkle)
            (producer_to_last_produced)(producer_to_last_implied_irb)(block_signing_key)
            (confirm_count)(confirmations) )
