#pragma once
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/exceptions.hpp>
#include <utility>
#include <fc/variant_object.hpp>
#include <fc/scoped_exit.hpp>

namespace eosio { namespace chain {

using std::map;
using std::string;
using std::function;
using std::pair;
using namespace fc;

namespace impl {
   struct abi_from_variant;
   struct abi_to_variant;

   struct abi_traverse_context;
   struct abi_traverse_context_with_path;
   struct binary_to_variant_context;
   struct variant_to_binary_context;
}

/**
 *  Describes the binary representation message and table contents so that it can
 *  be converted to and from JSON.
 */
struct abi_serializer {

   /// passed recursion_depth on each invocation
   using yield_function_t = fc::optional_delegate<void(size_t)>;

   abi_serializer(){ configure_built_in_types(); }
   abi_serializer( const abi_def& abi, const yield_function_t& yield );
   void set_abi( const abi_def& abi, const yield_function_t& yield );

   /// @return string_view of `t` or internal string type
   std::string_view resolve_type(const std::string_view& t)const;
   bool      is_array(const std::string_view& type)const;
   bool      is_optional(const std::string_view& type)const;
   bool      is_type( const std::string_view& type, const yield_function_t& yield )const;
   bool      is_builtin_type(const std::string_view& type)const;
   bool      is_integer(const std::string_view& type) const;
   int       get_integer_size(const std::string_view& type) const;
   bool      is_struct(const std::string_view& type)const;

   /// @return string_view of `type`
   std::string_view fundamental_type(const std::string_view& type)const;

   const struct_def& get_struct(const std::string_view& type)const;

   type_name get_action_type(name action)const;
   type_name get_table_type(name action)const;

   optional<string>  get_error_message( uint64_t error_code )const;

   fc::variant binary_to_variant( const std::string_view& type, const bytes& binary, const yield_function_t& yield, bool short_path = false )const;
   fc::variant binary_to_variant( const std::string_view& type, fc::datastream<const char*>& binary, const yield_function_t& yield, bool short_path = false )const;

   bytes       variant_to_binary( const std::string_view& type, const fc::variant& var, const yield_function_t& yield, bool short_path = false )const;
   void        variant_to_binary( const std::string_view& type, const fc::variant& var, fc::datastream<char*>& ds, const yield_function_t& yield, bool short_path = false )const;

   template<typename T, typename Resolver>
   static void to_variant( const T& o, fc::variant& vo, Resolver resolver, const yield_function_t& yield );

   template<typename T, typename Resolver>
   static void from_variant( const fc::variant& v, T& o, Resolver resolver, const yield_function_t& yield );

   template<typename Vec>
   static bool is_empty_abi(const Vec& abi_vec)
   {
      return abi_vec.size() <= 4;
   }

   template<typename Vec>
   static bool to_abi(const Vec& abi_vec, abi_def& abi)
   {
      if( !is_empty_abi(abi_vec) ) { /// 4 == packsize of empty Abi
         fc::datastream<const char*> ds( abi_vec.data(), abi_vec.size() );
         fc::raw::unpack( ds, abi );
         return true;
      }
      return false;
   }

   typedef std::function<fc::variant(fc::datastream<const char*>&, bool, bool, const abi_serializer::yield_function_t&)>  unpack_function;
   typedef std::function<void(const fc::variant&, fc::datastream<char*>&, bool, bool, const abi_serializer::yield_function_t&)>  pack_function;

   void add_specialized_unpack_pack( const string& name, std::pair<abi_serializer::unpack_function, abi_serializer::pack_function> unpack_pack );

   static constexpr size_t max_recursion_depth = 32; // arbitrary depth to prevent infinite recursion

