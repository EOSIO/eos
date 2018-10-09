/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/trace.hpp>
#include <eosio/chain/exceptions.hpp>
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
   abi_serializer(){ configure_built_in_types(); }
   abi_serializer( const abi_def& abi, const fc::microseconds& max_serialization_time );
   void set_abi(const abi_def& abi, const fc::microseconds& max_serialization_time);

   type_name resolve_type(const type_name& t)const;
   bool      is_array(const type_name& type)const;
   bool      is_optional(const type_name& type)const;
   bool      is_type(const type_name& type, const fc::microseconds& max_serialization_time)const;
   bool      is_builtin_type(const type_name& type)const;
   bool      is_integer(const type_name& type) const;
   int       get_integer_size(const type_name& type) const;
   bool      is_struct(const type_name& type)const;
   type_name fundamental_type(const type_name& type)const;

   const struct_def& get_struct(const type_name& type)const;

   type_name get_action_type(name action)const;
   type_name get_table_type(name action)const;

   optional<string>  get_error_message( uint64_t error_code )const;

   fc::variant binary_to_variant( const type_name& type, const bytes& binary, const fc::microseconds& max_serialization_time, bool short_path = false )const;
   fc::variant binary_to_variant( const type_name& type, fc::datastream<const char*>& binary, const fc::microseconds& max_serialization_time, bool short_path = false )const;

   bytes       variant_to_binary( const type_name& type, const fc::variant& var, const fc::microseconds& max_serialization_time, bool short_path = false )const;
   void        variant_to_binary( const type_name& type, const fc::variant& var, fc::datastream<char*>& ds, const fc::microseconds& max_serialization_time, bool short_path = false )const;

   template<typename T, typename Resolver>
   static void to_variant( const T& o, fc::variant& vo, Resolver resolver, const fc::microseconds& max_serialization_time );

   template<typename T, typename Resolver>
   static void from_variant( const fc::variant& v, T& o, Resolver resolver, const fc::microseconds& max_serialization_time );

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

   typedef std::function<fc::variant(fc::datastream<const char*>&, bool, bool)>  unpack_function;
   typedef std::function<void(const fc::variant&, fc::datastream<char*>&, bool, bool)>  pack_function;

   void add_specialized_unpack_pack( const string& name, std::pair<abi_serializer::unpack_function, abi_serializer::pack_function> unpack_pack );

   static const size_t max_recursion_depth = 32; // arbitrary depth to prevent infinite recursion

private:

   map<type_name, type_name>     typedefs;
   map<type_name, struct_def>    structs;
   map<name,type_name>           actions;
   map<name,type_name>           tables;
   map<uint64_t, string>         error_messages;
   map<type_name, variant_def>   variants;

   map<type_name, pair<unpack_function, pack_function>> built_in_types;
   void configure_built_in_types();

   fc::variant _binary_to_variant( const type_name& type, const bytes& binary, impl::binary_to_variant_context& ctx )const;
   fc::variant _binary_to_variant( const type_name& type, fc::datastream<const char*>& binary, impl::binary_to_variant_context& ctx )const;
   void        _binary_to_variant( const type_name& type, fc::datastream<const char*>& stream,
                                   fc::mutable_variant_object& obj, impl::binary_to_variant_context& ctx )const;

   bytes       _variant_to_binary( const type_name& type, const fc::variant& var, impl::variant_to_binary_context& ctx )const;
   void        _variant_to_binary( const type_name& type, const fc::variant& var,
                                   fc::datastream<char*>& ds, impl::variant_to_binary_context& ctx )const;

   static type_name _remove_bin_extension(const type_name& type);
   bool _is_type( const type_name& type, impl::abi_traverse_context& ctx )const;

   void validate( impl::abi_traverse_context& ctx )const;

   friend struct impl::abi_from_variant;
   friend struct impl::abi_to_variant;
   friend struct impl::abi_traverse_context_with_path;
};

namespace impl {

   struct abi_traverse_context {
      abi_traverse_context( fc::microseconds max_serialization_time )
      : max_serialization_time( max_serialization_time ), deadline( fc::time_point::now() + max_serialization_time ), recursion_depth(0)
      {}

      abi_traverse_context( fc::microseconds max_serialization_time, fc::time_point deadline )
      : max_serialization_time( max_serialization_time ), deadline( deadline ), recursion_depth(0)
      {}

      void check_deadline()const;

      fc::scoped_exit<std::function<void()>> enter_scope();

   protected:
      fc::microseconds max_serialization_time;
      fc::time_point   deadline;
      size_t           recursion_depth;
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
      abi_traverse_context_with_path( const abi_serializer& abis, fc::microseconds max_serialization_time, const type_name& type )
      : abi_traverse_context( max_serialization_time ), abis(abis)
      {
         set_path_root(type);
      }

      abi_traverse_context_with_path( const abi_serializer& abis, fc::microseconds max_serialization_time, fc::time_point deadline, const type_name& type )
      : abi_traverse_context( max_serialization_time, deadline ), abis(abis)
      {
         set_path_root(type);
      }

      abi_traverse_context_with_path( const abi_serializer& abis, const abi_traverse_context& ctx, const type_name& type )
      : abi_traverse_context(ctx), abis(abis)
      {
         set_path_root(type);
      }

