#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/exceptions.hpp>
#include <limits>

namespace eosio { namespace chain {

  uint32_t block_header_state::calc_dpos_last_irreversible()const {
    if( producer_to_last_produced.size() == 0 )
       return 0;

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

   block_timestamp_type block_header_state::get_slot_time( uint32_t slot_num )const {
      auto t = header.timestamp;
      FC_ASSERT( std::numeric_limits<decltype(t.slot)>::max() - t.slot >= slot_num, "block timestamp overflow" );
      t.slot += slot_num;
      return t;
   }

   uint32_t block_header_state::get_slot_at_time( block_timestamp_type t )const {
      auto first_slot_time = get_slot_time(1);
      if( t < first_slot_time )
         return 0;
      return (t.slot - first_slot_time.slot + 1);
   }

   producer_key block_header_state::get_scheduled_producer( uint32_t slot_num )const {
      return get_scheduled_producer( get_slot_time(slot_num) );
   }

   producer_key block_header_state::get_scheduled_producer( block_timestamp_type t )const {
      auto index = t.slot % (active_schedule.producers.size() * config::producer_repetitions);
      index /= config::producer_repetitions;
      return active_schedule.producers[index];
   }

   uint32_t block_header_state::producer_participation_rate()const
   {
      return static_cast<uint32_t>(config::percent_100); // Ignore participation rate for now until we construct a better metric.
   }


  /**
   *  Generate a template block header state for a given block time, it will not
   *  contain a transaction mroot, action mroot, or new_producers as those components
   *  are derived from chain state.
   */
  block_header_state block_header_state::generate_next( block_timestamp_type when )const {
    block_header_state result;

    if( when != block_timestamp_type() ) {
       FC_ASSERT( when > header.timestamp, "next block must be in the future" );
    } else {
       (when = header.timestamp).slot++;
    }
    result.header.timestamp                      = when;
    result.header.previous                       = id;
    result.header.schedule_version               = active_schedule.version;

    auto prokey                                  = get_scheduled_producer(when);
    result.block_signing_key                     = prokey.block_signing_key;
    result.header.producer                       = prokey.producer_name;

    result.pending_schedule_lib_num              = pending_schedule_lib_num;
    result.pending_schedule_hash                 = pending_schedule_hash;
    result.block_num                             = block_num + 1;
    result.producer_to_last_produced             = producer_to_last_produced;
    result.producer_to_last_produced[prokey.producer_name] = result.block_num;
    result.dpos_last_irreversible_blocknum       = result.calc_dpos_last_irreversible();
    result.bft_irreversible_blocknum             =
                std::max(bft_irreversible_blocknum,result.dpos_last_irreversible_blocknum);
    result.blockroot_merkle = blockroot_merkle;
    result.blockroot_merkle.append( id );

    auto block_mroot = result.blockroot_merkle.get_root();

    result.active_schedule  = active_schedule;
    result.pending_schedule = pending_schedule;


    if( result.pending_schedule.producers.size() &&
        result.dpos_last_irreversible_blocknum >= pending_schedule_lib_num ) {
      result.active_schedule = move( result.pending_schedule );

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
      result.producer_to_last_produced[prokey.producer_name] = result.block_num;
    }

    return result;
  } /// generate_next


  void block_header_state::set_new_producers( producer_schedule_type pending ) {
      FC_ASSERT( pending.version == active_schedule.version + 1, "wrong producer schedule version specified" );
      FC_ASSERT( pending_schedule.producers.size() == 0,
                 "cannot set new pending producers until last pending is confirmed" );
      header.new_producers     = move(pending);
      pending_schedule_hash    = digest_type::hash( *header.new_producers );
      pending_schedule         = *header.new_producers;
      pending_schedule_lib_num = block_num;
  }


  /**
   *  Transitions the current header state into the next header state given the supplied signed block header.
   *
   *  Given a signed block header, generate the expected template based upon the header time,
   *  then validate that the provided header matches the template.
   *
   *  If the header specifies new_producers then apply them accordingly.
   */
  block_header_state block_header_state::next( const signed_block_header& h )const {
    FC_ASSERT( h.timestamp != block_timestamp_type(), "", ("h",h) );

    FC_ASSERT( h.timestamp > header.timestamp, "block must be later in time" );
    FC_ASSERT( h.previous == id, "block must link to current state" );
    auto result = generate_next( h.timestamp );
    FC_ASSERT( result.header.producer == h.producer, "wrong producer specified" );
    FC_ASSERT( result.header.schedule_version == h.schedule_version, "schedule_version in signed block is corrupted" );

    // FC_ASSERT( result.header.block_mroot == h.block_mroot, "mistmatch block merkle root" );

     /// below this point is state changes that cannot be validated with headers alone, but never-the-less,
     /// must result in header state changes
    if( h.new_producers ) {
       result.set_new_producers( *h.new_producers );
    }

    result.header.action_mroot       = h.action_mroot;
    result.header.transaction_mroot  = h.transaction_mroot;
    result.header.producer_signature = h.producer_signature;
    result.id                        = result.header.id();

    FC_ASSERT( result.block_signing_key == result.signee(), "block not signed by expected key",
               ("result.block_signing_key", result.block_signing_key)("signee", result.signee() ) );

    return result;
  } /// next

  digest_type   block_header_state::sig_digest()const {
     auto header_bmroot = digest_type::hash( std::make_pair( header.digest(), blockroot_merkle.get_root() ) );
     return digest_type::hash( std::make_pair(header_bmroot, pending_schedule_hash) );
  }

  void block_header_state::sign( const std::function<signature_type(const digest_type&)>& signer ) {
     auto d = sig_digest();
     header.producer_signature = signer( d );
     FC_ASSERT( block_signing_key == fc::crypto::public_key( header.producer_signature, d ) );
  }

  public_key_type block_header_state::signee()const {
    return fc::crypto::public_key( header.producer_signature, sig_digest(), true );
  }

  void block_header_state::add_confirmation( const header_confirmation& conf ) {
     for( const auto& c : confirmations )
        FC_ASSERT( c.producer == conf.producer, "block already confirmed by this producer" );

     auto key = active_schedule.get_producer_key( conf.producer );
     FC_ASSERT( key != public_key_type(), "producer not in current schedule" );
     auto signer = fc::crypto::public_key( conf.producer_signature, sig_digest(), true );
     FC_ASSERT( signer == key, "confirmation not signed by expected key" );

     confirmations.emplace_back( conf );
  }


} } /// namespace eosio::chain