   // create standard yield function that checks for max_serialization_time and max_recursion_depth.
   // now() deadline caputered at time of this call
   static yield_function_t create_yield_function(const fc::microseconds& max_serialization_time) {
      fc::time_point deadline = fc::time_point::now();
      if( max_serialization_time > fc::microseconds::maximum() - deadline.time_since_epoch() ) {
         deadline = fc::time_point::maximum();
      } else {
         deadline += max_serialization_time;
      }
      return [max_serialization_time, deadline](size_t recursion_depth) {
         EOS_ASSERT( recursion_depth < max_recursion_depth, abi_recursion_depth_exception,
                     "recursive definition, max_recursion_depth ${r} ", ("r", max_recursion_depth) );

         EOS_ASSERT( fc::time_point::now() < deadline, abi_serialization_deadline_exception,
                     "serialization time limit ${t}us exceeded", ("t", max_serialization_time) );
      };
   }

private:

   map<type_name, type_name, std::less<>>     typedefs;
   map<type_name, struct_def, std::less<>>    structs;
   map<name,type_name>                        actions;
   map<name,type_name>                        tables;
   map<uint64_t, string>                      error_messages;
   map<type_name, variant_def, std::less<>>   variants;

   map<type_name, pair<unpack_function, pack_function>, std::less<>> built_in_types;
   void configure_built_in_types();

   fc::variant _binary_to_variant( const std::string_view& type, const bytes& binary, impl::binary_to_variant_context& ctx )const;
   fc::variant _binary_to_variant( const std::string_view& type, fc::datastream<const char*>& binary, impl::binary_to_variant_context& ctx )const;
   void        _binary_to_variant( const std::string_view& type, fc::datastream<const char*>& stream,
                                   fc::mutable_variant_object& obj, impl::binary_to_variant_context& ctx )const;

   bytes       _variant_to_binary( const std::string_view& type, const fc::variant& var, impl::variant_to_binary_context& ctx )const;
   void        _variant_to_binary( const std::string_view& type, const fc::variant& var,
                                   fc::datastream<char*>& ds, impl::variant_to_binary_context& ctx )const;

   static std::string_view _remove_bin_extension(const std::string_view& type);
   bool _is_type( const std::string_view& type, impl::abi_traverse_context& ctx )const;

   void validate( impl::abi_traverse_context& ctx )const;

   friend struct impl::abi_from_variant;
   friend struct impl::abi_to_variant;
   friend struct impl::abi_traverse_context_with_path;
};

namespace impl {

   struct abi_traverse_context {
      explicit abi_traverse_context( abi_serializer::yield_function_t yield )
      : yield(std::move( yield )),
        recursion_depth(0)
      {
      }

      void check_deadline()const { yield( recursion_depth ); }
      abi_serializer::yield_function_t get_yield_function() { return yield; }

      fc::scoped_exit<std::function<void()>> enter_scope();

   protected:
      abi_serializer::yield_function_t  yield;
      size_t                            recursion_depth;
   };

   struct empty_path_root {};

   struct array_type_path_root {
   };

   struct struct_type_path_root {
      map<type_name, struct_def>::const_iterator  struct_itr;
   };

   struct variant_type_path_root {
      map<type_name, variant_def>::const_iterator variant_itr;
   };

   using path_root = static_variant<empty_path_root, array_type_path_root, struct_type_path_root, variant_type_path_root>;

   struct empty_path_item {};

   struct array_index_path_item {
      path_root                                   type_hint;
      uint32_t                                    array_index = 0;
   };

   struct field_path_item {
      map<type_name, struct_def>::const_iterator  parent_struct_itr;
      uint32_t                                    field_ordinal = 0;
   };

   struct variant_path_item {
      map<type_name, variant_def>::const_iterator variant_itr;
      uint32_t                                    variant_ordinal = 0;
   };

   using path_item = static_variant<empty_path_item, array_index_path_item, field_path_item, variant_path_item>;

   struct abi_traverse_context_with_path : public abi_traverse_context {
      abi_traverse_context_with_path( const abi_serializer& abis, abi_serializer::yield_function_t yield, const std::string_view& type )
      : abi_traverse_context( std::move( yield ) ), abis(abis)
      {
         set_path_root(type);
      }

      abi_traverse_context_with_path( const abi_serializer& abis, const abi_traverse_context& ctx, const std::string_view& type )
      : abi_traverse_context(ctx), abis(abis)
      {
         set_path_root(type);
      }

      void set_path_root( const std::string_view& type );

