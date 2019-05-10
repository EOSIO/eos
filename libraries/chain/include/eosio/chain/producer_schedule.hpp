#pragma once
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>
#include <chainbase/chainbase.hpp>
#include <eosio/chain/authority.hpp>

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

      friend bool operator == ( const block_signing_authority_v0& lhs, const block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) == tie( rhs.threshold, rhs.keys );
      }
      friend bool operator != ( const block_signing_authority_v0& lhs, const block_signing_authority_v0& rhs ) {
         return tie( lhs.threshold, lhs.keys ) != tie( rhs.threshold, rhs.keys );
      }
   };

   using block_signing_authority = static_variant<block_signing_authority_v0>;

   struct producer_authority {
      name                    name;
      block_signing_authority authority;

      friend bool operator == ( const producer_authority& lhs, const producer_authority& rhs ) {
         return tie( lhs.name, lhs.authority ) == tie( rhs.name, rhs.authority );
      }
      friend bool operator != ( const producer_authority& lhs, const producer_authority& rhs ) {
         return tie( lhs.name, lhs.authority ) != tie( rhs.name, rhs.authority );
      }
   };

   struct shared_block_signing_authority_v0 {
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
      name                           name;
      shared_block_signing_authority authority;

      friend bool operator == ( const shared_producer_authority& lhs, const shared_producer_authority& rhs ) {
         return tie( lhs.name, lhs.authority ) == tie( rhs.name, rhs.authority );
      }
      friend bool operator != ( const shared_producer_authority& lhs, const shared_producer_authority& rhs ) {
         return tie( lhs.name, lhs.authority ) != tie( rhs.name, rhs.authority );
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
            shared_producer_authority result;
            result.name = src.name;
            result.authority = src.authority.visit([&result, &alloc](const auto& a) {
                return shared_convert(alloc, a);
            });

            return result;
         }

      };

      template<>
      struct shared_converter<shared_producer_authority> {
         static producer_authority convert (const shared_producer_authority& src) {
            producer_authority result;
            result.name = src.name;
            result.authority = src.authority.visit([&result](const auto& a) {
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

      producer_schedule_change_extension( producer_authority_schedule&& sched )
      :producer_authority_schedule(sched) {}
   };



   struct shared_producer_authority_schedule {
      shared_producer_authority_schedule( chainbase::allocator<char> alloc )
      :producers(alloc){}

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


} } /// eosio::chain

FC_REFLECT( eosio::chain::legacy::producer_key, (producer_name)(block_signing_key) )
FC_REFLECT( eosio::chain::legacy::producer_schedule_type, (version)(producers) )
FC_REFLECT( eosio::chain::block_signing_authority_v0, (threshold)(keys))
FC_REFLECT( eosio::chain::producer_authority, (name)(authority) )
FC_REFLECT( eosio::chain::producer_authority_schedule, (version)(producers) )
FC_REFLECT_DERIVED( eosio::chain::producer_schedule_change_extension, (eosio::chain::producer_authority_schedule), )
FC_REFLECT( eosio::chain::shared_producer_authority_schedule, (version)(producers) )

