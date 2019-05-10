#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/exceptions.hpp>
#include <limits>

namespace eosio { namespace chain {


   bool block_header_state::is_active_producer( account_name n )const {
      return producer_to_last_produced.find(n) != producer_to_last_produced.end();
   }

   producer_keys block_header_state::get_scheduled_producer( block_timestamp_type t )const {
      auto index = t.slot % (active_schedule.producers.size() * config::producer_repetitions);
      index /= config::producer_repetitions;
      return active_schedule.producers[index];
   }

   uint32_t block_header_state::calc_dpos_last_irreversible( account_name producer_of_next_block )const {
      vector<uint32_t> blocknums; blocknums.reserve( producer_to_last_implied_irb.size() );
      for( auto& i : producer_to_last_implied_irb ) {
         blocknums.push_back( (i.first == producer_of_next_block) ? dpos_proposed_irreversible_blocknum : i.second);
      }
      /// 2/3 must be greater, so if I go 1/3 into the list sorted from low to high, then 2/3 are greater

      if( blocknums.size() == 0 ) return 0;

      std::size_t index = (blocknums.size()-1) / 3;
      std::nth_element( blocknums.begin(),  blocknums.begin() + index, blocknums.end() );
      return blocknums[ index ];
   }

   pending_block_header_state  block_header_state::next( block_timestamp_type when,
                                                         uint16_t num_prev_blocks_to_confirm )const
   {
      pending_block_header_state result;

      if( when != block_timestamp_type() ) {
        EOS_ASSERT( when > header.timestamp, block_validate_exception, "next block must be in the future" );
      } else {
        (when = header.timestamp).slot++;
      }

      auto prokey = get_scheduled_producer(when);

      auto itr = producer_to_last_produced.find( prokey.producer_name );
      if( itr != producer_to_last_produced.end() ) {
        EOS_ASSERT( itr->second < (block_num+1) - num_prev_blocks_to_confirm, producer_double_confirm,
                    "producer ${prod} double-confirming known range",
                    ("prod", prokey.producer_name)("num", block_num+1)
                    ("confirmed", num_prev_blocks_to_confirm)("last_produced", itr->second) );
      }

      result.block_num                                       = block_num + 1;
      result.previous                                        = id;
      result.timestamp                                       = when;
      result.confirmed                                       = num_prev_blocks_to_confirm;
      result.active_schedule_version                         = active_schedule.version;
      result.prev_activated_protocol_features                = activated_protocol_features;

      result.valid_block_signing_keys                        = prokey.block_signing_keys;
      result.producer                                        = prokey.producer_name;

      result.blockroot_merkle = blockroot_merkle;
      result.blockroot_merkle.append( id );

      /// grow the confirmed count
      static_assert(std::numeric_limits<uint8_t>::max() >= (config::max_producers * 2 / 3) + 1, "8bit confirmations may not be able to hold all of the needed confirmations");

      // This uses the previous block active_schedule because thats the "schedule" that signs and therefore confirms _this_ block
      auto num_active_producers = active_schedule.producers.size();
      uint32_t required_confs = (uint32_t)(num_active_producers * 2 / 3) + 1;

      if( confirm_count.size() < config::maximum_tracked_dpos_confirmations ) {
         result.confirm_count.reserve( confirm_count.size() + 1 );
         result.confirm_count  = confirm_count;
         result.confirm_count.resize( confirm_count.size() + 1 );
         result.confirm_count.back() = (uint8_t)required_confs;
      } else {
         result.confirm_count.resize( confirm_count.size() );
         memcpy( &result.confirm_count[0], &confirm_count[1], confirm_count.size() - 1 );
         result.confirm_count.back() = (uint8_t)required_confs;
      }

      auto new_dpos_proposed_irreversible_blocknum = dpos_proposed_irreversible_blocknum;

      int32_t i = (int32_t)(result.confirm_count.size() - 1);
      uint32_t blocks_to_confirm = num_prev_blocks_to_confirm + 1; /// confirm the head block too
      while( i >= 0 && blocks_to_confirm ) {
        --result.confirm_count[i];
        //idump((confirm_count[i]));
        if( result.confirm_count[i] == 0 )
        {
            uint32_t block_num_for_i = result.block_num - (uint32_t)(result.confirm_count.size() - 1 - i);
            new_dpos_proposed_irreversible_blocknum = block_num_for_i;
            //idump((dpos2_lib)(block_num)(dpos_irreversible_blocknum));

            if (i == static_cast<int32_t>(result.confirm_count.size() - 1)) {
               result.confirm_count.resize(0);
            } else {
               memmove( &result.confirm_count[0], &result.confirm_count[i + 1], result.confirm_count.size() - i  - 1);
               result.confirm_count.resize( result.confirm_count.size() - i - 1 );
            }

            break;
        }
        --i;
        --blocks_to_confirm;
      }

      result.dpos_proposed_irreversible_blocknum   = new_dpos_proposed_irreversible_blocknum;
      result.dpos_irreversible_blocknum            = calc_dpos_last_irreversible( prokey.producer_name );

      result.prev_pending_schedule                 = pending_schedule;

      if( pending_schedule.schedule.producers.size() &&
          result.dpos_irreversible_blocknum >= pending_schedule.schedule_lib_num )
      {
         result.active_schedule = pending_schedule.schedule;

         flat_map<account_name,uint32_t> new_producer_to_last_produced;

         for( const auto& pro : result.active_schedule.producers ) {
            if( pro.producer_name == prokey.producer_name ) {
               new_producer_to_last_produced[pro.producer_name] = result.block_num;
            } else {
               auto existing = producer_to_last_produced.find( pro.producer_name );
               if( existing != producer_to_last_produced.end() ) {
                  new_producer_to_last_produced[pro.producer_name] = existing->second;
               } else {
                  new_producer_to_last_produced[pro.producer_name] = result.dpos_irreversible_blocknum;
               }
            }
         }
         new_producer_to_last_produced[prokey.producer_name] = result.block_num;

         result.producer_to_last_produced = std::move( new_producer_to_last_produced );

         flat_map<account_name,uint32_t> new_producer_to_last_implied_irb;

         for( const auto& pro : result.active_schedule.producers ) {
            if( pro.producer_name == prokey.producer_name ) {
               new_producer_to_last_implied_irb[pro.producer_name] = dpos_proposed_irreversible_blocknum;
            } else {
               auto existing = producer_to_last_implied_irb.find( pro.producer_name );
               if( existing != producer_to_last_implied_irb.end() ) {
                  new_producer_to_last_implied_irb[pro.producer_name] = existing->second;
               } else {
                  new_producer_to_last_implied_irb[pro.producer_name] = result.dpos_irreversible_blocknum;
               }
            }
         }

         result.producer_to_last_implied_irb = std::move( new_producer_to_last_implied_irb );

         result.was_pending_promoted = true;
      } else {
         result.active_schedule                  = active_schedule;
         result.producer_to_last_produced        = producer_to_last_produced;
         result.producer_to_last_produced[prokey.producer_name] = block_num;
         result.producer_to_last_implied_irb     = producer_to_last_implied_irb;
         result.producer_to_last_implied_irb[prokey.producer_name] = dpos_proposed_irreversible_blocknum;
      }

      return result;
   }