      fc::scoped_exit<std::function<void()>> push_to_path( const path_item& item );

      void set_array_index_of_path_back( uint32_t i );
      void hint_array_type_if_in_array();
      void hint_struct_type_if_in_array( const map<type_name, struct_def>::const_iterator& itr );
      void hint_variant_type_if_in_array( const map<type_name, variant_def>::const_iterator& itr );

      string get_path_string()const;

      string maybe_shorten( const std::string_view& str );

   protected:
      const abi_serializer&  abis;
      path_root              root_of_path;
      vector<path_item>      path;
   public:
      bool                   short_path = false;
   };

   struct binary_to_variant_context : public abi_traverse_context_with_path {
      using abi_traverse_context_with_path::abi_traverse_context_with_path;
   };

   struct variant_to_binary_context : public abi_traverse_context_with_path {
      using abi_traverse_context_with_path::abi_traverse_context_with_path;

      fc::scoped_exit<std::function<void()>> disallow_extensions_unless( bool condition );

      bool extensions_allowed()const { return allow_extensions; }

   protected:
      bool                   allow_extensions = true;
   };

   /// limits the string size to default max_length of output_name
   string limit_size( const std::string_view& str );

   /**
    * Determine if a type contains ABI related info, perhaps deeply nested
    * @tparam T - the type to check
    */
   template<typename T>
   constexpr bool single_type_requires_abi_v() {
      return std::is_base_of<transaction, T>::value ||
             std::is_same<T, packed_transaction_v0>::value ||
             std::is_same<T, packed_transaction>::value ||
             std::is_same<T, transaction_trace>::value ||
             std::is_same<T, transaction_receipt_v0>::value ||
             std::is_same<T, transaction_receipt>::value ||
             std::is_same<T, action_trace>::value ||
             std::is_same<T, signed_transaction>::value ||
             std::is_same<T, signed_block_v0>::value ||
             std::is_same<T, signed_block>::value ||
             std::is_same<T, action>::value;
   }

   /**
    * Basic constexpr for a type, aliases the basic check directly
    * @tparam T - the type to check
    */
   template<typename T>
   struct type_requires_abi {
      static constexpr bool value() {
         return single_type_requires_abi_v<T>();
      }
   };

   /**
    * specialization that catches common container patterns and checks their contained-type
    * @tparam Container - a templated container type whose first argument is the contained type
    */
   template<template<typename ...> class Container, typename T, typename ...Args >
   struct type_requires_abi<Container<T, Args...>> {
      static constexpr bool value() {
         return single_type_requires_abi_v<T>();
      }
   };

   template<typename T>
   constexpr bool type_requires_abi_v() {
      return type_requires_abi<T>::value();
   }

   /**
    * convenience aliases for creating overload-guards based on whether the type contains ABI related info
    */
   template<typename T>
   using not_require_abi_t = std::enable_if_t<!type_requires_abi_v<T>(), int>;

   template<typename T>
   using require_abi_t = std::enable_if_t<type_requires_abi_v<T>(), int>;

   struct abi_to_variant {
      /**
       * template which overloads add for types which are not relvant to ABI information
       * and can be degraded to the normal ::to_variant(...) processing
       */
      template<typename M, typename Resolver, not_require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const M& v, Resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         mvo(name,v);
      }

