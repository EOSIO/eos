#pragma once
#include <eosio/chain/block_header.hpp>
#include <eosio/chain/incremental_merkle.hpp>
#include <future>

namespace eosio { namespace chain {

struct block_header_state;

struct pending_block_header_state {
   uint32_t                          block_num = 0;
   block_id_type                     previous;
   block_timestamp_type              timestamp;
   account_name                      producer;
   uint16_t                          confirmed = 1;
   uint32_t                          dpos_proposed_irreversible_blocknum = 0;
   uint32_t                          dpos_irreversible_blocknum = 0;
   uint32_t                          active_schedule_version = 0;
   uint32_t                          prev_pending_schedule_lib_num = 0; /// last irr block num
   digest_type                       prev_pending_schedule_hash;
   producer_schedule_type            prev_pending_schedule;
   producer_schedule_type            active_schedule;
   incremental_merkle                blockroot_merkle;
   flat_map<account_name,uint32_t>   producer_to_last_produced;
   flat_map<account_name,uint32_t>   producer_to_last_implied_irb;
   public_key_type                   block_signing_key;
   vector<uint8_t>                   confirm_count;
   bool                              was_pending_promoted = false;

   signed_block_header make_block_header( const checksum256_type& transaction_mroot,
                                          const checksum256_type& action_mroot,
                                          optional<producer_schedule_type>&& new_producers )const;

   block_header_state  finish_next( const signed_block_header& h, bool skip_validate_signee = false )&&;

   block_header_state  finish_next( signed_block_header& h,
                                   const std::function<signature_type(const digest_type&)>& signer )&&;

private:
   block_header_state  _finish_next( const signed_block_header& h )&&;
};


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

   pending_block_header_state  next( block_timestamp_type when, uint16_t num_prev_blocks_to_confirm )const;

   block_header_state   next( const signed_block_header& h, bool skip_validate_signee = false )const;

    bool                 has_pending_producers()const { return pending_schedule.producers.size(); }
    uint32_t             calc_dpos_last_irreversible( account_name producer_of_next_block )const;
    bool                 is_active_producer( account_name n )const;

    producer_key         get_scheduled_producer( block_timestamp_type t )const;
    const block_id_type& prev()const { return header.previous; }
    digest_type          sig_digest()const;
    void                 sign( const std::function<signature_type(const digest_type&)>& signer );
    public_key_type      signee()const;
    void                 verify_signee(const public_key_type& signee)const;
};

using block_header_state_ptr = std::shared_ptr<block_header_state>;

} } /// namespace eosio::chain

FC_REFLECT( eosio::chain::block_header_state,
            (id)(block_num)(header)(dpos_proposed_irreversible_blocknum)(dpos_irreversible_blocknum)         (pending_schedule_lib_num)(pending_schedule_hash)
            (pending_schedule)(active_schedule)(blockroot_merkle)
            (producer_to_last_produced)(producer_to_last_implied_irb)(block_signing_key)
            (confirm_count)(confirmations) )
