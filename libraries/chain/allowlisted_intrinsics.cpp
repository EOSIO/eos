#include <eosio/chain/allowlisted_intrinsics.hpp>
#include <eosio/chain/exceptions.hpp>

namespace eosio { namespace chain {

   template<typename Iterator>
   bool find_intrinsic_helper( uint64_t h, std::string_view name, Iterator& itr, const Iterator& end ) {
      for( ; itr != end && itr->first == h; ++itr ) {
         if( itr->second.compare( 0, itr->second.size(), name.data(), name.size() ) == 0 ) {
            return true;
         }
      }

      return false;
   }

   allowlisted_intrinsics_type::iterator
   find_intrinsic( allowlisted_intrinsics_type& allowlisted_intrinsics, uint64_t h, std::string_view name )
   {
      auto itr = allowlisted_intrinsics.lower_bound( h );
      const auto end = allowlisted_intrinsics.end();

      if( !find_intrinsic_helper( h, name, itr, end ) )
         return end;

      return itr;
   }

   allowlisted_intrinsics_type::const_iterator
   find_intrinsic( const allowlisted_intrinsics_type& allowlisted_intrinsics, uint64_t h, std::string_view name )
   {
      auto itr = allowlisted_intrinsics.lower_bound( h );
      const auto end = allowlisted_intrinsics.end();

      if( !find_intrinsic_helper( h, name, itr, end ) )
         return end;

      return itr;
   }

   bool is_intrinsic_allowlisted( const allowlisted_intrinsics_type& allowlisted_intrinsics, std::string_view name )
   {
      uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
      auto itr = allowlisted_intrinsics.lower_bound( h );
      const auto end = allowlisted_intrinsics.end();

      return find_intrinsic_helper( h, name, itr, end );
   }


   void add_intrinsic_to_allowlist( allowlisted_intrinsics_type& allowlisted_intrinsics, std::string_view name )
   {
      uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
      auto itr = find_intrinsic( allowlisted_intrinsics, h, name );
      EOS_ASSERT( itr == allowlisted_intrinsics.end(), database_exception,
                  "cannot add intrinsic '${name}' since it already exists in the allowlist",
                  ("name", std::string(name))
      );

      allowlisted_intrinsics.emplace( std::piecewise_construct,
                                      std::forward_as_tuple( h ),
                                      std::forward_as_tuple( name.data(), name.size(),
                                                             allowlisted_intrinsics.get_allocator() )
      );
   }

   void remove_intrinsic_from_allowlist( allowlisted_intrinsics_type& allowlisted_intrinsics, std::string_view name )
   {
      uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
      auto itr = find_intrinsic( allowlisted_intrinsics, h, name );
      EOS_ASSERT( itr != allowlisted_intrinsics.end(), database_exception,
                  "cannot remove intrinsic '${name}' since it does not exist in the allowlist",
                  ("name", std::string(name))
      );

      allowlisted_intrinsics.erase( itr );
   }

   void reset_intrinsic_allowlist( allowlisted_intrinsics_type& allowlisted_intrinsics,
                                   const std::set<std::string>& s )
   {
      allowlisted_intrinsics.clear();

      for( const auto& name : s ) {
         uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
         allowlisted_intrinsics.emplace( std::piecewise_construct,
                                         std::forward_as_tuple( h ),
                                         std::forward_as_tuple( name.data(), name.size(),
                                                                allowlisted_intrinsics.get_allocator() )
         );
      }
   }

   std::set<std::string> convert_intrinsic_allowlist_to_set( const allowlisted_intrinsics_type& allowlisted_intrinsics ) {
      std::set<std::string> s;

      for( const auto& p : allowlisted_intrinsics ) {
         s.emplace( p.second.data(), p.second.size() );
      }

      return s;
   }

} }