      /**
       * template which overloads add for types which contain ABI information in their trees
       * for these types we create new ABI aware visitors
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const M& v, Resolver resolver, abi_traverse_context& ctx );

      /**
       * template which overloads add for vectors of types which contain ABI information in their trees
       * for these members we call ::add in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const vector<M>& v, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         vector<variant> array;
         array.reserve(v.size());

         for (const auto& iter: v) {
            mutable_variant_object elem_mvo;
            add(elem_mvo, "_", iter, resolver, ctx);
            array.emplace_back(std::move(elem_mvo["_"]));
         }
         mvo(name, std::move(array));
      }

      /**
       * template which overloads add for deques of types which contain ABI information in their trees
       * for these members we call ::add in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const deque<M>& v, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         deque<variant> array;

         for (const auto& iter: v) {
            mutable_variant_object elem_mvo;
            add(elem_mvo, "_", iter, resolver, ctx);
            array.emplace_back(std::move(elem_mvo["_"]));
         }
         mvo(name, std::move(array));
      }

      /**
       * template which overloads add for shared_ptr of types which contain ABI information in their trees
       * for these members we call ::add in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const std::shared_ptr<M>& v, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         if( !v ) return;
         mutable_variant_object obj_mvo;
         add(obj_mvo, "_", *v, resolver, ctx);
         mvo(name, std::move(obj_mvo["_"]));
      }

      template<typename Resolver>
      struct add_static_variant
      {
         mutable_variant_object& obj_mvo;
         Resolver& resolver;
         abi_traverse_context& ctx;

         add_static_variant( mutable_variant_object& o, Resolver& r, abi_traverse_context& ctx )
               :obj_mvo(o), resolver(r), ctx(ctx) {}

         typedef void result_type;
         template<typename T> void operator()( T& v )const
         {
            add(obj_mvo, "_", v, resolver, ctx);
         }
      };

      template<typename Resolver, typename... Args>
      static void add( mutable_variant_object &mvo, const char* name, const fc::static_variant<Args...>& v, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         mutable_variant_object obj_mvo;
         add_static_variant<Resolver> adder(obj_mvo, resolver, ctx);
         v.visit(adder);
         mvo(name, std::move(obj_mvo["_"]));
      }

      /**
       * overload of to_variant_object for actions
       *
       * This matches the FC_REFLECT for this type, but this is provided to extract the contents of act.data
       * @tparam Resolver
       * @param act
       * @param resolver
       * @return
       */
      template<typename Resolver>
      static void add( mutable_variant_object &out, const char* name, const action& act, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert(fc::reflector<action>::total_member_count == 4);
         auto h = ctx.enter_scope();
         mutable_variant_object mvo;
         mvo("account", act.account);
         mvo("name", act.name);
         mvo("authorization", act.authorization);

         try {
            auto abi = resolver(act.account);
            if (abi.valid()) {
               auto type = abi->get_action_type(act.name);
               if (!type.empty()) {
                  try {
                     binary_to_variant_context _ctx(*abi, ctx, type);
                     _ctx.short_path = true; // Just to be safe while avoiding the complexity of threading an override boolean all over the place
                     mvo( "data", abi->_binary_to_variant( type, act.data, _ctx ));
                     mvo("hex_data", act.data);
                  } catch(...) {
                     // any failure to serialize data, then leave as not serailzed
                     mvo("data", act.data);
                  }
               } else {
                  mvo("data", act.data);
               }
            } else {
               mvo("data", act.data);
            }
         } catch(...) {
            mvo("data", act.data);
         }
         out(name, std::move(mvo));
      }

      /**
       * overload of to_variant_object for packed_transaction_v0
       *
       * This matches the FC_REFLECT for packed_transaction_v0 type with the addition of "transaction" which is provided
       * to allow extracting the contents of ptrx.get_transaction()
       */
      template<typename Resolver>
      static void add( mutable_variant_object& out, const char* name, const packed_transaction_v0& ptrx, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert(fc::reflector<packed_transaction_v0>::total_member_count == 4);
         auto h = ctx.enter_scope();
         mutable_variant_object mvo;
         const auto& trx = ptrx.get_transaction();
         mvo("id", trx.id());
         mvo("signatures", ptrx.get_signatures());
         mvo("compression", ptrx.get_compression());
         mvo("packed_context_free_data", ptrx.get_packed_context_free_data());
         mvo("context_free_data", ptrx.get_context_free_data());
         mvo("packed_trx", ptrx.get_packed_transaction());
         add(mvo, "transaction", trx, resolver, ctx);
         out(name, std::move(mvo));
      }

