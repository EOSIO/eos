/**
 *  @file
 *  @copyright defined in eos/LICENSE
 */
#pragma once
#include <eosio/chain/name.hpp>
#include <eosio/chain/chain_id_type.hpp>

#include <chainbase/chainbase.hpp>

#include <fc/interprocess/container.hpp>
#include <fc/io/varint.hpp>
#include <fc/io/enum_type.hpp>
#include <fc/crypto/sha224.hpp>
#include <fc/optional.hpp>
#include <fc/safe.hpp>
#include <fc/container/flat.hpp>
#include <fc/string.hpp>
#include <fc/io/raw.hpp>
#include <fc/static_variant.hpp>
#include <fc/smart_ref_fwd.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/fixed_string.hpp>
#include <fc/crypto/private_key.hpp>

#include <memory>
#include <vector>
#include <deque>
#include <cstdint>

#define OBJECT_CTOR1(NAME) \
    NAME() = delete; \
    public: \
    template<typename Constructor, typename Allocator> \
    NAME(Constructor&& c, chainbase::allocator<Allocator>) \
    { c(*this); }
#define OBJECT_CTOR2_MACRO(x, y, field) ,field(a)
#define OBJECT_CTOR2(NAME, FIELDS) \
    NAME() = delete; \
    public: \
    template<typename Constructor, typename Allocator> \
    NAME(Constructor&& c, chainbase::allocator<Allocator> a) \
    : id(0) BOOST_PP_SEQ_FOR_EACH(OBJECT_CTOR2_MACRO, _, FIELDS) \
    { c(*this); }
#define OBJECT_CTOR(...) BOOST_PP_OVERLOAD(OBJECT_CTOR, __VA_ARGS__)(__VA_ARGS__)

#define _V(n, v)  fc::mutable_variant_object(n, v)

namespace eosio { namespace chain {
   using                               std::map;
   using                               std::vector;
   using                               std::unordered_map;
   using                               std::string;
   using                               std::deque;
   using                               std::shared_ptr;
   using                               std::weak_ptr;
   using                               std::unique_ptr;
   using                               std::set;
   using                               std::pair;
   using                               std::make_pair;
   using                               std::enable_shared_from_this;
   using                               std::tie;
   using                               std::move;
   using                               std::forward;
   using                               std::to_string;
   using                               std::all_of;

   using                               fc::path;
   using                               fc::smart_ref;
   using                               fc::variant_object;
   using                               fc::variant;
   using                               fc::enum_type;
   using                               fc::optional;
   using                               fc::unsigned_int;
   using                               fc::signed_int;
   using                               fc::time_point_sec;
   using                               fc::time_point;
   using                               fc::safe;
   using                               fc::flat_map;
   using                               fc::flat_set;
   using                               fc::static_variant;
   using                               fc::ecc::range_proof_type;
   using                               fc::ecc::range_proof_info;
   using                               fc::ecc::commitment_type;

   using public_key_type  = fc::crypto::public_key;
   using private_key_type = fc::crypto::private_key;
   using signature_type   = fc::crypto::signature;

   struct void_t{};

   using chainbase::allocator;
   using shared_string = boost::interprocess::basic_string<char, std::char_traits<char>, allocator<char>>;
   template<typename T>
   using shared_vector = boost::interprocess::vector<T, allocator<T>>;
   template<typename T>
   using shared_set = boost::interprocess::set<T, std::less<T>, allocator<T>>;
   template<typename K, typename V>
   using shared_flat_multimap = boost::interprocess::flat_multimap< K, V, std::less<K>, allocator< std::pair<K,V> > >;

   /**
    * For bugs in boost interprocess we moved our blob data to shared_string
    * this wrapper allows us to continue that while also having a type-level distinction for
    * serialization and to/from variant
    */
   class shared_blob : public shared_string {
      public:
         shared_blob() = delete;
         shared_blob(shared_blob&&) = default;

         shared_blob(const shared_blob& s)
         :shared_string(s.get_allocator())
         {
            assign(s.c_str(), s.size());
         }


         shared_blob& operator=(const shared_blob& s) {
            assign(s.c_str(), s.size());
            return *this;
         }

