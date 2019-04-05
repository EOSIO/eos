#pragma once
#include <eosio/chain/config.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/authority.hpp>
#include <chainbase/chainbase.hpp>

namespace eosio { namespace chain {

   /**
    *  Used as part of the producer_schedule_type_v1, maps the producer name to their block signing authority.
    */
   struct producer_key_v1 {
   public:
      producer_key_v1( account_name producer_name, const block_signing_authority& auth )
      :producer_name( producer_name )
      ,auth( auth )
      {}

      producer_key_v1( account_name producer_name, block_signing_authority&& auth )
      :producer_name( producer_name )
      ,auth( std::move(auth) )
      {}

      producer_key_v1() = default;

      friend bool operator == ( const producer_key_v1& lhs, const producer_key_v1& rhs ) {
         return tie( lhs.producer_name, lhs.auth ) == tie( rhs.producer_name, rhs.auth );
      }

      friend bool operator != ( const producer_key_v1& lhs, const producer_key_v1& rhs ) {
         return !(lhs == rhs);
      }

   public:
      account_name                    producer_name;
      block_signing_authority         auth;
   };

   /**
    *  Used as part of the producer_schedule_type_v0, maps the producer name to their key.
    */
   struct producer_key_v0 {
   public:
      friend bool operator == ( const producer_key_v0& lhs, const producer_key_v0& rhs ) {
         return tie( lhs.producer_name, lhs.block_signing_key ) == tie( rhs.producer_name, rhs.block_signing_key );
      }

      friend bool operator != ( const producer_key_v0& lhs, const producer_key_v0& rhs ) {
         return !(lhs == rhs);
      }

      producer_key_v1 to_producer_key_v1()const {
         producer_key_v1 result;
         result.producer_name = producer_name;
         result.auth = block_signing_authority(block_signing_key);
         return result;
      }

   public:
      account_name      producer_name;
      public_key_type   block_signing_key;
   };

   /**
    *  Used as part of the shared_producer_schedule_v1, maps the producer name to their block signing authority.
    */
   struct shared_producer_key_v1 {
   public:
      shared_producer_key_v1( chainbase::allocator<char> alloc )
      :auth(alloc)
      {}

      shared_producer_key_v1& operator=(const producer_key_v1& pk) {
         producer_name = pk.producer_name;
         auth = pk.auth;
         return *this;
      }

      operator producer_key_v1()const { return to_producer_key_v1(); }

      producer_key_v1 to_producer_key_v1()const {
         producer_key_v1 result;
         result.producer_name = producer_name;
         result.auth = auth.to_block_signing_authority();
         return result;
      }

   public:
      account_name                    producer_name;
      shared_block_signing_authority  auth;
   };

   struct no_producer_schedule {};

   /**
    *  Defines both the order, account name, and block signing authorities of the active set of producers.
    */
   struct producer_schedule_v1 {
   public:
      friend bool operator == ( const producer_schedule_v1& lhs, const producer_schedule_v1& rhs ) {
         return tie( lhs.version, lhs.producers ) == tie( rhs.version, rhs.producers );
      }

      friend bool operator != ( const producer_schedule_v1& lhs, const producer_schedule_v1& rhs ) {
         return !(lhs == rhs);
      }

   public:
      uint32_t                                       version = 0; ///< sequentially incrementing version number
      vector<producer_key_v1>                        producers;
   };

   /**
    *  Defines both the order, account name, and signing keys of the active set of producers.
    */
   struct producer_schedule_v0 {
   public:
      public_key_type get_producer_key( account_name p )const {
         for( const auto& i : producers )
            if( i.producer_name == p )
               return i.block_signing_key;
         return public_key_type();
      }

      friend bool operator == ( const producer_schedule_v0& lhs, const producer_schedule_v0& rhs ) {
         return tie( lhs.version, lhs.producers ) == tie( rhs.version, rhs.producers );
      }

      friend bool operator != ( const producer_schedule_v0& lhs, const producer_schedule_v0& rhs ) {
         return !(lhs == rhs);
      }

      producer_schedule_v1 to_producer_schedule_v1()const {
         producer_schedule_v1 result;
         result.version = version;
         result.producers.reserve(producers.size());
         for( const auto& p : producers )
            result.producers.push_back( p.to_producer_key_v1() );
         return result;
      }

   public:
      uint32_t                                       version = 0; ///< sequentially incrementing version number
      vector<producer_key_v0>                        producers;
   };

   using optional_producer_schedule = static_variant<no_producer_schedule, producer_schedule_v0, producer_schedule_v1>;

   struct shared_producer_schedule_v1 {
   public:
      shared_producer_schedule_v1( chainbase::allocator<char> alloc )
      :producers(alloc){}

      shared_producer_schedule_v1& operator=( const producer_schedule_v0& s ) {
         version = s.version;
         producers.clear();
         producers.reserve( s.producers.size() );
         for( const auto& p : s.producers ) {
            producers.emplace_back( producers.get_allocator() );
            producers.back() = p.to_producer_key_v1();
         }
         return *this;
      }

      shared_producer_schedule_v1& operator=( const producer_schedule_v1& s ) {
         version = s.version;
         producers.clear();
         producers.reserve( s.producers.size() );
         for( const auto& p : s.producers ) {
            producers.emplace_back( producers.get_allocator() );
            producers.back() = p;
         }
         return *this;
      }

      operator producer_schedule_v1()const { return to_producer_schedule_v1(); }

      producer_schedule_v1 to_producer_schedule_v1()const {
         producer_schedule_v1 result;
         result.version = version;
         result.producers.reserve(producers.size());
         for( const auto& p : producers )
            result.producers.push_back(p);
         return result;
      }

      void clear() {
         version = 0;
         producers.clear();
      }

   public:
      uint32_t                                 version = 0; ///< sequentially incrementing version number
      shared_vector<shared_producer_key_v1>    producers;
   };

} } /// eosio::chain

FC_REFLECT( eosio::chain::producer_key_v0, (producer_name)(block_signing_key) )
FC_REFLECT( eosio::chain::producer_key_v1, (producer_name)(auth) )
FC_REFLECT( eosio::chain::shared_producer_key_v1, (producer_name)(auth) )
FC_REFLECT( eosio::chain::no_producer_schedule, BOOST_PP_SEQ_NIL )
FC_REFLECT( eosio::chain::producer_schedule_v0, (version)(producers) )
FC_REFLECT( eosio::chain::producer_schedule_v1, (version)(producers) )
FC_REFLECT( eosio::chain::shared_producer_schedule_v1, (version)(producers) )