      /**
       * overload of to_variant_object for packed_transaction, providing original packed_transaction_v0 variant layout
       *
       * This matches the FC_REFLECT for packed_transaction_v0 type with the addition of "transaction" which is provided
       * to allow extracting the contents of ptrx.get_transaction(). The generated variant should match above method.
       */
      template<typename Resolver>
      static void add( mutable_variant_object& out, const char* name, const packed_transaction& ptrx, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert(fc::reflector<packed_transaction>::total_member_count == 3);
         auto h = ctx.enter_scope();
         mutable_variant_object mvo;
         const auto& trx = ptrx.get_transaction();
         mvo("id", trx.id());
         const auto* sigs = ptrx.get_signatures();
         if( ptrx.get_prunable_data().prunable_data.contains<packed_transaction::prunable_data_type::full_legacy>() ) {
            const auto& legacy = ptrx.get_prunable_data().prunable_data.get<packed_transaction::prunable_data_type::full_legacy>();
            mvo("signatures", legacy.signatures );
         } else {
            mvo("signatures", vector<signature_type>());
         }
         mvo("compression", ptrx.get_compression());
         if( ptrx.get_prunable_data().prunable_data.contains<packed_transaction::prunable_data_type::full_legacy>() ) {
            const auto& legacy = ptrx.get_prunable_data().prunable_data.get<packed_transaction::prunable_data_type::full_legacy>();
            mvo("packed_context_free_data", legacy.packed_context_free_data);
            mvo("context_free_data", legacy.context_free_segments);
         } else {
            mvo("packed_context_free_data", bytes());
            mvo("context_free_data", vector<bytes>());
         }
         mvo("packed_trx", ptrx.get_packed_transaction());
         add(mvo, "transaction", trx, resolver, ctx);
         out(name, std::move(mvo));
      }

      /**
       * overload of to_variant_object for transaction
       *
       * This matches the FC_REFLECT for this type, but this is provided to allow extracting the contents of trx.transaction_extensions
       */
      template<typename Resolver>
      static void add( mutable_variant_object &out, const char* name, const transaction& trx, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert(fc::reflector<transaction>::total_member_count == 9);
         auto h = ctx.enter_scope();
         mutable_variant_object mvo;
         mvo("expiration", trx.expiration);
         mvo("ref_block_num", trx.ref_block_num);
         mvo("ref_block_prefix", trx.ref_block_prefix);
         mvo("max_net_usage_words", trx.max_net_usage_words);
         mvo("max_cpu_usage_ms", trx.max_cpu_usage_ms);
         mvo("delay_sec", trx.delay_sec);
         add(mvo, "context_free_actions", trx.context_free_actions, resolver, ctx);
         add(mvo, "actions", trx.actions, resolver, ctx);

         // process contents of block.transaction_extensions
         auto exts = trx.validate_and_extract_extensions();
         if (exts.count(deferred_transaction_generation_context::extension_id()) > 0) {
            const auto& deferred_transaction_generation = exts.lower_bound(deferred_transaction_generation_context::extension_id())->second.get<deferred_transaction_generation_context>();
            mvo("deferred_transaction_generation", deferred_transaction_generation);
         }

         out(name, std::move(mvo));
      }

      template<typename Resolver, typename SignedBlock>
      static void add_signed_block( mutable_variant_object& out, const char* name, const SignedBlock& block, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert( std::is_same_v<SignedBlock, signed_block> || std::is_same_v<SignedBlock, signed_block_v0> );
         auto h = ctx.enter_scope();
         mutable_variant_object mvo;
         mvo("timestamp", block.timestamp);
         mvo("producer", block.producer);
         mvo("confirmed", block.confirmed);
         mvo("previous", block.previous);
         mvo("transaction_mroot", block.transaction_mroot);
         mvo("action_mroot", block.action_mroot);
         mvo("schedule_version", block.schedule_version);
         mvo("new_producers", block.new_producers);

         // process contents of block.header_extensions
         flat_multimap<uint16_t, block_header_extension> header_exts = block.validate_and_extract_header_extensions();
         if ( header_exts.count(protocol_feature_activation::extension_id() > 0) ) {
            const auto& new_protocol_features =
                  header_exts.lower_bound(protocol_feature_activation::extension_id())->second.template get<protocol_feature_activation>().protocol_features;
            vector<variant> pf_array;
            pf_array.reserve(new_protocol_features.size());
            for (auto feature : new_protocol_features) {
               mutable_variant_object feature_mvo;
               add(feature_mvo, "feature_digest", feature, resolver, ctx);
               pf_array.push_back(feature_mvo);
            }
            mvo("new_protocol_features", pf_array);
         }
         if ( header_exts.count(producer_schedule_change_extension::extension_id())) {
            const auto& new_producer_schedule =
                  header_exts.lower_bound(producer_schedule_change_extension::extension_id())->second.template get<producer_schedule_change_extension>();
            mvo("new_producer_schedule", new_producer_schedule);
         }

         mvo("producer_signature", block.producer_signature);
         add(mvo, "transactions", block.transactions, resolver, ctx);

         // process contents of block.block_extensions
         auto block_exts = block.validate_and_extract_extensions();
         if ( block_exts.count(additional_block_signatures_extension::extension_id()) > 0) {
            const auto& additional_signatures =
                  block_exts.lower_bound(additional_block_signatures_extension::extension_id())->second.template get<additional_block_signatures_extension>();
            mvo("additional_signatures", additional_signatures);
         }

         out(name, std::move(mvo));
      }

