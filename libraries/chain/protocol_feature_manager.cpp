/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */

#include <eosio/chain/protocol_feature_manager.hpp>
#include <eosio/chain/exceptions.hpp>

#include <algorithm>
#include <boost/assign/list_of.hpp>

namespace eosio { namespace chain {

   const std::unordered_map<builtin_protocol_feature_t, builtin_protocol_feature_spec>
   builtin_protocol_feature_codenames =
      boost::assign::map_list_of<builtin_protocol_feature_t, builtin_protocol_feature_spec>
         ( builtin_protocol_feature_t::preactivate_feature, {
            "PREACTIVATE_FEATURE",
            digest_type{},
            {},
            {time_point{}, false, true} // enabled without preactivation and ready to go at any time
         } )
   ;


   const char* builtin_protocol_feature_codename( builtin_protocol_feature_t codename ) {
      auto itr = builtin_protocol_feature_codenames.find( codename );
      EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Unsupported builtin_protocol_feature_t passed to builtin_protocol_feature_codename: ${codename}",
                  ("codename", static_cast<uint32_t>(codename)) );

      return itr->second.codename;
   }

   protocol_feature_base::protocol_feature_base( protocol_feature_t feature_type,
                                                 const digest_type& description_digest,
                                                 flat_set<digest_type>&& dependencies,
                                                 const protocol_feature_subjective_restrictions& restrictions )
   :description_digest( description_digest )
   ,dependencies( std::move(dependencies) )
   ,subjective_restrictions( restrictions )
   ,_type( feature_type )
   {
      switch( feature_type ) {
         case protocol_feature_t::builtin:
            protocol_feature_type = builtin_protocol_feature::feature_type_string;
         break;
         default:
         {
            EOS_THROW( protocol_feature_validation_exception,
                       "Unsupported protocol_feature_t passed to constructor: ${type}",
                       ("type", static_cast<uint32_t>(feature_type)) );
         }
         break;
      }
   }

   void protocol_feature_base::reflector_init() {
      static_assert( fc::raw::has_feature_reflector_init_on_unpacked_reflected_types,
                     "protocol_feature_activation expects FC to support reflector_init" );

      if( protocol_feature_type == builtin_protocol_feature::feature_type_string ) {
         _type = protocol_feature_t::builtin;
      } else {
         EOS_THROW( protocol_feature_validation_exception,
                    "Unsupported protocol feature type: ${type}", ("type", protocol_feature_type) );
      }
   }

   builtin_protocol_feature::builtin_protocol_feature( builtin_protocol_feature_t codename,
                                                       const digest_type& description_digest,
                                                       flat_set<digest_type>&& dependencies,
                                                       const protocol_feature_subjective_restrictions& restrictions )
   :protocol_feature_base( protocol_feature_t::builtin, description_digest, std::move(dependencies), restrictions )
   ,_codename(codename)
   {
      auto itr = builtin_protocol_feature_codenames.find( codename );
      EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Unsupported builtin_protocol_feature_t passed to constructor: ${codename}",
                  ("codename", static_cast<uint32_t>(codename)) );