   signed_block_header pending_block_header_state::make_block_header(
                                                      const checksum256_type& transaction_mroot,
                                                      const checksum256_type& action_mroot,
                                                      std::optional<producer_authority_schedule>&& new_producers,
                                                      vector<digest_type>&& new_protocol_feature_activations,
                                                      const protocol_feature_set& pfs
   )const
   {
      signed_block_header h;

      h.timestamp         = timestamp;
      h.producer          = producer;
      h.confirmed         = confirmed;
      h.previous          = previous;
      h.transaction_mroot = transaction_mroot;
      h.action_mroot      = action_mroot;
      h.schedule_version  = active_schedule_version;

      if( new_protocol_feature_activations.size() > 0 ) {
         h.header_extensions.emplace_back(
               protocol_feature_activation::extension_id(),
               fc::raw::pack( protocol_feature_activation{ std::move(new_protocol_feature_activations) } )
         );
      }

      if (new_producers) {
         auto wtmsig_digest = pfs.get_builtin_digest(builtin_protocol_feature_t::wtmsig_block_signatures);
         const auto& protocol_features = prev_activated_protocol_features->protocol_features;
         bool wtmsig_enabled = wtmsig_digest && protocol_features.find(*wtmsig_digest) != protocol_features.end();

         if ( wtmsig_enabled ) {
            // add the header extension to update the block schedule
            h.header_extensions.emplace_back(
                  producer_schedule_change_extension::extension_id(),
                  fc::raw::pack( producer_schedule_change_extension( std::move(*new_producers) ) )
            );
         } else {
            legacy::producer_schedule_type downgraded_producers;
            downgraded_producers.version = new_producers->version;
            for (const auto &p : new_producers->producers) {
               p.visit([&downgraded_producers](const auto& auth){
                  EOS_ASSERT(auth.keys.size() == 1 && auth.keys.front().weight == auth.threshold, producer_schedule_exception, "multisig block signing present before enabled!");
                  downgraded_producers.producers.emplace_back(legacy::producer_key{auth.producer_name, auth.keys.front().key});
               });
            }
            h.new_producers = downgraded_producers;
         }
      }

      return h;
   }

