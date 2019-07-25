#pragma once
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>
#include <chainbase/chainbase.hpp>
#include <eosio/chain/authority.hpp>
#include <eosio/chain/snapshot.hpp>

namespace eosio { namespace chain {

   namespace legacy {
      /**
       *  Used as part of the producer_schedule_type, maps the producer name to their key.
       */
      struct producer_key {
         account_name      producer_name;
         public_key_type   block_signing_key;

         friend bool operator == ( const producer_key& lhs, const producer_key& rhs ) {
            return tie( lhs.producer_name, lhs.block_signing_key ) == tie( rhs.producer_name, rhs.block_signing_key );
         }
         friend bool operator != ( const producer_key& lhs, const producer_key& rhs ) {
            return tie( lhs.producer_name, lhs.block_signing_key ) != tie( rhs.producer_name, rhs.block_signing_key );
         }
      };

      /**
       *  Defines both the order, account name, and signing keys of the active set of producers.
       */
      struct producer_schedule_type {
         uint32_t                                       version = 0; ///< sequentially incrementing version number
         vector<producer_key>                           producers;

         friend bool operator == ( const producer_schedule_type& a, const producer_schedule_type& b )
         {
            if( a.version != b.version ) return false;
            if ( a.producers.size() != b.producers.size() ) return false;
            for( uint32_t i = 0; i < a.producers.size(); ++i )
               if( a.producers[i] != b.producers[i] ) return false;
            return true;
         }

         friend bool operator != ( const producer_schedule_type& a, const producer_schedule_type& b )
         {
            return !(a==b);
         }
      };
   }

   struct shared_block_signing_authority_v0 {
      shared_block_signing_authority_v0() = delete;
      shared_block_signing_authority_v0( const shared_block_signing_authority_v0& ) = default;
      shared_block_signing_authority_v0( shared_block_signing_authority_v0&& ) = default;
      shared_block_signing_authority_v0& operator= ( shared_block_signing_authority_v0 && ) = default;
      shared_block_signing_authority_v0& operator= ( const shared_block_signing_authority_v0 & ) = default;

      explicit shared_block_signing_authority_v0( chainbase::allocator<char> alloc )
      :keys(alloc){}

      uint32_t                           threshold = 0;
      shared_vector<shared_key_weight>   keys;
   };

   using shared_block_signing_authority = static_variant<shared_block_signing_authority_v0>;

   struct shared_producer_authority {
      shared_producer_authority() = delete;
      shared_producer_authority( const shared_producer_authority& ) = default;
      shared_producer_authority( shared_producer_authority&& ) = default;
      shared_producer_authority& operator= ( shared_producer_authority && ) = default;
      shared_producer_authority& operator= ( const shared_producer_authority & ) = default;

      shared_producer_authority( const name& producer_name, shared_block_signing_authority&& authority )
      :producer_name(producer_name)
      ,authority(std::move(authority))
      {}

      name                                     producer_name;
      shared_block_signing_authority           authority;
   };

   struct shared_producer_authority_schedule {
      shared_producer_authority_schedule() = delete;

      explicit shared_producer_authority_schedule( chainbase::allocator<char> alloc )
      :producers(alloc){}

      shared_producer_authority_schedule( const shared_producer_authority_schedule& ) = default;
      shared_producer_authority_schedule( shared_producer_authority_schedule&& ) = default;
      shared_producer_authority_schedule& operator= ( shared_producer_authority_schedule && ) = default;
      shared_producer_authority_schedule& operator= ( const shared_producer_authority_schedule & ) = default;

      uint32_t                                       version = 0; ///< sequentially incrementing version number
      shared_vector<shared_producer_authority>       producers;
   };

   /**
    * block signing authority version 0
    * this authority allows for a weighted threshold multi-sig per-producer
    */
   struct block_signing_authority_v0 {
      static constexpr std::string_view abi_type_name() { return "block_signing_authority_v0"; }

      uint32_t                    threshold = 0;
      vector<key_weight>          keys;