         shared_blob& operator=(shared_blob&& ) = default;

         template <typename InputIterator>
         shared_blob(InputIterator f, InputIterator l, const allocator_type& a)
         :shared_string(f,l,a)
         {}

         shared_blob(const allocator_type& a)
         :shared_string(a)
         {}
   };

   using action_name      = name;
   using scope_name       = name;
   using account_name     = name;
   using permission_name  = name;
   using table_name       = name;


   /**
    * List all object types from all namespaces here so they can
    * be easily reflected and displayed in debug output.  If a 3rd party
    * wants to extend the core code then they will have to change the
    * packed_object::type field from enum_type to uint16 to avoid
    * warnings when converting packed_objects to/from json.
    *
    * UNUSED_ enums can be taken for new purposes but otherwise the offsets
    * in this enumeration are potentially shared_memory breaking
    */
   enum object_type
   {
      null_object_type = 0,
      account_object_type,
      account_metadata_object_type,
      permission_object_type,
      permission_usage_object_type,
      permission_link_object_type,
      UNUSED_action_code_object_type,
      key_value_object_type,
      index64_object_type,
      index128_object_type,
      index256_object_type,
      index_double_object_type,
      index_long_double_object_type,
      global_property_object_type,
      dynamic_global_property_object_type,
      block_summary_object_type,
      transaction_object_type,
      generated_transaction_object_type,
      UNUSED_producer_object_type,
      UNUSED_chain_property_object_type,
      account_control_history_object_type,     ///< Defined by history_plugin
      UNUSED_account_transaction_history_object_type,
      UNUSED_transaction_history_object_type,
      public_key_history_object_type,          ///< Defined by history_plugin
      UNUSED_balance_object_type,
      UNUSED_staked_balance_object_type,
      UNUSED_producer_votes_object_type,
      UNUSED_producer_schedule_object_type,
      UNUSED_proxy_vote_object_type,
      UNUSED_scope_sequence_object_type,
      table_id_object_type,
      resource_limits_object_type,
      resource_usage_object_type,
      resource_limits_state_object_type,
      resource_limits_config_object_type,
      account_history_object_type,              ///< Defined by history_plugin
      action_history_object_type,               ///< Defined by history_plugin
      reversible_block_object_type,
      protocol_state_object_type,
      account_ram_correction_object_type,
      code_object_type,
      OBJECT_TYPE_COUNT ///< Sentry value which contains the number of different object types
   };

   /**
    *  Important notes on using chainbase objects in EOSIO code:
    *
    *  There are several constraints that need to be followed when using chainbase objects.
    *  Some of these constraints are due to the requirements imposed by the chainbase library,
    *  others are due to requirements to ensure determinism in the EOSIO chain library.
    *
    *  Before listing the constraints, the "restricted field set" must be defined.
    *
    *  Every chainbase object includes a field called id which has the type id_type.
    *  The id field is always included in the restricted field set.
    *
    *  A field of a chainbase object is considered to be in the restricted field set if it is involved in the
    *  derivation of the key used for one of the indices in the chainbase multi-index unless its only involvement
    *  is through being included in composite_keys that end with the id field.
    *
    *  So if the multi-index includes an index like the following
    *  ```
    *    ordered_unique< tag<by_sender_id>,
    *       composite_key< generated_transaction_object,
    *          BOOST_MULTI_INDEX_MEMBER( generated_transaction_object, account_name, sender),
    *          BOOST_MULTI_INDEX_MEMBER( generated_transaction_object, uint128_t, sender_id)
    *       >
    *    >
    *  ```
    *  both `sender` and `sender_id` fields are part of the restricted field set.
    *
    *  On the other hand, an index like the following
    *  ```
    *    ordered_unique< tag<by_expiration>,
    *       composite_key< generated_transaction_object,
    *          BOOST_MULTI_INDEX_MEMBER( generated_transaction_object, time_point, expiration),
    *          BOOST_MULTI_INDEX_MEMBER( generated_transaction_object, generated_transaction_object::id_type, id)
    *       >
    *    >
    *  ```
    *  would not by itself require the `expiration` field to be part of the restricted field set.
    *
    *  The restrictions on usage of the chainbase objects within this code base are:
    *     + The chainbase object includes the id field discussed above.
    *     + The multi-index must include an ordered_unique index tagged with by_id that is based on the id field as the sole key.
    *     + No other types of indices other than ordered_unique are allowed.
    *       If an index is desired that does not enforce uniqueness, then use a composite key that ends with the id field.
    *     + When creating a chainbase object, the constructor lambda should never mutate the id field.
    *     + When modifying a chainbase object, the modifier lambda should never mutate any fields in the restricted field set.
    */