   block_header_state pending_block_header_state::_finish_next(
                                 const signed_block_header& h,
                                 const protocol_feature_set& pfs,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator

   )&&
   {
      EOS_ASSERT( h.timestamp == timestamp, block_validate_exception, "timestamp mismatch" );
      EOS_ASSERT( h.previous == previous, unlinkable_block_exception, "previous mismatch" );
      EOS_ASSERT( h.confirmed == confirmed, block_validate_exception, "confirmed mismatch" );
      EOS_ASSERT( h.producer == producer, wrong_producer, "wrong producer specified" );
      EOS_ASSERT( h.schedule_version == active_schedule_version, producer_schedule_exception, "schedule_version in signed block is corrupted" );

      auto exts = h.validate_and_extract_header_extensions();

      std::optional<producer_authority_schedule> maybe_new_producer_schedule;

      if( h.new_producers ) {
         auto wtmsig_digest = pfs.get_builtin_digest(builtin_protocol_feature_t::wtmsig_block_signatures);
         const auto& protocol_features = prev_activated_protocol_features->protocol_features;
         bool wtmsig_enabled = wtmsig_digest && protocol_features.find(*wtmsig_digest) != protocol_features.end();

         EOS_ASSERT(!wtmsig_enabled, producer_schedule_exception, "Block header contains legacy producer schedule outdated by activation of WTMsig Block Signatures" );

         EOS_ASSERT( !was_pending_promoted, producer_schedule_exception, "cannot set pending producer schedule in the same block in which pending was promoted to active" );

         const auto& new_producers = *h.new_producers;
         EOS_ASSERT( new_producers.version == active_schedule.version + 1, producer_schedule_exception, "wrong producer schedule version specified" );
         EOS_ASSERT( prev_pending_schedule.schedule.producers.empty(), producer_schedule_exception,
                    "cannot set new pending producers until last pending is confirmed" );

         maybe_new_producer_schedule.emplace(new_producers);
      }

      if ( exts.count(producer_schedule_change_extension::extension_id()) > 0 ) {
         auto wtmsig_digest = pfs.get_builtin_digest(builtin_protocol_feature_t::wtmsig_block_signatures);
         const auto& protocol_features = prev_activated_protocol_features->protocol_features;
         bool wtmsig_enabled = wtmsig_digest && protocol_features.find(*wtmsig_digest) != protocol_features.end();

         EOS_ASSERT(wtmsig_enabled, producer_schedule_exception, "Block header producer_schedule_change_extension before activation of WTMsig Block Signatures" );
         EOS_ASSERT( !was_pending_promoted, producer_schedule_exception, "cannot set pending producer schedule in the same block in which pending was promoted to active" );

         const auto& new_producer_schedule = exts.lower_bound(producer_schedule_change_extension::extension_id())->second.get<producer_schedule_change_extension>();

         EOS_ASSERT( new_producer_schedule.version == active_schedule.version + 1, producer_schedule_exception, "wrong producer schedule version specified" );
         EOS_ASSERT( prev_pending_schedule.schedule.producers.empty(), producer_schedule_exception,
                     "cannot set new pending producers until last pending is confirmed" );

         maybe_new_producer_schedule.emplace(new_producer_schedule);
      }

      protocol_feature_activation_set_ptr new_activated_protocol_features;
      { // handle protocol_feature_activation
         if( exts.count(protocol_feature_activation::extension_id() > 0) ) {
            const auto& new_protocol_features = exts.lower_bound(protocol_feature_activation::extension_id())->second.get<protocol_feature_activation>().protocol_features;
            validator( timestamp, prev_activated_protocol_features->protocol_features, new_protocol_features );

            new_activated_protocol_features =   std::make_shared<protocol_feature_activation_set>(
                                                   *prev_activated_protocol_features,
                                                   new_protocol_features
                                                );
         } else {
            new_activated_protocol_features = std::move( prev_activated_protocol_features );
         }
      }

      auto block_number = block_num;

      block_header_state result( std::move( *static_cast<detail::block_header_state_common*>(this) ) );

      result.id      = h.id();
      result.header  = h;

      result.header_exts = std::move(exts);

      if( maybe_new_producer_schedule ) {
         result.pending_schedule.schedule = std::move(*maybe_new_producer_schedule);
         result.pending_schedule.schedule_hash = digest_type::hash(result.pending_schedule.schedule);
         result.pending_schedule.schedule_lib_num    = block_number;
      } else {
         if( was_pending_promoted ) {
            result.pending_schedule.schedule.version = prev_pending_schedule.schedule.version;
         } else {
            result.pending_schedule.schedule         = std::move( prev_pending_schedule.schedule );
         }
         result.pending_schedule.schedule_hash       = std::move( prev_pending_schedule.schedule_hash );
         result.pending_schedule.schedule_lib_num    = prev_pending_schedule.schedule_lib_num;
      }

      result.activated_protocol_features = std::move( new_activated_protocol_features );

      return result;
   }