      template<typename Op>
      void for_each_key( Op&& op ) const {
         for (const auto& kw : keys ) {
            op(kw.key);
         }
      }

      std::pair<bool, size_t> keys_satisfy_and_relevant( const std::set<public_key_type>& presented_keys ) const {
         size_t num_relevant_keys = 0;
         uint32_t total_weight = 0;
         for (const auto& kw : keys ) {
            const auto& iter = presented_keys.find(kw.key);
            if (iter != presented_keys.end()) {
               ++num_relevant_keys;

               if( total_weight < threshold ) {
                  total_weight += std::min<uint32_t>(std::numeric_limits<uint32_t>::max() - total_weight, kw.weight);
               }
            }
         }

         return {total_weight >= threshold, num_relevant_keys};
      }

      auto to_shared(chainbase::allocator<char> alloc) const {
         shared_block_signing_authority_v0 result(alloc);
         result.threshold = threshold;
         result.keys.clear();
         result.keys.reserve(keys.size());
         for (const auto& k: keys) {
            result.keys.emplace_back(shared_key_weight::convert(alloc, k));
         }

         return result;
      }

      static auto from_shared(const shared_block_signing_authority_v0& src) {
         block_signing_authority_v0 result;
         result.threshold = src.threshold;
         result.keys.reserve(src.keys.size());
         for (const auto& k: src.keys) {
            result.keys.push_back(k);
         }

         return result;
      }

