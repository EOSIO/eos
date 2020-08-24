#pragma once

#include <eosio/chain/types.hpp>
#include <iterator>

namespace eosio { namespace chain {

enum class protocol_feature_t : uint32_t {
   builtin
};

enum class builtin_protocol_feature_t : uint32_t {
   preactivate_feature,
   only_link_to_existing_permission,
   replace_deferred,
   no_duplicate_deferred_id,
   fix_linkauth_restriction,
   disallow_empty_producer_schedule,
   restrict_action_to_self,
   only_bill_first_authorizer,
   forward_setcode,
   get_sender,
   ram_restrictions,
   webauthn_key,
   wtmsig_block_signatures,
   action_return_value,
   kv_database,
   configurable_wasm_limits,
   blockchain_parameters
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
   static const char* feature_type_string;

   builtin_protocol_feature() = default;

   builtin_protocol_feature( builtin_protocol_feature_t codename,
                             const digest_type& description_digest,
                             flat_set<digest_type>&& dependencies,
                             const protocol_feature_subjective_restrictions& restrictions );

   void reflector_init();

   digest_type digest()const;

   builtin_protocol_feature_t get_codename()const { return _codename; }

   friend class protocol_feature_set;

public:
   std::string                 builtin_feature_codename;
protected:
   builtin_protocol_feature_t  _codename;
};

struct protocol_feature {
   digest_type                           feature_digest;
   digest_type                           description_digest;
   flat_set<digest_type>                 dependencies;
   time_point                            earliest_allowed_activation_time;
   bool                                  preactivation_required = false;
   bool                                  enabled = false;
   optional<builtin_protocol_feature_t>  builtin_feature;

   fc::variant to_variant( bool include_subjective_restrictions = true,
                           fc::mutable_variant_object* additional_fields = nullptr )const;

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

class protocol_feature_set {
protected:
   using protocol_feature_set_type = std::set< protocol_feature, std::less<> >;

public:
   protocol_feature_set();

   enum class recognized_t {
      unrecognized,
      disabled,
      too_early,
      ready
   };

   recognized_t is_recognized( const digest_type& feature_digest, time_point now )const;

   optional<digest_type> get_builtin_digest( builtin_protocol_feature_t feature_codename )const;

   const protocol_feature& get_protocol_feature( const digest_type& feature_digest )const;

   bool validate_dependencies( const digest_type& feature_digest,
                               const std::function<bool(const digest_type&)>& validator )const;

   static builtin_protocol_feature
   make_default_builtin_protocol_feature(
      builtin_protocol_feature_t codename,
      const std::function<digest_type(builtin_protocol_feature_t dependency)>& handle_dependency
   );

   const protocol_feature& add_feature( const builtin_protocol_feature& f );

   class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const protocol_feature> {
   protected:
      protocol_feature_set_type::const_iterator _itr;

   protected:
      explicit const_iterator( protocol_feature_set_type::const_iterator itr )
      :_itr(itr)
      {}

      const protocol_feature* get_pointer()const { return &*_itr; }

      friend class protocol_feature_set;

   public:
      const_iterator() = default;

      friend bool operator == ( const const_iterator& lhs, const const_iterator& rhs ) {
         return (lhs._itr == rhs._itr);
      }

      friend bool operator != ( const const_iterator& lhs, const const_iterator& rhs ) {
         return (lhs._itr != rhs._itr);
      }

      const protocol_feature& operator*()const {
         return *get_pointer();
      }

      const protocol_feature* operator->()const {
         return get_pointer();
      }

      const_iterator& operator++() {
         ++_itr;
         return *this;
      }

      const_iterator& operator--() {
         --_itr;
         return *this;
      }

      const_iterator operator++(int) {
         const_iterator result(*this);
         ++(*this);
         return result;
      }

      const_iterator operator--(int) {
         const_iterator result(*this);
         --(*this);
         return result;
      }
   };

   using const_reverse_iterator = std::reverse_iterator<const_iterator>;

   const_iterator cbegin()const { return const_iterator( _recognized_protocol_features.cbegin() ); }
   const_iterator begin()const  { return cbegin(); }

   const_iterator cend()const   { return const_iterator( _recognized_protocol_features.cend() ); }
   const_iterator end()const    { return cend(); }

   const_reverse_iterator crbegin()const { return std::make_reverse_iterator( cend() ); }
   const_reverse_iterator rbegin()const  { return crbegin(); }

   const_reverse_iterator crend()const   { return std::make_reverse_iterator( cbegin() ); }
   const_reverse_iterator rend()const    { return crend(); }

   bool empty()const           { return _recognized_protocol_features.empty(); }
   std::size_t size()const     { return _recognized_protocol_features.size(); }
   std::size_t max_size()const { return _recognized_protocol_features.max_size(); }

   template<typename K>
   const_iterator find( const K& x )const {
      return const_iterator( _recognized_protocol_features.find( x ) );
   }

