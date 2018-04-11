#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

  uint32_t block_header_state::calc_dpos_last_irreversible()const {
    vector<uint32_t> irb;
    irb.reserve( producer_to_last_produced.size() );
    for( const auto& item : producer_to_last_produced ) 
       irb.push_back(item.second);

    size_t offset = EOS_PERCENT(irb.size(), config::percent_100- config::irreversible_threshold_percent);
    std::nth_element( irb.begin(), irb.begin() + offset, irb.end() );

    return irb[offset];
  }


  bool block_header_state::is_active_producer( account_name n )const {
    return producer_to_last_produced.find(n) != producer_to_last_produced.end();
  }

  producer_key block_header_state::scheduled_producer( block_timestamp_type t )const {
    auto index = t.slot % (active_schedule.producers.size() * config::producer_repetitions);
    index /= config::producer_repetitions;
    return active_schedule.producers[index];
  }


  /**
   *  Transitions the current header state into the next header state given the supplied signed block header.
   *
   *  @return the next block_header_state
   */
  block_header_state block_header_state::next( const signed_block_header& h )const {
    FC_ASSERT( h.previous == id, "block must link to current state" );
    FC_ASSERT( h.timestamp > header.timestamp, "block must be later in time" );

    auto prokey = scheduled_producer(h.timestamp);
    FC_ASSERT( prokey.producer_name == h.producer, "producer is not scheduled for this time slot" );

    auto signkey = h.signee( pending_schedule_hash );
    FC_ASSERT( signkey == prokey.block_signing_key, "block is not signed by proper key" );

    block_header_state result(*this);
    result.id = h.id();
    result.block_num = h.block_num();
    result.producer_to_last_produced[h.producer] = result.block_num;
    result.dpos_last_irreversible_blocknum = result.calc_dpos_last_irreversible();

    result.blockroot_merkle.append( result.id );
    FC_ASSERT( h.block_mroot == result.blockroot_merkle.get_root(), "unexpected block merkle root" );

    if( result.dpos_last_irreversible_blocknum >= pending_schedule_lib_num ) {
      result.active_schedule = move(result.pending_schedule);

      flat_map<account_name,uint32_t> new_producer_to_last_produced;
      for( const auto& pro : result.active_schedule.producers ) {
        auto existing = producer_to_last_produced.find( pro.producer_name );
        if( existing != producer_to_last_produced.end() ) {
          new_producer_to_last_produced[pro.producer_name] = existing->second;
        } else {
          new_producer_to_last_produced[pro.producer_name] = result.dpos_last_irreversible_blocknum;
        }
      }
      result.producer_to_last_produced = move( new_producer_to_last_produced );
    }

    if( h.new_producers ) {

      FC_ASSERT( h.new_producers->version == result.active_schedule.version + 1, "wrong producer schedule version specified" );
      FC_ASSERT( result.pending_schedule.producers.size() == 0, 
                 "cannot set new pending producers until last pending is confirmed" );
      result.pending_schedule         = *h.new_producers;
      result.pending_schedule_hash    = digest_type::hash( result.pending_schedule );
      result.pending_schedule_lib_num = h.block_num();
    } 

    result.header = h;

    return result;
  } /// next

} } /// namespace eosio::chain