      friend bool operator == ( const block_signing_authority_v0& lhs, const block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) == tie( rhs.threshold, rhs.keys );
      }
      friend bool operator != ( const block_signing_authority_v0& lhs, const block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) != tie( rhs.threshold, rhs.keys );
      }
   };

   using block_signing_authority = static_variant<block_signing_authority_v0>;

   struct producer_authority {
      name                    producer_name;
      block_signing_authority authority;

      template<typename Op>
      static void for_each_key( const block_signing_authority& authority, Op&& op ) {
         authority.visit([&op](const auto &a){
            a.for_each_key(std::forward<Op>(op));
         });
      }

      template<typename Op>
      void for_each_key( Op&& op ) const {
         for_each_key(authority, std::forward<Op>(op));
      }

      static std::pair<bool, size_t> keys_satisfy_and_relevant( const std::set<public_key_type>& keys, const block_signing_authority& authority ) {
         return authority.visit([&keys](const auto &a){
            return a.keys_satisfy_and_relevant(keys);
         });
      }

      std::pair<bool, size_t> keys_satisfy_and_relevant( const std::set<public_key_type>& presented_keys ) const {
         return keys_satisfy_and_relevant(presented_keys, authority);
      }

      auto to_shared(chainbase::allocator<char> alloc) const {
         auto shared_auth = authority.visit([&alloc](const auto& a) {
            return a.to_shared(alloc);
         });

         return shared_producer_authority(producer_name, std::move(shared_auth));
      }

      static auto from_shared( const shared_producer_authority& src ) {
         producer_authority result;
         result.producer_name = src.producer_name;
         result.authority = src.authority.visit(overloaded {
            [](const shared_block_signing_authority_v0& a) {
               return block_signing_authority_v0::from_shared(a);
            }
         });

         return result;
      }

      /**
       * ABI's for contracts expect variants to be serialized as a 2 entry array of
       * [type-name, value].
       *
       * This is incompatible with standard FC rules for
       * static_variants which produce
       *
       * [ordinal, value]
       *
       * this method produces an appropriate variant for contracts where the authority field
       * is correctly formatted
       */
      fc::variant get_abi_variant() const;

      friend bool operator == ( const producer_authority& lhs, const producer_authority& rhs ) {
         return tie( lhs.producer_name, lhs.authority ) == tie( rhs.producer_name, rhs.authority );
      }
      friend bool operator != ( const producer_authority& lhs, const producer_authority& rhs ) {
         return tie( lhs.producer_name, lhs.authority ) != tie( rhs.producer_name, rhs.authority );
      }
   };

   struct producer_authority_schedule {
      producer_authority_schedule() = default;

      /**
       * Up-convert a legacy producer schedule
       */
      explicit producer_authority_schedule( const legacy::producer_schedule_type& old )
      :version(old.version)
      {
         producers.reserve( old.producers.size() );
         for( const auto& p : old.producers )
            producers.emplace_back(producer_authority{ p.producer_name, block_signing_authority_v0{ 1, {{p.block_signing_key, 1}} } });
      }

      producer_authority_schedule( uint32_t version,  std::initializer_list<producer_authority> producers )
      :version(version)
      ,producers(producers)
      {}

      auto to_shared(chainbase::allocator<char> alloc) const {
         auto result = shared_producer_authority_schedule(alloc);
         result.version = version;
         result.producers.clear();
         result.producers.reserve( producers.size() );
         for( const auto& p : producers ) {
            result.producers.emplace_back(p.to_shared(alloc));
         }
         return result;
      }

      static auto from_shared( const shared_producer_authority_schedule& src ) {
         producer_authority_schedule result;
         result.version = src.version;
         result.producers.reserve(src.producers.size());
         for( const auto& p : src.producers ) {
            result.producers.emplace_back(producer_authority::from_shared(p));
         }

         return result;
      }

      uint32_t                                       version = 0; ///< sequentially incrementing version number
      vector<producer_authority>                     producers;

      friend bool operator == ( const producer_authority_schedule& a, const producer_authority_schedule& b )
      {
         if( a.version != b.version ) return false;
         if ( a.producers.size() != b.producers.size() ) return false;
         for( uint32_t i = 0; i < a.producers.size(); ++i )
            if( ! (a.producers[i] == b.producers[i]) ) return false;
         return true;
      }

      friend bool operator != ( const producer_authority_schedule& a, const producer_authority_schedule& b )
      {
         return !(a==b);
      }
   };

   /**
    * Block Header Extension Compatibility
    */
   struct producer_schedule_change_extension : producer_authority_schedule {

      static constexpr uint16_t extension_id() { return 1; }
      static constexpr bool     enforce_unique() { return true; }

      producer_schedule_change_extension() = default;
      producer_schedule_change_extension(const producer_schedule_change_extension&) = default;
      producer_schedule_change_extension( producer_schedule_change_extension&& ) = default;

      producer_schedule_change_extension( const producer_authority_schedule& sched )
      :producer_authority_schedule(sched) {}
   };


   inline bool operator == ( const producer_authority& pa, const shared_producer_authority& pb )
   {
      if(pa.producer_name != pb.producer_name) return false;
      if(pa.authority.which() != pb.authority.which()) return false;

      bool authority_matches = pa.authority.visit([&pb]( const auto& lhs ){
         return pb.authority.visit( [&lhs](const auto& rhs ) {
            if (lhs.threshold != rhs.threshold) return false;
            return std::equal(lhs.keys.cbegin(), lhs.keys.cend(), rhs.keys.cbegin(), rhs.keys.cend());
         });
      });

      if (!authority_matches) return false;
      return true;
   }

} } /// eosio::chain

FC_REFLECT( eosio::chain::legacy::producer_key, (producer_name)(block_signing_key) )
FC_REFLECT( eosio::chain::legacy::producer_schedule_type, (version)(producers) )
FC_REFLECT( eosio::chain::block_signing_authority_v0, (threshold)(keys))
FC_REFLECT( eosio::chain::producer_authority, (producer_name)(authority) )
FC_REFLECT( eosio::chain::producer_authority_schedule, (version)(producers) )
FC_REFLECT_DERIVED( eosio::chain::producer_schedule_change_extension, (eosio::chain::producer_authority_schedule), )

FC_REFLECT( eosio::chain::shared_block_signing_authority_v0, (threshold)(keys))
FC_REFLECT( eosio::chain::shared_producer_authority, (producer_name)(authority) )
FC_REFLECT( eosio::chain::shared_producer_authority_schedule, (version)(producers) )

