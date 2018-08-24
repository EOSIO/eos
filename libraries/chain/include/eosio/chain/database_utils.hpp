/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <chainbase/chainbase.hpp>
#include <fc/io/raw.hpp>


namespace eosio { namespace chain {

   template<typename ...Indices>
   class index_set;

   template<typename Index>
   class index_utils {
      public:
         using index_t = Index;

         template<typename F>
         static void walk( const chainbase::database& db, F function ) {
            auto const& index = db.get_index<Index>().indices();
            const auto& first = index.begin();
            const auto& last = index.end();
            for (auto itr = first; itr != last; ++itr) {
               function(*itr);
            }
         }
   };

   template<typename Index>
   class index_set<Index> {
   public:
      static void add_indices( chainbase::database& db ) {
         db.add_index<Index>();
      }

      template<typename F>
      static void walk_indices( F function ) {
         function( index_utils<Index>() );
      }
   };

   template<typename FirstIndex, typename ...RemainingIndices>
   class index_set<FirstIndex, RemainingIndices...> {
   public:
      static void add_indices( chainbase::database& db ) {
         index_set<FirstIndex>::add_indices(db);
         index_set<RemainingIndices...>::add_indices(db);
      }

      template<typename F>
      static void walk_indices( F function ) {
         index_set<FirstIndex>::walk_indices(function);
         index_set<RemainingIndices...>::walk_indices(function);
      }
   };

   template<typename DataStream, typename OidType>
   DataStream& operator << ( DataStream& ds, const chainbase::oid<OidType>& oid ) {
      fc::raw::pack(ds, oid._id);
      return ds;
   }
} }