/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

enum class protocol_feature_t : uint32_t {
   builtin
};

enum class builtin_protocol_feature_t : uint32_t {
   preactivate_feature,
};

struct protocol_feature_subjective_restrictions {
   time_point             earliest_allowed_activation_time;
   bool                   preactivation_required = false;
   bool                   enabled = false;
};

struct builtin_protocol_feature_spec {
   const char*                               codename = nullptr;
   digest_type                               description_digest;
   flat_set<builtin_protocol_feature_t>      builtin_dependencies;
   protocol_feature_subjective_restrictions  subjective_restrictions{time_point{}, true, true};
};

template<typename T>
struct enum_hash
{
   static_assert( std::is_enum<T>::value, "enum_hash can only be used on enumeration types" );
   
   using underlying_type = typename std::underlying_type<T>::type;

   std::size_t operator()(T t) const
   {
        return std::hash<underlying_type>{}( static_cast<underlying_type>(t) );
   }
};

// enum_hash needed to support old gcc compiler of Ubuntu 16.04

extern const std::unordered_map<builtin_protocol_feature_t, builtin_protocol_feature_spec, enum_hash<builtin_protocol_feature_t>> builtin_protocol_feature_codenames;

const char* builtin_protocol_feature_codename( builtin_protocol_feature_t );

class protocol_feature_base : public fc::reflect_init {
public:
   protocol_feature_base() = default;

   protocol_feature_base( protocol_feature_t feature_type,
                          const digest_type& description_digest,
                          flat_set<digest_type>&& dependencies,
                          const protocol_feature_subjective_restrictions& restrictions );

   void reflector_init();

   protocol_feature_t get_type()const { return _type; }

public:
   std::string                               protocol_feature_type;
   digest_type                               description_digest;
   flat_set<digest_type>                     dependencies;
   protocol_feature_subjective_restrictions  subjective_restrictions;
protected:
   protocol_feature_t                        _type;
};

class builtin_protocol_feature : public protocol_feature_base {
public:
   static constexpr const char* feature_type_string = "builtin";

   builtin_protocol_feature() = default;

   builtin_protocol_feature( builtin_protocol_feature_t codename,
                             const digest_type& description_digest,
                             flat_set<digest_type>&& dependencies,
                             const protocol_feature_subjective_restrictions& restrictions );

   void reflector_init();

   digest_type digest()const;

   builtin_protocol_feature_t get_codename()const { return _codename; }

   friend class protocol_feature_manager;

public:
   std::string                 builtin_feature_codename;
protected:
   builtin_protocol_feature_t  _codename;
};

class protocol_feature_manager {
public:

   protocol_feature_manager();

   enum class recognized_t {
      unrecognized,
      disabled,
      too_early,
      ready_if_preactivated,
      ready
   };

   struct protocol_feature {
      digest_type                           feature_digest;
      flat_set<digest_type>                 dependencies;
      time_point                            earliest_allowed_activation_time;
      bool                                  preactivation_required = false;
      bool                                  enabled = false;
      optional<builtin_protocol_feature_t>  builtin_feature;

      friend bool operator <( const protocol_feature& lhs, const protocol_feature& rhs ) {
         return lhs.feature_digest < rhs.feature_digest;
      }

      friend bool operator <( const digest_type& lhs, const protocol_feature& rhs ) {
         return lhs < rhs.feature_digest;
      }

      friend bool operator <( const protocol_feature& lhs, const digest_type& rhs ) {
         return lhs.feature_digest < rhs;
      }
   };

   recognized_t is_recognized( const digest_type& feature_digest, time_point now )const;

   const protocol_feature& get_protocol_feature( const digest_type& feature_digest )const;

   bool is_builtin_activated( builtin_protocol_feature_t feature_codename, uint32_t current_block_num )const;

   optional<digest_type> get_builtin_digest( builtin_protocol_feature_t feature_codename )const;

   bool validate_dependencies( const digest_type& feature_digest,
                               const std::function<bool(const digest_type&)>& validator )const;

   builtin_protocol_feature make_default_builtin_protocol_feature(
                              builtin_protocol_feature_t codename,
                              const std::function<void(builtin_protocol_feature_t dependency)>& handle_dependency
                            )const;

   void add_feature( const builtin_protocol_feature& f );

   void activate_feature( const digest_type& feature_digest, uint32_t current_block_num );
   void popped_blocks_to( uint32_t block_num );

protected:
   using protocol_feature_set_type = std::set<protocol_feature, std::less<>>;

   struct builtin_protocol_feature_entry {
      static constexpr uint32_t not_active  = std::numeric_limits<uint32_t>::max();
      static constexpr size_t   no_previous =  std::numeric_limits<size_t>::max();

      protocol_feature_set_type::iterator iterator_to_protocol_feature;
      uint32_t                            activation_block_num = not_active;
      size_t                              previous = no_previous;
   };

protected:
   protocol_feature_set_type              _recognized_protocol_features;
   vector<builtin_protocol_feature_entry> _builtin_protocol_features;
   size_t                                 _head_of_builtin_activation_list = builtin_protocol_feature_entry::no_previous;
};

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::protocol_feature_subjective_restrictions,
               (earliest_allowed_activation_time)(preactivation_required)(enabled))

FC_REFLECT(eosio::chain::protocol_feature_base,
               (protocol_feature_type)(dependencies)(description_digest)(subjective_restrictions))

FC_REFLECT_DERIVED(eosio::chain::builtin_protocol_feature, (eosio::chain::protocol_feature_base),
                     (builtin_feature_codename))