   template<typename K>
   const_iterator lower_bound( const K& x )const {
      return const_iterator( _recognized_protocol_features.lower_bound( x ) );
   }

   template<typename K>
   const_iterator upper_bound( const K& x )const {
      return const_iterator( _recognized_protocol_features.upper_bound( x ) );
   }

   friend class protocol_feature_manager;

protected:
   protocol_feature_set_type                         _recognized_protocol_features;
   vector<protocol_feature_set_type::const_iterator> _recognized_builtin_protocol_features;
};


class protocol_feature_manager {
public:

   protocol_feature_manager( protocol_feature_set&& pfs, std::function<fc::logger*()> get_deep_mind_logger );

   class const_iterator : public std::iterator<std::bidirectional_iterator_tag, const protocol_feature> {
   protected:
      const protocol_feature_manager* _pfm = nullptr;
      std::size_t                     _index = 0;

   protected:
      static constexpr std::size_t end_index =  std::numeric_limits<std::size_t>::max();

      explicit const_iterator( const protocol_feature_manager* pfm, std::size_t i = end_index )
      :_pfm(pfm)
      ,_index(i)
      {}

      const protocol_feature* get_pointer()const;

      friend class protocol_feature_manager;

   public:
      const_iterator() = default;

      friend bool operator == ( const const_iterator& lhs, const const_iterator& rhs ) {
         return std::tie( lhs._pfm, lhs._index ) == std::tie( rhs._pfm, rhs._index );
      }

      friend bool operator != ( const const_iterator& lhs, const const_iterator& rhs ) {
         return !(lhs == rhs);
      }

      uint32_t activation_ordinal()const;

      uint32_t activation_block_num()const;

      const protocol_feature& operator*()const {
         return *get_pointer();
      }

      const protocol_feature* operator->()const {
         return get_pointer();
      }

      const_iterator& operator++();

      const_iterator& operator--();

      const_iterator operator++(int) {
         const_iterator result(*this);
         ++(*this);
         return result;
      }

      const_iterator operator--(int) {
         const_iterator result(*this);
         --(*this);
         return result;
      }
   };

   friend class const_iterator;

   using const_reverse_iterator = std::reverse_iterator<const_iterator>;

   void init( chainbase::database& db );

   bool is_initialized()const { return _initialized; }

   const protocol_feature_set& get_protocol_feature_set()const { return _protocol_feature_set; }

   optional<digest_type> get_builtin_digest( builtin_protocol_feature_t feature_codename )const {
      return _protocol_feature_set.get_builtin_digest( feature_codename );
   }

   // All methods below require is_initialized() as a precondition.

   const_iterator cbegin()const;
   const_iterator begin()const  { return cbegin(); }

   const_iterator cend()const   { return const_iterator( this ); }
   const_iterator end()const    { return cend(); }

   const_reverse_iterator crbegin()const { return std::make_reverse_iterator( cend() ); }
   const_reverse_iterator rbegin()const  { return crbegin(); }

   const_reverse_iterator crend()const   { return std::make_reverse_iterator( cbegin() ); }
   const_reverse_iterator rend()const    { return crend(); }

   const_iterator at_activation_ordinal( uint32_t activation_ordinal )const;

   const_iterator lower_bound( uint32_t block_num )const;

   const_iterator upper_bound( uint32_t block_num )const;


   bool is_builtin_activated( builtin_protocol_feature_t feature_codename, uint32_t current_block_num )const;

   void activate_feature( const digest_type& feature_digest, uint32_t current_block_num );
   void popped_blocks_to( uint32_t block_num );

protected:

   struct protocol_feature_entry {
      protocol_feature_set::const_iterator  iterator_to_protocol_feature;
      uint32_t                              activation_block_num;
   };

   struct builtin_protocol_feature_entry {
      static constexpr size_t   no_previous =  std::numeric_limits<size_t>::max();
      static constexpr uint32_t not_active  = std::numeric_limits<uint32_t>::max();

      size_t                               previous = no_previous;
      uint32_t                             activation_block_num = not_active;
   };

protected:
   protocol_feature_set                   _protocol_feature_set;
   vector<protocol_feature_entry>         _activated_protocol_features;
   vector<builtin_protocol_feature_entry> _builtin_protocol_features;
   size_t                                 _head_of_builtin_activation_list = builtin_protocol_feature_entry::no_previous;
   bool                                   _initialized = false;

private:
   std::function<fc::logger*()>           _get_deep_mind_logger;
};

} } // namespace eosio::chain

FC_REFLECT(eosio::chain::protocol_feature_subjective_restrictions,
               (earliest_allowed_activation_time)(preactivation_required)(enabled))

// @ignore _type
FC_REFLECT(eosio::chain::protocol_feature_base,
               (protocol_feature_type)(description_digest)(dependencies)(subjective_restrictions))

// @ignore _codename
FC_REFLECT_DERIVED(eosio::chain::builtin_protocol_feature, (eosio::chain::protocol_feature_base),
                     (builtin_feature_codename))
