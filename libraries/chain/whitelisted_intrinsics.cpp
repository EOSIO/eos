#include <eosio/chain/whitelisted_intrinsics.hpp>
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

   whitelisted_intrinsics_type::iterator
   find_intrinsic( whitelisted_intrinsics_type& whitelisted_intrinsics, uint64_t h, std::string_view name )
   {
      auto itr = whitelisted_intrinsics.lower_bound( h );
      const auto end = whitelisted_intrinsics.end();

      if( !find_intrinsic_helper( h, name, itr, end ) )
         return end;

      return itr;
   }

   whitelisted_intrinsics_type::const_iterator
   find_intrinsic( const whitelisted_intrinsics_type& whitelisted_intrinsics, uint64_t h, std::string_view name )
   {
      auto itr = whitelisted_intrinsics.lower_bound( h );
      const auto end = whitelisted_intrinsics.end();

      if( !find_intrinsic_helper( h, name, itr, end ) )
         return end;

      return itr;
   }

   bool is_intrinsic_whitelisted( const whitelisted_intrinsics_type& whitelisted_intrinsics, std::string_view name )
   {
      uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
      auto itr = whitelisted_intrinsics.lower_bound( h );
      const auto end = whitelisted_intrinsics.end();

      return find_intrinsic_helper( h, name, itr, end );
   }


   void add_intrinsic_to_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, std::string_view name )
   {
      uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
      auto itr = find_intrinsic( whitelisted_intrinsics, h, name );
      EOS_ASSERT( itr == whitelisted_intrinsics.end(), database_exception,
                  "cannot add intrinsic '${name}' since it already exists in the whitelist",
                  ("name", std::string(name))
      );

      whitelisted_intrinsics.emplace( std::piecewise_construct,
                                      std::forward_as_tuple( h ),
                                      std::forward_as_tuple( name.data(), name.size(),
                                                             whitelisted_intrinsics.get_allocator() )
      );
   }

   void remove_intrinsic_from_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics, std::string_view name )
   {
      uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
      auto itr = find_intrinsic( whitelisted_intrinsics, h, name );
      EOS_ASSERT( itr != whitelisted_intrinsics.end(), database_exception,
                  "cannot remove intrinsic '${name}' since it does not exist in the whitelist",
                  ("name", std::string(name))
      );

      whitelisted_intrinsics.erase( itr );
   }

   void reset_intrinsic_whitelist( whitelisted_intrinsics_type& whitelisted_intrinsics,
                                   const std::set<std::string>& s )
   {
      whitelisted_intrinsics.clear();

      for( const auto& name : s ) {
         uint64_t h = static_cast<uint64_t>( std::hash<std::string_view>{}( name ) );
         whitelisted_intrinsics.emplace( std::piecewise_construct,
                                         std::forward_as_tuple( h ),
                                         std::forward_as_tuple( name.data(), name.size(),
                                                                whitelisted_intrinsics.get_allocator() )
         );
      }
   }

   std::set<std::string> convert_intrinsic_whitelist_to_set( const whitelisted_intrinsics_type& whitelisted_intrinsics ) {
      std::set<std::string> s;

      for( const auto& p : whitelisted_intrinsics ) {
         s.emplace( p.second.data(), p.second.size() );
      }

      return s;
   }

} }