      /**
       * overload of to_variant_object for signed_block, support old signed_block_v0 format
       */
      template<typename Resolver>
      static void add( mutable_variant_object& out, const char* name, const signed_block& block, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert(fc::reflector<signed_block>::total_member_count == 13);
         add_signed_block( out, name, block, std::move(resolver), ctx );
      }

      /**
       * overload of to_variant_object for signed_block_v0
       *
       * This matches the FC_REFLECT for this type, but this is provided to allow extracting the contents of
       * block.header_extensions and block.block_extensions
       */
      template<typename Resolver>
      static void add( mutable_variant_object &out, const char* name, const signed_block_v0& block, Resolver resolver, abi_traverse_context& ctx )
      {
         static_assert(fc::reflector<signed_block_v0>::total_member_count == 12);
         add_signed_block( out, name, block, std::move(resolver), ctx );
      }
   };

   /**
    * Reflection visitor that uses a resolver to resolve ABIs for nested types
    * this will degrade to the common fc::to_variant as soon as the type no longer contains
    * ABI related info
    *
    * @tparam Reslover - callable with the signature (const name& code_account) -> optional<abi_def>
    */
   template<typename T, typename Resolver>
   class abi_to_variant_visitor
   {
      public:
         abi_to_variant_visitor( mutable_variant_object& _mvo, const T& _val, Resolver _resolver, abi_traverse_context& _ctx )
         :_vo(_mvo)
         ,_val(_val)
         ,_resolver(_resolver)
         ,_ctx(_ctx)
         {}

         /**
          * Visit a single member and add it to the variant object
          * @tparam Member - the member to visit
          * @tparam Class - the class we are traversing
          * @tparam member - pointer to the member
          * @param name - the name of the member
          */
         template<typename Member, class Class, Member (Class::*member) >
         void operator()( const char* name )const
         {
            abi_to_variant::add( _vo, name, (_val.*member), _resolver, _ctx );
         }

      private:
         mutable_variant_object& _vo;
         const T& _val;
         Resolver _resolver;
         abi_traverse_context& _ctx;
   };

   struct abi_from_variant {
      /**
       * template which overloads extract for types which are not relvant to ABI information
       * and can be degraded to the normal ::from_variant(...) processing
       */
      template<typename M, typename Resolver, not_require_abi_t<M> = 1>
      static void extract( const variant& v, M& o, Resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         from_variant(v, o);
      }