   block_header_state pending_block_header_state::finish_next(
                                 const signed_block_header& h,
                                 const protocol_feature_set& pfs,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator,
                                 bool skip_validate_signee
   )&&
   {
      auto result = std::move(*this)._finish_next( h, pfs, validator );

      // ASSUMPTION FROM controller_impl::apply_block = all untrusted blocks will have their signatures pre-validated here
      if( !skip_validate_signee ) {
        result.verify_signee( );
      }

      return result;
   }

   block_header_state pending_block_header_state::finish_next(
                                 signed_block_header& h,
                                 const protocol_feature_set& pfs,
                                 const std::function<void( block_timestamp_type,
                                                           const flat_set<digest_type>&,
                                                           const vector<digest_type>& )>& validator,
                                 const std::function<signature_type(const digest_type&)>& signer
   )&&
   {
      auto result = std::move(*this)._finish_next( h, pfs, validator );
      result.sign( signer );
      h.producer_signature = result.header.producer_signature;
      return result;
   }

   /**
    *  Transitions the current header state into the next header state given the supplied signed block header.
    *
    *  Given a signed block header, generate the expected template based upon the header time,
    *  then validate that the provided header matches the template.
    *
    *  If the header specifies new_producers then apply them accordingly.
    */
   block_header_state block_header_state::next(
                        const signed_block_header& h,
                        const protocol_feature_set& pfs,
                        const std::function<void( block_timestamp_type,
                                                  const flat_set<digest_type>&,
                                                  const vector<digest_type>& )>& validator,
                        bool skip_validate_signee )const
   {
      return next( h.timestamp, h.confirmed ).finish_next( h, pfs, validator, skip_validate_signee );
   }

   digest_type   block_header_state::sig_digest()const {
      auto header_bmroot = digest_type::hash( std::make_pair( header.digest(), blockroot_merkle.get_root() ) );
      return digest_type::hash( std::make_pair(header_bmroot, pending_schedule.schedule_hash) );
   }

   void block_header_state::sign( const std::function<signature_type(const digest_type&)>& signer ) {
      auto d = sig_digest();
      header.producer_signature = signer( d );

      EOS_ASSERT( valid_block_signing_keys.find(signee()) != valid_block_signing_keys.end(),
                  wrong_signing_key, "block is signed with unexpected key" );
   }

   public_key_type block_header_state::signee()const {
      return fc::crypto::public_key( header.producer_signature, sig_digest(), true );
   }

   void block_header_state::verify_signee( )const {
      auto signing_key = signee();
      EOS_ASSERT( valid_block_signing_keys.find(signing_key) != valid_block_signing_keys.end(), wrong_signing_key,
                  "block not signed by expected key",
                  ("signing_key", signing_key)( "valid_keys", valid_block_signing_keys ) );
   }

   /**
    *  Reference cannot outlive *this. Assumes header_exts is not mutated after instatiation.
    */
   const vector<digest_type>& block_header_state::get_new_protocol_feature_activations()const {
      static const vector<digest_type> no_activations{};

      if( header_exts.count(protocol_feature_activation::extension_id()) == 0 )
         return no_activations;

      return header_exts.lower_bound(protocol_feature_activation::extension_id())->second.get<protocol_feature_activation>().protocol_features;
   }

} } /// namespace eosio::chain