      builtin_feature_codename = itr->second.codename;
   }

   void builtin_protocol_feature::reflector_init() {
      protocol_feature_base::reflector_init();

      auto codename = get_codename();
      for( const auto& p : builtin_protocol_feature_codenames ) {
         if( p.first ==  codename ) {
            EOS_ASSERT( builtin_feature_codename == p.second.codename,
                        protocol_feature_validation_exception,
                        "Deserialized protocol feature has invalid codename. Expected: ${expected}. Actual: ${actual}.",
                        ("expected", p.second.codename)
                        ("actual", protocol_feature_type)
            );
            _codename = p.first;
            return;
         }
      }

      EOS_THROW( protocol_feature_validation_exception,
                  "Unsupported protocol feature type: ${type}", ("type", protocol_feature_type) );
   }


   digest_type builtin_protocol_feature::digest()const {
      digest_type::encoder enc;
      fc::raw::pack( enc, _type );
      fc::raw::pack( enc, description_digest  );
      fc::raw::pack( enc, _codename );

      return enc.result();
   }

   protocol_feature_manager::protocol_feature_manager() {
      _builtin_protocol_features.reserve( builtin_protocol_feature_codenames.size() );
   }

   protocol_feature_manager::recognized_t
   protocol_feature_manager::is_recognized( const digest_type& feature_digest, time_point now )const {
      auto itr = _recognized_protocol_features.find( feature_digest );

      if( itr == _recognized_protocol_features.end() )
         return recognized_t::unrecognized;

      if( !itr->enabled )
         return recognized_t::disabled;

      if( itr->earliest_allowed_activation_time > now )
         return recognized_t::too_early;

      if( itr->preactivation_required )
         return recognized_t::ready_if_preactivated;

      return recognized_t::ready;
   }

   const protocol_feature_manager::protocol_feature&
   protocol_feature_manager::get_protocol_feature( const digest_type& feature_digest )const {
      auto itr = _recognized_protocol_features.find( feature_digest );

      EOS_ASSERT( itr != _recognized_protocol_features.end(), protocol_feature_exception,
                  "unrecognized protocol feature with digest: ${digest}",
                  ("digest", feature_digest)
      );

      return *itr;
   }

   bool protocol_feature_manager::is_builtin_activated( builtin_protocol_feature_t feature_codename,
                                                        uint32_t current_block_num )const
   {
      uint32_t indx = static_cast<uint32_t>( feature_codename );

      if( indx >= _builtin_protocol_features.size() ) return false;

      return (_builtin_protocol_features[indx].activation_block_num <= current_block_num);
   }

   optional<digest_type>
   protocol_feature_manager::get_builtin_digest( builtin_protocol_feature_t feature_codename )const
   {
      uint32_t indx = static_cast<uint32_t>( feature_codename );

      if( indx >= _builtin_protocol_features.size() )
         return {};

      if( _builtin_protocol_features[indx].iterator_to_protocol_feature == _recognized_protocol_features.end() )
         return {};

      return _builtin_protocol_features[indx].iterator_to_protocol_feature->feature_digest;
   }

   bool protocol_feature_manager::validate_dependencies(
                                    const digest_type& feature_digest,
                                    const std::function<bool(const digest_type&)>& validator
   )const {
      auto itr = _recognized_protocol_features.find( feature_digest );

      if( itr == _recognized_protocol_features.end() ) return false;

      for( const auto& d : itr->dependencies ) {
         if( !validator(d) ) return false;
      }

      return true;
   }

   builtin_protocol_feature
   protocol_feature_manager::make_default_builtin_protocol_feature( builtin_protocol_feature_t codename )const
   {
      auto itr = builtin_protocol_feature_codenames.find( codename );

      EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Unsupported builtin_protocol_feature_t: ${codename}",
                  ("codename", static_cast<uint32_t>(codename)) );

      flat_set<digest_type> dependencies;
      dependencies.reserve( itr->second.builtin_dependencies.size() );

      for( const auto& d : itr->second.builtin_dependencies ) {
         auto dependency_digest = get_builtin_digest( d );
         EOS_ASSERT( dependency_digest, protocol_feature_exception,
                     "cannot make default builtin protocol feature with codename '${codename}' since it has a dependency that has not been added yet: ${dependency_codename}",
                     ("codename", static_cast<uint32_t>(itr->first))
                     ("dependency_codename", static_cast<uint32_t>(d))
         );
         dependencies.insert( *dependency_digest );
      }

      return {itr->first, itr->second.description_digest, std::move(dependencies), itr->second.subjective_restrictions};
   }

   void protocol_feature_manager::add_feature( const builtin_protocol_feature& f ) {
      EOS_ASSERT( _head_of_builtin_activation_list == builtin_protocol_feature_entry::no_previous,
                  protocol_feature_exception,
                  "new builtin protocol features cannot be added after a protocol feature has already been activated" );

      auto builtin_itr = builtin_protocol_feature_codenames.find( f._codename );
      EOS_ASSERT( builtin_itr != builtin_protocol_feature_codenames.end(), protocol_feature_validation_exception,
                  "Builtin protocol feature has unsupported builtin_protocol_feature_t: ${codename}",
                  ("codename", static_cast<uint32_t>( f._codename )) );

      uint32_t indx = static_cast<uint32_t>( f._codename );

      if( indx < _builtin_protocol_features.size() ) {
         EOS_ASSERT( _builtin_protocol_features[indx].iterator_to_protocol_feature == _recognized_protocol_features.end(),
                     protocol_feature_exception,
                     "builtin protocol feature with codename '${codename}' already added",
                     ("codename", f.builtin_feature_codename) );
      }

      auto feature_digest = f.digest();

      const auto& expected_builtin_dependencies = builtin_itr->second.builtin_dependencies;
      flat_set<builtin_protocol_feature_t> satisfied_builtin_dependencies;
      satisfied_builtin_dependencies.reserve( expected_builtin_dependencies.size() );

      for( const auto& d : f.dependencies ) {
         auto itr = _recognized_protocol_features.find( d );
         EOS_ASSERT( itr != _recognized_protocol_features.end(), protocol_feature_exception,
            "builtin protocol feature with codename '${codename}' and digest of ${digest} has a dependency on a protocol feature with digest ${dependency_digest} that is not recognized",
            ("codename", f.builtin_feature_codename)
            ("digest",  feature_digest)
            ("dependency_digest", d )
         );

         if( itr->builtin_feature
             && expected_builtin_dependencies.find( *itr->builtin_feature )
                  != expected_builtin_dependencies.end() )
         {
            satisfied_builtin_dependencies.insert( *itr->builtin_feature );
         }
      }

      if( expected_builtin_dependencies.size() > satisfied_builtin_dependencies.size() ) {
         flat_set<builtin_protocol_feature_t> missing_builtins;
         missing_builtins.reserve( expected_builtin_dependencies.size() - satisfied_builtin_dependencies.size() );
         std::set_difference( expected_builtin_dependencies.begin(), expected_builtin_dependencies.end(),
                              satisfied_builtin_dependencies.begin(), satisfied_builtin_dependencies.end(),
                              end_inserter( missing_builtins )
         );

         vector<string> missing_builtins_with_names;
         missing_builtins_with_names.reserve( missing_builtins.size() );
         for( const auto& builtin_codename : missing_builtins ) {
            auto itr = builtin_protocol_feature_codenames.find( builtin_codename );
            EOS_ASSERT( itr != builtin_protocol_feature_codenames.end(),
                        protocol_feature_exception,
                        "Unexpected error"
            );
            missing_builtins_with_names.emplace_back( itr->second.codename );
         }

         EOS_THROW(  protocol_feature_validation_exception,
                     "Not all the builtin dependencies of the builtin protocol feature with codename '${codename}' and digest of ${digest} were satisfied.",
                     ("missing_dependencies", missing_builtins_with_names)
         );
      }

      auto res = _recognized_protocol_features.insert( protocol_feature{
         feature_digest,
         f.dependencies,
         f.subjective_restrictions.earliest_allowed_activation_time,
         f.subjective_restrictions.preactivation_required,
         f.subjective_restrictions.enabled,
         f._codename
      } );

      EOS_ASSERT( res.second, protocol_feature_exception,
                  "builtin protocol feature with codename '${codename}' has a digest of ${digest} but another protocol feature with the same digest has already been added",
                  ("codename", f.builtin_feature_codename)("digest", feature_digest) );

      if( indx < _builtin_protocol_features.size() ) {
         for( auto i =_builtin_protocol_features.size(); i <= indx; ++i ) {
            _builtin_protocol_features.push_back( builtin_protocol_feature_entry{
                                                   _recognized_protocol_features.end(),
                                                   builtin_protocol_feature_entry::not_active
                                                  } );
         }
      }

      _builtin_protocol_features[indx].iterator_to_protocol_feature = res.first;
   }

   void protocol_feature_manager::activate_feature( const digest_type& feature_digest,
                                                    uint32_t current_block_num )
   {
      auto itr = _recognized_protocol_features.find( feature_digest );

      EOS_ASSERT( itr != _recognized_protocol_features.end(), protocol_feature_exception,
                  "unrecognized protocol feature digest: ${digest}", ("digest", feature_digest) );

      if( itr->builtin_feature ) {
         if( _head_of_builtin_activation_list != builtin_protocol_feature_entry::no_previous ) {
            auto largest_block_num_of_activated_builtins = _builtin_protocol_features[_head_of_builtin_activation_list].activation_block_num;
            EOS_ASSERT( largest_block_num_of_activated_builtins <= current_block_num,
                        protocol_feature_exception,
                        "trying to activate a builtin protocol feature with a current block number of "
                        "${current_block_num} when the largest activation block number of all activated builtin "
                        "protocol features is ${largest_block_num_of_activated_builtins}",
                        ("current_block_num", current_block_num)
                        ("largest_block_num_of_activated_builtins", largest_block_num_of_activated_builtins)
            );
         }

         uint32_t indx = static_cast<uint32_t>( *itr->builtin_feature );

         EOS_ASSERT( indx < _builtin_protocol_features.size() &&
                     _builtin_protocol_features[indx].iterator_to_protocol_feature != _recognized_protocol_features.end(),
                     protocol_feature_exception,
                     "invariant failure: problem with activating builtin protocol feature with digest: ${digest}",
                     ("digest", feature_digest) );

         EOS_ASSERT( _builtin_protocol_features[indx].activation_block_num == builtin_protocol_feature_entry::not_active,
                     protocol_feature_exception,
                     "cannot activate already activated builtin feature with digest: ${digest}",
                     ("digest", feature_digest) );

         _builtin_protocol_features[indx].activation_block_num = current_block_num;
         _builtin_protocol_features[indx].previous = _head_of_builtin_activation_list;
         _head_of_builtin_activation_list = indx;
      }
   }

   void protocol_feature_manager::popped_blocks_to( uint32_t block_num ) {
      while( _head_of_builtin_activation_list != builtin_protocol_feature_entry::no_previous ) {
         auto& e = _builtin_protocol_features[_head_of_builtin_activation_list];
         if( e.activation_block_num <= block_num ) break;

         _head_of_builtin_activation_list = e.previous;
         e.activation_block_num = builtin_protocol_feature_entry::not_active;
         e.previous = builtin_protocol_feature_entry::no_previous;
      }
   }

} }  // eosio::chain
