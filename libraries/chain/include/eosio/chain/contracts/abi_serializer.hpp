/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eosio/chain/contracts/types.hpp>
#include <eosio/chain/transaction.hpp>
#include <eosio/chain/block.hpp>

#include <fc/variant_object.hpp>

namespace eosio { namespace chain { namespace contracts {

using std::map;
using std::string;
using std::function;
using std::pair;
using namespace fc;

/**
 *  Describes the binary representation message and table contents so that it can
 *  be converted to and from JSON.
 */
struct abi_serializer {
   abi_serializer(){ configure_built_in_types(); }
   abi_serializer( const abi_def& abi );
   void set_abi(const abi_def& abi);

   map<type_name, type_name>  typedefs;
   map<type_name, struct_def> structs;
   map<name,type_name>        actions;
   map<name,type_name>        tables;

   typedef std::function<fc::variant(fc::datastream<const char*>&, bool)>  unpack_function;
   typedef std::function<void(const fc::variant&, fc::datastream<char*>&, bool)>  pack_function;
   
   map<type_name, pair<unpack_function, pack_function>> built_in_types;
   void configure_built_in_types();

   bool has_cycle(const vector<pair<string, string>>& sedges)const;
   void validate()const;

   type_name resolve_type(const type_name& t)const;
   bool      is_array(const type_name& type)const;
   bool      is_type(const type_name& type)const;
   bool      is_builtin_type(const type_name& type)const;
   bool      is_integer(const type_name& type) const;
   int       get_integer_size(const type_name& type) const;
   bool      is_struct(const type_name& type)const;
   type_name array_type(const type_name& type)const;

   const struct_def& get_struct(const type_name& type)const;

   type_name get_action_type(name action)const;
   type_name get_table_type(name action)const;

   fc::variant binary_to_variant(const type_name& type, const bytes& binary)const;
   bytes       variant_to_binary(const type_name& type, const fc::variant& var)const;

   fc::variant binary_to_variant(const type_name& type, fc::datastream<const char*>& binary)const;
   void        variant_to_binary(const type_name& type, const fc::variant& var, fc::datastream<char*>& ds)const;

   template<typename T, typename Resolver>
   static void to_variant( const T& o, fc::variant& vo, Resolver resolver );

   template<typename T, typename Resolver>
   static void from_variant( const fc::variant& v, T& o, Resolver resolver );

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

