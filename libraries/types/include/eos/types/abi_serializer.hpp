/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <eos/types/types.hpp>

namespace eosio { namespace types {

using std::map;
using std::string;
using std::function;
using std::pair;

/**
 *  Describes the binary representation message and table contents so that it can
 *  be converted to and from JSON.
 */
struct abi_serializer {
   abi_serializer(){ configure_built_in_types(); }
   abi_serializer( const abi& abi );
   void set_abi(const abi& abi);

   map<type_name, type_name> typedefs;
   map<type_name, struct_t>  structs;
   map<name,type_name>       actions;
   map<name,type_name>       tables;

   typedef std::function<fc::variant(fc::datastream<const char*>&, bool)>  unpack_function;
   typedef std::function<void(const fc::variant&, fc::datastream<char*>&, bool)>  pack_function;
   
   map<type_name, pair<unpack_function, pack_function>> built_in_types;
   void configure_built_in_types();

   bool has_cycle(const vector<pair<string, string>>& sedges)const;
   void validate()const;

   type_name resolve_type(const type_name& t)const;
   int get_integer_size(const type_name& type)const;
   bool is_integer(const type_name& type)const;
   bool is_array(const type_name& type)const;
   bool is_type(const type_name& type)const;
   bool is_struct(const type_name& type)const;
   bool is_builtin_type(const type_name& type)const;

   type_name array_type(const type_name& type)const;

   const struct_t& get_struct(const type_name& type)const;

   type_name get_action_type(name action)const;
   type_name get_table_type(name action)const;

   fc::variant binary_to_variant(const type_name& type, const bytes& binary)const;
   bytes       variant_to_binary(const type_name& type, const fc::variant& var)const;

   fc::variant binary_to_variant(const type_name& type, fc::datastream<const char*>& binary)const;
   void        variant_to_binary(const type_name& type, const fc::variant& var, fc::datastream<char*>& ds)const;

   template<typename Vec>
   static bool is_empty_abi(const Vec& abi_vec)
   {
      return abi_vec.size() <= 4;
   }

   template<typename Vec>
   static bool to_abi(const Vec& abi_vec, abi& abi)
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

} } // eosio::types