      /**
       * template which overloads extract for types which contain ABI information in their trees
       * for these types we create new ABI aware visitors
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void extract( const variant& v, M& o, Resolver resolver, abi_traverse_context& ctx );

      /**
       * template which overloads extract for vectors of types which contain ABI information in their trees
       * for these members we call ::extract in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void extract( const variant& v, vector<M>& o, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         const variants& array = v.get_array();
         o.clear();
         o.reserve( array.size() );
         for( auto itr = array.begin(); itr != array.end(); ++itr ) {
            M o_iter;
            extract(*itr, o_iter, resolver, ctx);
            o.emplace_back(std::move(o_iter));
         }
      }

      /**
       * template which overloads extract for deque of types which contain ABI information in their trees
       * for these members we call ::extract in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void extract( const variant& v, deque<M>& o, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         const variants& array = v.get_array();
         o.clear();
         for( auto itr = array.begin(); itr != array.end(); ++itr ) {
            M o_iter;
            extract(*itr, o_iter, resolver, ctx);
            o.emplace_back(std::move(o_iter));
         }
      }

      /**
       * template which overloads extract for shared_ptr of types which contain ABI information in their trees
       * for these members we call ::extract in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void extract( const variant& v, std::shared_ptr<M>& o, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         const variant_object& vo = v.get_object();
         M obj;
         extract(vo, obj, resolver, ctx);
         o = std::make_shared<M>(obj);
      }

      /**
       * Non templated overload that has priority for the action structure
       * this type has members which must be directly translated by the ABI so it is
       * exploded and processed explicitly
       */
      template<typename Resolver>
      static void extract( const variant& v, action& act, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         const variant_object& vo = v.get_object();
         EOS_ASSERT(vo.contains("account"), packed_transaction_type_exception, "Missing account");
         EOS_ASSERT(vo.contains("name"), packed_transaction_type_exception, "Missing name");
         from_variant(vo["account"], act.account);
         from_variant(vo["name"], act.name);

         if (vo.contains("authorization")) {
            from_variant(vo["authorization"], act.authorization);
         }

         bool valid_empty_data = false;
         if( vo.contains( "data" ) ) {
            const auto& data = vo["data"];
            if( data.is_string() ) {
               from_variant(data, act.data);
               valid_empty_data = act.data.empty();
            } else if ( data.is_object() ) {
               auto abi = resolver(act.account);
               if (abi.valid()) {
                  auto type = abi->get_action_type(act.name);
                  if (!type.empty()) {
                     variant_to_binary_context _ctx(*abi, ctx, type);
                     _ctx.short_path = true; // Just to be safe while avoiding the complexity of threading an override boolean all over the place
                     act.data = std::move( abi->_variant_to_binary( type, data, _ctx ));
                     valid_empty_data = act.data.empty();
                  }
               }
            }
         }

         if( !valid_empty_data && act.data.empty() ) {
            if( vo.contains( "hex_data" ) ) {
               const auto& data = vo["hex_data"];
               if( data.is_string() ) {
                  from_variant(data, act.data);
               }
            }
         }

         EOS_ASSERT(valid_empty_data || !act.data.empty(), packed_transaction_type_exception,
                    "Failed to deserialize data for ${account}:${name}", ("account", act.account)("name", act.name));
      }

      template<typename Resolver>
      static void extract( const variant& v, packed_transaction_v0& ptrx, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         const variant_object& vo = v.get_object();
         EOS_ASSERT(vo.contains("signatures"), packed_transaction_type_exception, "Missing signatures");
         EOS_ASSERT(vo.contains("compression"), packed_transaction_type_exception, "Missing compression");
         std::vector<signature_type> signatures;
         packed_transaction_v0::compression_type compression;
         from_variant(vo["signatures"], signatures);
         from_variant(vo["compression"], compression);

         bytes packed_cfd;
         std::vector<bytes> cfd;
         bool use_packed_cfd = false;
         if( vo.contains("packed_context_free_data") && vo["packed_context_free_data"].is_string() && !vo["packed_context_free_data"].as_string().empty() ) {
            from_variant(vo["packed_context_free_data"], packed_cfd );
            use_packed_cfd = true;
         } else if( vo.contains("context_free_data") ) {
            from_variant(vo["context_free_data"], cfd);
         }

         if( vo.contains("packed_trx") && vo["packed_trx"].is_string() && !vo["packed_trx"].as_string().empty() ) {
            bytes packed_trx;
            from_variant(vo["packed_trx"], packed_trx);
            if( use_packed_cfd ) {
               ptrx = packed_transaction_v0( std::move( packed_trx ), std::move( signatures ), std::move( packed_cfd ), compression );
            } else {
               ptrx = packed_transaction_v0( std::move( packed_trx ), std::move( signatures ), std::move( cfd ), compression );
            }
         } else {
            EOS_ASSERT(vo.contains("transaction"), packed_transaction_type_exception, "Missing transaction");
            if( use_packed_cfd ) {
               transaction trx;
               extract( vo["transaction"], trx, resolver, ctx );
               ptrx = packed_transaction_v0( std::move(trx), std::move(signatures), std::move(packed_cfd), compression );
            } else {
               signed_transaction trx;
               extract( vo["transaction"], trx, resolver, ctx );
               trx.signatures = std::move( signatures );
               trx.context_free_data = std::move(cfd);
               ptrx = packed_transaction_v0( std::move( trx ), compression );
            }
         }
      }

   };