      void set_path_root( const type_name& type );

      fc::scoped_exit<std::function<void()>> push_to_path( const path_item& item );

      void set_array_index_of_path_back( uint32_t i );
      void hint_array_type_if_in_array();
      void hint_struct_type_if_in_array( const map<type_name, struct_def>::const_iterator& itr );
      void hint_variant_type_if_in_array( const map<type_name, variant_def>::const_iterator& itr );

      string get_path_string()const;

      string maybe_shorten( const string& str );

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

   /**
    * Determine if a type contains ABI related info, perhaps deeply nested
    * @tparam T - the type to check
    */
   template<typename T>
   constexpr bool single_type_requires_abi_v() {
      return std::is_base_of<transaction, T>::value ||
             std::is_same<T, packed_transaction>::value ||
             std::is_same<T, transaction_trace>::value ||
             std::is_same<T, transaction_receipt>::value ||
             std::is_same<T, base_action_trace>::value ||
             std::is_same<T, action_trace>::value ||
             std::is_same<T, signed_transaction>::value ||
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
       * @tparam Resolver
       * @param act
       * @param resolver
       * @return
       */
      template<typename Resolver>
      static void add( mutable_variant_object &out, const char* name, const action& act, Resolver resolver, abi_traverse_context& ctx )
      {
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
       * overload of to_variant_object for packed_transaction
       * @tparam Resolver
       * @param act
       * @param resolver
       * @return
       */
      template<typename Resolver>
      static void add( mutable_variant_object &out, const char* name, const packed_transaction& ptrx, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         mutable_variant_object mvo;
         auto trx = ptrx.get_transaction();
         mvo("id", trx.id());
         mvo("signatures", ptrx.signatures);
         mvo("compression", ptrx.compression);
         mvo("packed_context_free_data", ptrx.packed_context_free_data);
         mvo("context_free_data", ptrx.get_context_free_data());
         mvo("packed_trx", ptrx.packed_trx);
         add(mvo, "transaction", trx, resolver, ctx);

         out(name, std::move(mvo));
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
      static void extract( const variant& v, packed_transaction& ptrx, Resolver resolver, abi_traverse_context& ctx )
      {
         auto h = ctx.enter_scope();
         const variant_object& vo = v.get_object();
         EOS_ASSERT(vo.contains("signatures"), packed_transaction_type_exception, "Missing signatures");
         EOS_ASSERT(vo.contains("compression"), packed_transaction_type_exception, "Missing compression");
         from_variant(vo["signatures"], ptrx.signatures);
         from_variant(vo["compression"], ptrx.compression);

         // TODO: Make this nicer eventually. But for now, if it works... good enough.
         if( vo.contains("packed_trx") && vo["packed_trx"].is_string() && !vo["packed_trx"].as_string().empty() ) {
            from_variant(vo["packed_trx"], ptrx.packed_trx);
            auto trx = ptrx.get_transaction(); // Validates transaction data provided.
            if( vo.contains("packed_context_free_data") && vo["packed_context_free_data"].is_string() && !vo["packed_context_free_data"].as_string().empty() ) {
               from_variant(vo["packed_context_free_data"], ptrx.packed_context_free_data );
            } else if( vo.contains("context_free_data") ) {
               vector<bytes> context_free_data;
               from_variant(vo["context_free_data"], context_free_data);
               ptrx.set_transaction(trx, context_free_data, ptrx.compression);
            }
         } else {
            EOS_ASSERT(vo.contains("transaction"), packed_transaction_type_exception, "Missing transaction");
            transaction trx;
            vector<bytes> context_free_data;
            extract(vo["transaction"], trx, resolver, ctx);
            if( vo.contains("packed_context_free_data") && vo["packed_context_free_data"].is_string() && !vo["packed_context_free_data"].as_string().empty() ) {
               from_variant(vo["packed_context_free_data"], ptrx.packed_context_free_data );
               context_free_data = ptrx.get_context_free_data();
            } else if( vo.contains("context_free_data") ) {
               from_variant(vo["context_free_data"], context_free_data);
            }
            ptrx.set_transaction(trx, context_free_data, ptrx.compression);
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
   class abi_from_variant_visitor : reflector_verifier_visitor<T>
   {
      public:
         abi_from_variant_visitor( const variant_object& _vo, T& v, Resolver _resolver, abi_traverse_context& _ctx )
         : reflector_verifier_visitor<T>(v)
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
void abi_serializer::to_variant( const T& o, variant& vo, Resolver resolver, const fc::microseconds& max_serialization_time ) try {
   mutable_variant_object mvo;
   impl::abi_traverse_context ctx(max_serialization_time);
   impl::abi_to_variant::add(mvo, "_", o, resolver, ctx);
   vo = std::move(mvo["_"]);
} FC_RETHROW_EXCEPTIONS(error, "Failed to serialize type", ("object",o))

template<typename T, typename Resolver>
void abi_serializer::from_variant( const variant& v, T& o, Resolver resolver, const fc::microseconds& max_serialization_time ) try {
   impl::abi_traverse_context ctx(max_serialization_time);
   impl::abi_from_variant::extract(v, o, resolver, ctx);
} FC_RETHROW_EXCEPTIONS(error, "Failed to deserialize variant", ("variant",v))


} } // eosio::chain
