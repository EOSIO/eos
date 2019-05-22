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

   /**
    * block signing authority version 0
    * this authority allows for a weighted threshold multi-sig per-producer
    */
   struct block_signing_authority_v0 {
      uint32_t                    threshold;
      vector<key_weight>          keys;

      bool key_is_relevant( const public_key_type& key ) const {
         return std::find_if(keys.begin(), keys.end(), [&key](const auto& kw){
            return kw.key == key;
         }) != keys.end();
      }

      bool keys_satisfy( const flat_set<public_key_type>& presented_keys ) const {
         uint32_t total_weight = 0;
         for (const auto& kw : keys) {
            const auto& iter = presented_keys.find(kw.key);
            if (iter != presented_keys.end()) {
               total_weight += kw.weight;
            }

            if (total_weight >= threshold)
               return true;
         }
         return false;
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

      static bool key_is_relevant( const public_key_type& key, const block_signing_authority& authority ) {
         return authority.visit([&key](const auto &a){
            return a.key_is_relevant(key);
         });
      }

      bool key_is_relevant( const public_key_type& key ) const {
         return key_is_relevant(key, authority);
      }

      static bool keys_satisfy( const flat_set<public_key_type>& keys, const block_signing_authority& authority ) {
         return authority.visit([&keys](const auto &a){
            return a.keys_satisfy(keys);
         });
      }

      bool keys_satisfy( const flat_set<public_key_type>& keys ) const {
         return keys_satisfy(keys, authority);
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

   struct shared_block_signing_authority_v0 {
      shared_block_signing_authority_v0( const shared_block_signing_authority_v0& ) = default;
      shared_block_signing_authority_v0( shared_block_signing_authority_v0&& ) = default;
      shared_block_signing_authority_v0& operator= ( shared_block_signing_authority_v0 && ) = default;
      shared_block_signing_authority_v0& operator= ( const shared_block_signing_authority_v0 & ) = default;

      shared_block_signing_authority_v0( chainbase::allocator<char> alloc )
      :keys(alloc){}

      uint32_t                    threshold;
      shared_vector<key_weight>   keys;

      friend bool operator == ( const shared_block_signing_authority_v0& lhs, const shared_block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) == tie( rhs.threshold, rhs.keys );
      }
      friend bool operator != ( const shared_block_signing_authority_v0& lhs, const shared_block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) != tie( rhs.threshold, rhs.keys );
      }
   };

   using shared_block_signing_authority = static_variant<shared_block_signing_authority_v0>;

   struct shared_producer_authority {
      shared_producer_authority() = default;
      shared_producer_authority( const shared_producer_authority& ) = default;
      shared_producer_authority( shared_producer_authority&& ) = default;
      shared_producer_authority& operator= ( shared_producer_authority && ) = default;
      shared_producer_authority& operator= ( const shared_producer_authority & ) = default;

      shared_producer_authority( const name& producer_name, shared_block_signing_authority&& authority )
      :producer_name(producer_name)
      ,authority(std::forward<shared_block_signing_authority>(authority))
      {}

      name                                     producer_name;
      shared_block_signing_authority           authority;

      friend bool operator == ( const shared_producer_authority& lhs, const shared_producer_authority& rhs ) {
         return tie( lhs.producer_name, lhs.authority ) == tie( rhs.producer_name, rhs.authority );
      }
      friend bool operator != ( const shared_producer_authority& lhs, const shared_producer_authority& rhs ) {
         return tie( lhs.producer_name, lhs.authority ) != tie( rhs.producer_name, rhs.authority );
      }
   };

   namespace detail {
      template<typename T>
      struct shared_converter;

      template<typename T>
      auto shared_convert( chainbase::allocator<char> alloc, const T& src ) {
         return shared_converter<std::decay_t<decltype(src)>>::convert(alloc, src);
      }

      template<typename T>
      auto shared_convert( const T& src ) {
         return shared_converter<std::decay_t<decltype(src)>>::convert(src);
      }

      template<>
      struct shared_converter<block_signing_authority_v0> {
         static shared_block_signing_authority_v0 convert (chainbase::allocator<char> alloc, const block_signing_authority_v0& src) {
            shared_block_signing_authority_v0 result(alloc);
            result.threshold = src.threshold;
            result.keys.clear();
            result.keys.reserve(src.keys.size());
            for (const auto& k: src.keys) {
               result.keys.push_back(k);
            }

            return result;
         }
      };

      template<>
      struct shared_converter<shared_block_signing_authority_v0> {
         static block_signing_authority_v0 convert (const shared_block_signing_authority_v0& src) {
            block_signing_authority_v0 result;
            result.threshold = src.threshold;
            result.keys.reserve(src.keys.size());
            for (const auto& k: src.keys) {
               result.keys.push_back(k);
            }

            return result;
         }
      };

      template<>
      struct shared_converter<producer_authority> {
         static shared_producer_authority convert (chainbase::allocator<char> alloc, const producer_authority& src) {
            auto authority = src.authority.visit([&alloc](const auto& a) {
                return shared_convert(alloc, a);
            });

            return shared_producer_authority(src.producer_name, std::move(authority));
         }

      };

      template<>
      struct shared_converter<shared_producer_authority> {
         static producer_authority convert (const shared_producer_authority& src) {
            producer_authority result;
            result.producer_name = src.producer_name;
            result.authority = src.authority.visit([](const auto& a) {
               return shared_convert(a);
            });

            return result;
         }
      };
   }

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
   struct producer_schedule_change_extension : producer_authority_schedule, fc::reflect_init {

      static constexpr uint16_t extension_id() { return 1; }
      static constexpr bool     enforce_unique() { return true; }
      void reflector_init() {
         static_assert( fc::raw::has_feature_reflector_init_on_unpacked_reflected_types, "producer_schedule_extension expects FC to support reflector_init" );
      }

      producer_schedule_change_extension() = default;
      producer_schedule_change_extension(const producer_schedule_change_extension&) = default;
      producer_schedule_change_extension( producer_schedule_change_extension&& ) = default;

      producer_schedule_change_extension( const producer_authority_schedule& sched )
      :producer_authority_schedule(sched) {}
   };



   struct shared_producer_authority_schedule {
      shared_producer_authority_schedule( chainbase::allocator<char> alloc )
      :producers(alloc){}

      shared_producer_authority_schedule( const shared_producer_authority_schedule& ) = default;
      shared_producer_authority_schedule( shared_producer_authority_schedule&& ) = default;
      shared_producer_authority_schedule& operator= ( shared_producer_authority_schedule && ) = default;
      shared_producer_authority_schedule& operator= ( const shared_producer_authority_schedule & ) = default;


      shared_producer_authority_schedule& operator=( const producer_authority_schedule& a ) {
         version = a.version;
         producers.clear();
         producers.reserve( a.producers.size() );
         for( const auto& p : a.producers ) {
            producers.emplace_back(detail::shared_convert(producers.get_allocator(), p));
         }
         return *this;
      }

      explicit operator producer_authority_schedule()const {
         producer_authority_schedule result;
         result.version = version;
         result.producers.reserve(producers.size());
         for( const auto& p : producers ) {
            result.producers.emplace_back(detail::shared_convert(p));
         }

         return result;
      }

      void clear() {
         version = 0;
         producers.clear();
      }

      uint32_t                                       version = 0; ///< sequentially incrementing version number
      shared_vector<shared_producer_authority>       producers;
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