   class account_object;

   using block_id_type       = fc::sha256;
   using checksum_type       = fc::sha256;
   using checksum256_type    = fc::sha256;
   using checksum512_type    = fc::sha512;
   using checksum160_type    = fc::ripemd160;
   using transaction_id_type = checksum_type;
   using digest_type         = checksum_type;
   using weight_type         = uint16_t;
   using block_num_type      = uint32_t;
   using share_type          = int64_t;
   using int128_t            = __int128;
   using uint128_t           = unsigned __int128;
   using bytes               = vector<char>;

   struct sha256_less {
      bool operator()( const fc::sha256& lhs, const fc::sha256& rhs ) const {
       return
             std::tie(lhs._hash[0], lhs._hash[1], lhs._hash[2], lhs._hash[3]) <
             std::tie(rhs._hash[0], rhs._hash[1], rhs._hash[2], rhs._hash[3]);
      }
   };


   /**
    *  Extentions are prefixed with type and are a buffer that can be
    *  interpreted by code that is aware and ignored by unaware code.
    */
   typedef vector<std::pair<uint16_t,vector<char>>> extensions_type;


   template<typename Container>
   class end_insert_iterator : public std::iterator< std::output_iterator_tag, void, void, void, void >
   {
   protected:
      Container* container;

   public:
      using container_type = Container;

      explicit end_insert_iterator( Container& c )
      :container(&c)
      {}

      end_insert_iterator& operator=( typename Container::const_reference value ) {
         container->insert( container->cend(), value );
         return *this;
      }

      end_insert_iterator& operator*() { return *this; }
      end_insert_iterator& operator++() { return *this; }
      end_insert_iterator  operator++(int) { return *this; }
   };

   template<typename Container>
   inline end_insert_iterator<Container> end_inserter( Container& c ) {
      return end_insert_iterator<Container>( c );
   }

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

   namespace detail {
      struct extract_match {
         bool enforce_unique = false;
      };

      template<typename... Ts>
      struct decompose;

      template<>
      struct decompose<> {
         template<typename ResultVariant>
         static auto extract( uint16_t id, const vector<char>& data, ResultVariant& result )
         -> fc::optional<extract_match>
         {
            return {};
         }
      };

      template<typename T, typename... Rest>
      struct decompose<T, Rest...> {
         using head_t = T;
         using tail_t = decompose< Rest... >;

         template<typename ResultVariant>
         static auto extract( uint16_t id, const vector<char>& data, ResultVariant& result )
         -> fc::optional<extract_match>
         {
            if( id == head_t::extension_id() ) {
               result = fc::raw::unpack<head_t>( data );
               return { extract_match{ head_t::enforce_unique() } };
            }

            return tail_t::template extract<ResultVariant>( id, data, result );
         }
      };
   }

   template<typename E, typename F>
   static inline auto has_field( F flags, E field )
   -> std::enable_if_t< std::is_integral<F>::value && std::is_unsigned<F>::value &&
                        std::is_enum<E>::value && std::is_same< F, std::underlying_type_t<E> >::value, bool>
   {
      return ( (flags & static_cast<F>(field)) != 0 );
   }

   template<typename E, typename F>
   static inline auto set_field( F flags, E field, bool value = true )
   -> std::enable_if_t< std::is_integral<F>::value && std::is_unsigned<F>::value &&
                        std::is_enum<E>::value && std::is_same< F, std::underlying_type_t<E> >::value, F >
   {
      if( value )
         return ( flags | static_cast<F>(field) );
      else
         return ( flags & ~static_cast<F>(field) );
   }

} }  // eosio::chain

FC_REFLECT( eosio::chain::void_t, )