   /**
    * Reflection visitor that uses a resolver to resolve ABIs for nested types
    * this will degrade to the common fc::from_variant as soon as the type no longer contains
    * ABI related info
    *
    * @tparam Reslover - callable with the signature (const name& code_account) -> optional<abi_def>
    */
   template<typename T, typename Resolver>
   class abi_from_variant_visitor : public reflector_init_visitor<T>
   {
      public:
         abi_from_variant_visitor( const variant_object& _vo, T& v, Resolver _resolver, abi_traverse_context& _ctx )
         : reflector_init_visitor<T>(v)
         ,_vo(_vo)
         ,_resolver(_resolver)
         ,_ctx(_ctx)
         {}

         /**
          * Visit a single member and extract it from the variant object
          * @tparam Member - the member to visit
          * @tparam Class - the class we are traversing
          * @tparam member - pointer to the member
          * @param name - the name of the member
          */
         template<typename Member, class Class, Member (Class::*member)>
         void operator()( const char* name )const
         {
            auto itr = _vo.find(name);
            if( itr != _vo.end() )
               abi_from_variant::extract( itr->value(), this->obj.*member, _resolver, _ctx );
         }

      private:
         const variant_object& _vo;
         Resolver _resolver;
         abi_traverse_context& _ctx;
   };

   template<typename M, typename Resolver, require_abi_t<M>>
   void abi_to_variant::add( mutable_variant_object &mvo, const char* name, const M& v, Resolver resolver, abi_traverse_context& ctx )
   {
      auto h = ctx.enter_scope();
      mutable_variant_object member_mvo;
      fc::reflector<M>::visit( impl::abi_to_variant_visitor<M, Resolver>( member_mvo, v, resolver, ctx) );
      mvo(name, std::move(member_mvo));
   }

   template<typename M, typename Resolver, require_abi_t<M>>
   void abi_from_variant::extract( const variant& v, M& o, Resolver resolver, abi_traverse_context& ctx )
   {
      auto h = ctx.enter_scope();
      const variant_object& vo = v.get_object();
      fc::reflector<M>::visit( abi_from_variant_visitor<M, decltype(resolver)>( vo, o, resolver, ctx ) );
   }
} /// namespace eosio::chain::impl

template<typename T, typename Resolver>
void abi_serializer::to_variant( const T& o, variant& vo, Resolver resolver, const yield_function_t& yield ) try {
   mutable_variant_object mvo;
   impl::abi_traverse_context ctx( yield );
   impl::abi_to_variant::add(mvo, "_", o, resolver, ctx);
   vo = std::move(mvo["_"]);
} FC_RETHROW_EXCEPTIONS(error, "Failed to serialize: ${type}", ("type", boost::core::demangle( typeid(o).name() ) ))

template<typename T, typename Resolver>
void abi_serializer::from_variant( const variant& v, T& o, Resolver resolver, const yield_function_t& yield ) try {
   static_assert( !std::is_same_v<T, packed_transaction>, "use packed_transaction_v0" );
   static_assert( !std::is_same_v<T, signed_block>, "use signed_block_v0" );
   impl::abi_traverse_context ctx( yield );
   impl::abi_from_variant::extract(v, o, resolver, ctx);
} FC_RETHROW_EXCEPTIONS(error, "Failed to deserialize variant", ("variant",v))


} } // eosio::chain