   private:
   void binary_to_variant(const type_name& type, fc::datastream<const char*>& stream, fc::mutable_variant_object& obj)const;
};

namespace impl {
   /**
    * Determine if a type contains ABI related info, perhaps deeply nested
    * @tparam T - the type to check
    */
   template<typename T>
   constexpr bool single_type_requires_abi_v() {
      return std::is_base_of<transaction, T>::value ||
             std::is_same<T, packed_transaction>::value ||
             std::is_same<T, action>::value ||
             std::is_same<T, transaction_trace>::value ||
             std::is_same<T, action_trace>::value ||
             std::is_same<T, shard_summary>::value ||
             std::is_same<T, region_summary>::value ||
             std::is_base_of<signed_block_summary, T>:: value;
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
      static void add( mutable_variant_object &mvo, const char* name, const M& v, Resolver )
      {
         mvo(name,v);
      }

      /**
       * template which overloads add for types which contain ABI information in their trees
       * for these types we create new ABI aware visitors
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const M& v, Resolver resolver );

      /**
       * template which overloads add for vectors of types which contain ABI information in their trees
       * for these members we call ::add in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void add( mutable_variant_object &mvo, const char* name, const vector<M>& v, Resolver resolver )
      {
         vector<variant> array;
         array.reserve(v.size());

         for (const auto& iter: v) {
            mutable_variant_object elem_mvo;
            add(elem_mvo, "_", iter, resolver);
            array.emplace_back(std::move(elem_mvo["_"]));
         }
         mvo(name, std::move(array));
      }

      /**
       * overload of to_variant_object for actions
       * @tparam Resolver
       * @param act
       * @param resolver
       * @return
       */
      template<typename Resolver>
      static void add(mutable_variant_object &out, const char* name, const action& act, Resolver resolver) {
         mutable_variant_object mvo;
         mvo("account", act.account);
         mvo("name", act.name);
         mvo("authorization", act.authorization);

         auto abi = resolver(act.account);
         if (abi.valid()) {
            auto type = abi->get_action_type(act.name);
            mvo("data", abi->binary_to_variant(type, act.data));
            mvo("hex_data", act.data);
         } else {
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
      static void add(mutable_variant_object &out, const char* name, const packed_transaction& ptrx, Resolver resolver) {
         mutable_variant_object mvo;
         mvo("signatures", ptrx.signatures);
         mvo("compression", ptrx.compression);
         mvo("hex_data", ptrx.data);

         transaction trx = ptrx.get_transaction();
         add(mvo, "data", trx, resolver);
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
         abi_to_variant_visitor( mutable_variant_object& _mvo, const T& _val, Resolver _resolver )
         :_vo(_mvo)
         ,_val(_val)
         ,_resolver(_resolver)
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
            abi_to_variant::add(_vo, name, (_val.*member), _resolver);
         }

      private:
         mutable_variant_object& _vo;
         const T& _val;
         Resolver _resolver;
   };

   struct abi_from_variant {
      /**
       * template which overloads extract for types which are not relvant to ABI information
       * and can be degraded to the normal ::from_variant(...) processing
       */
      template<typename M, typename Resolver, not_require_abi_t<M> = 1>
      static void extract( const variant& v, M& o, Resolver )
      {
         from_variant(v, o);
      }

      /**
       * template which overloads extract for types which contain ABI information in their trees
       * for these types we create new ABI aware visitors
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void extract( const variant& v, M& o, Resolver resolver );

      /**
       * template which overloads extract for vectors of types which contain ABI information in their trees
       * for these members we call ::extract in order to trigger further processing
       */
      template<typename M, typename Resolver, require_abi_t<M> = 1>
      static void extract( const variant& v, vector<M>& o, Resolver resolver )
      {
         const variants& array = v.get_array();
         o.clear();
         o.reserve( array.size() );
         for( auto itr = array.begin(); itr != array.end(); ++itr ) {
            M o_iter;
            extract(*itr, o_iter, resolver);
            o.emplace_back(std::move(o_iter));
         }
      }

      /**
       * Non templated overload that has priority for the action structure
       * this type has members which must be directly translated by the ABI so it is
       * exploded and processed explicitly
       */
      template<typename Resolver>
      static void extract( const variant& v, action& act, Resolver resolver )
      {
         const variant_object& vo = v.get_object();
         FC_ASSERT(vo.contains("account"));
         FC_ASSERT(vo.contains("name"));
         from_variant(vo["account"], act.account);
         from_variant(vo["name"], act.name);

         if (vo.contains("authorization")) {
            from_variant(vo["authorization"], act.authorization);
         }

         if( vo.contains( "data" ) ) {
            const auto& data = vo["data"];
            if( data.is_string() ) {
               from_variant(data, act.data);
            } else if ( data.is_object() ) {
               auto abi = resolver(act.account);
               if (abi.valid()) {
                  auto type = abi->get_action_type(act.name);
                  act.data = std::move(abi->variant_to_binary(type, data));
               }
            }
         }

         if (act.data.empty()) {
            if( vo.contains( "hex_data" ) ) {
               const auto& data = vo["hex_data"];
               if( data.is_string() ) {
                  from_variant(data, act.data);
               }
            }
         }

         FC_ASSERT(!act.data.empty(), "Failed to deserialize data for ${account}:${name}", ("account", act.account)("name", act.name));
      }

      template<typename Resolver>
      static void extract( const variant& v, packed_transaction& ptrx, Resolver resolver ) {
         const variant_object& vo = v.get_object();
         FC_ASSERT(vo.contains("signatures"));
         FC_ASSERT(vo.contains("compression"));
         from_variant(vo["signatures"], ptrx.signatures);
         from_variant(vo["compression"], ptrx.compression);

         if (vo.contains("hex_data") && vo["hex_data"].is_string() && !vo["hex_data"].as_string().empty()) {
            from_variant(vo["hex_data"], ptrx.data);
         } else {
            FC_ASSERT(vo.contains("data"));
            if (vo["data"].is_string()) {
               from_variant(vo["data"], ptrx.data);
            } else {
               transaction trx;
               extract(vo["data"], trx, resolver);
               ptrx.set_transaction(trx, ptrx.compression);
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
   class abi_from_variant_visitor
   {
      public:
         abi_from_variant_visitor( const variant_object& _vo, T& v, Resolver _resolver )
         :_vo(_vo)
         ,_val(v)
         ,_resolver(_resolver)
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
               abi_from_variant::extract( itr->value(), _val.*member, _resolver );
         }

      private:
         const variant_object& _vo;
         T& _val;
         Resolver _resolver;
   };


   template<typename M, typename Resolver, require_abi_t<M>>
   void abi_to_variant::add( mutable_variant_object &mvo, const char* name, const M& v, Resolver resolver ) {
      mutable_variant_object member_mvo;
      fc::reflector<M>::visit( impl::abi_to_variant_visitor<M, Resolver>( member_mvo, v, resolver ) );
      mvo(name, std::move(member_mvo));
   }

   template<typename M, typename Resolver, require_abi_t<M>>
   void abi_from_variant::extract( const variant& v, M& o, Resolver resolver ) {
      const variant_object& vo = v.get_object();
      fc::reflector<M>::visit( abi_from_variant_visitor<M, decltype(resolver)>( vo, o, resolver ) );
   }
}

template<typename T, typename Resolver>
void abi_serializer::to_variant( const T& o, variant& vo, Resolver resolver ) try {
   mutable_variant_object mvo;
   impl::abi_to_variant::add(mvo, "_", o, resolver);
   vo = std::move(mvo["_"]);
} FC_RETHROW_EXCEPTIONS(error, "Failed to serialize type", ("object",o))

template<typename T, typename Resolver>
void abi_serializer::from_variant( const variant& v, T& o, Resolver resolver ) try {
   impl::abi_from_variant::extract(v, o, resolver);
} FC_RETHROW_EXCEPTIONS(error, "Failed to deserialize variant", ("variant",v))


} } } // eosio::chain::contracts
