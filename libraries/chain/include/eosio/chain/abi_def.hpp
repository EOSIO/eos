#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

using type_name      = string;
using field_name     = string;
using index_name     = name;

struct type_def {
   type_def() = default;
   type_def(const type_name& new_type_name, const type_name& type)
   :new_type_name(new_type_name), type(type)
   {}

   type_name   new_type_name;
   type_name   type;
};

struct field_def {
   field_def() = default;
   field_def(const field_name& name, const type_name& type)
   :name(name), type(type)
   {}

   field_name name;
   type_name  type;

   bool operator==(const field_def& other) const {
      return std::tie(name, type) == std::tie(other.name, other.type);
   }
};

struct struct_def {
   struct_def() = default;
   struct_def(const type_name& name, const type_name& base, const vector<field_def>& fields)
   :name(name), base(base), fields(fields)
   {}

   type_name            name;
   type_name            base;
   vector<field_def>    fields;

   bool operator==(const struct_def& other) const {
      return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
   }
};

struct action_def {
   action_def() = default;
   action_def(const action_name& name, const type_name& type, const string& ricardian_contract)
   :name(name), type(type), ricardian_contract(ricardian_contract)
   {}

   action_name name;
   type_name   type;
   string      ricardian_contract;
};

struct table_def {
   table_def() = default;
   table_def(const table_name& name, const type_name& index_type, const vector<field_name>& key_names, const vector<type_name>& key_types, const type_name& type)
   :name(name), index_type(index_type), key_names(key_names), key_types(key_types), type(type)
   {}

   table_name         name;        // the name of the table
   type_name          index_type;  // the kind of index, i64, i128i128, etc
   vector<field_name> key_names;   // names for the keys defined by key_types
   vector<type_name>  key_types;   // the type of key parameters
   type_name          type;        // type of binary data stored in this table
};

struct primary_index_def {
   primary_index_def() = default;
   primary_index_def(const index_name &name, const type_name &type)
      : name(name), type(type)
   {}

   index_name name;                // the name of primary index
   type_name  type;                // the kind of index

   bool operator==(const primary_index_def &other) const {
      return std::tie(name, type) == std::tie(other.name, other.type);
   }

   bool operator!=(const primary_index_def &other) const {
      return !operator==(other);
   }
};

struct secondary_index_def
{
   secondary_index_def() = default;
   secondary_index_def(const type_name &type)
      : type(type)
   {}

   type_name  type;              // the kind of index

   bool operator==(const secondary_index_def &other) const {
      return type == other.type;
   }

   bool operator!=(const secondary_index_def &other) const {
      return !operator==(other);
   }
};

struct kv_table_def {
   kv_table_def() = default;
   kv_table_def(const type_name& type, const primary_index_def& primary_index, const map<index_name, secondary_index_def>& secondary_indices)
   :type(type), primary_index(primary_index), secondary_indices(secondary_indices)
   {}

   type_name                             type;                // the name of the struct_def
   primary_index_def                     primary_index;       // primary index field
   map<index_name, secondary_index_def>  secondary_indices;   // secondary indices fields

   string get_index_type(const string& index_name)const {
      if( index_name == primary_index.name.to_string() ) return primary_index.type;
      for( const auto& kv : secondary_indices ) {
         if( index_name == kv.first.to_string() ) return kv.second.type;
      }
      return {};
   }
 };

struct clause_pair {
   clause_pair() = default;
   clause_pair( const string& id, const string& body )
   : id(id), body(body)
   {}

   string id;
   string body;
};

struct error_message {
   error_message() = default;
   error_message( uint64_t error_code, const string& error_msg )
   : error_code(error_code), error_msg(error_msg)
   {}

   uint64_t error_code;
   string   error_msg;
};

struct variant_def {
   type_name            name;
   vector<type_name>    types;
};

struct action_result_def {
   action_result_def() = default;
   action_result_def(const action_name& name, const type_name& result_type)
   :name(name), result_type(result_type)
   {}

   action_name name;
   type_name   result_type;
};

template<typename T>
struct may_not_exist {
   T value{};
};

// Needed for custom from_variant and to_variant of kv_table_def
template<typename T>
struct kv_tables_as_object {
    T value{};
};

struct abi_def {
   abi_def() = default;
   abi_def(const vector<type_def>& types, const vector<struct_def>& structs, const vector<action_def>& actions, const vector<table_def>& tables, const vector<clause_pair>& clauses, const vector<error_message>& error_msgs, const kv_tables_as_object<map<table_name, kv_table_def>>& kv_tables)
   :types(types)
   ,structs(structs)
   ,actions(actions)
   ,tables(tables)
   ,ricardian_clauses(clauses)
   ,error_messages(error_msgs)
   ,kv_tables(kv_tables)

   {}

   string                                    version = "";
   vector<type_def>                          types;
   vector<struct_def>                        structs;
   vector<action_def>                        actions;
   vector<table_def>                         tables;
   vector<clause_pair>                       ricardian_clauses;
   vector<error_message>                     error_messages;
   extensions_type                           abi_extensions;
   may_not_exist<vector<variant_def>>        variants;
   may_not_exist<vector<action_result_def>>  action_results;
   kv_tables_as_object<map<table_name, kv_table_def>> kv_tables;
};

abi_def eosio_contract_abi(const abi_def& eosio_system_abi);
vector<type_def> common_type_defs();

extern unsigned char eosio_abi_bin[2132];

} } /// namespace eosio::chain

namespace fc {

template<typename ST, typename T>
ST& operator << (ST& s, const eosio::chain::may_not_exist<T>& v) {
   raw::pack(s, v.value);
   return s;
}

template<typename ST, typename T>
ST& operator >> (ST& s, eosio::chain::may_not_exist<T>& v) {
   if (s.remaining())
      raw::unpack(s, v.value);
   return s;
}

template<typename T>
void to_variant(const eosio::chain::may_not_exist<T>& e, fc::variant& v) {
   to_variant( e.value, v);
}

template<typename T>
void from_variant(const fc::variant& v, eosio::chain::may_not_exist<T>& e) {
   from_variant( v, e.value );
}


template<typename ST, typename T>
ST& operator << (ST& s, const eosio::chain::kv_tables_as_object<T>& v) {
   raw::pack(s, v.value);
   return s;
}

template<typename ST, typename T>
ST& operator >> (ST& s, eosio::chain::kv_tables_as_object<T>& v) {
   if( s.remaining() )
      raw::unpack(s, v.value);
   return s;
}

template<typename T>
void to_variant(const eosio::chain::kv_tables_as_object<T>& o, fc::variant& v) {
   const auto &kv_tables = o.value;
   mutable_variant_object mvo;

   for( const auto &table : kv_tables ) {
      mutable_variant_object vo_table;

      variant primary_index;
      to_variant(table.second.primary_index, primary_index);

      mutable_variant_object secondary_indices;
      for( const auto &sec_index : table.second.secondary_indices ) {
         variant sidx;
         to_variant(sec_index.second, sidx);
         secondary_indices(sec_index.first.to_string(), sidx);
      }

      vo_table("type", variant(table.second.type))("primary_index", primary_index)("secondary_indices", variant(secondary_indices));
      mvo(table.first.to_string(), variant(vo_table));
   }
   v = variant(mvo);
}

template<typename T>
void from_variant(const fc::variant& v, eosio::chain::kv_tables_as_object<T>& o) {
    EOS_ASSERT( v.is_object(), eosio::chain::invalid_type_inside_abi, "variant is not an variant_object type");

    auto &kv_tables = o.value;
    const auto& tables = v.get_object();

    for( const auto& table_it : tables ) {
        const auto &table_obj = table_it.value().get_object();
        eosio::chain::kv_table_def kv_tbl_def;
        from_variant(table_obj["type"], kv_tbl_def.type);
        from_variant(table_obj["primary_index"], kv_tbl_def.primary_index);
        if( const auto st_it = table_obj.find("secondary_indices"); st_it != table_obj.end() ) {
            const auto &sec_indices_obj = st_it->value().get_object();
            for( const auto& sidx_it : sec_indices_obj ) {
                eosio::chain::secondary_index_def idx_def;
                from_variant(sidx_it.value(), idx_def);
                kv_tbl_def.secondary_indices[eosio::chain::index_name(sidx_it.key())] = idx_def;
            }
        }
        kv_tables[eosio::chain::name(table_it.key())] = kv_tbl_def;
    }
}
} // namespace fc

FC_REFLECT( eosio::chain::type_def                         , (new_type_name)(type) )
FC_REFLECT( eosio::chain::field_def                        , (name)(type) )
FC_REFLECT( eosio::chain::struct_def                       , (name)(base)(fields) )
FC_REFLECT( eosio::chain::action_def                       , (name)(type)(ricardian_contract) )
FC_REFLECT( eosio::chain::table_def                        , (name)(index_type)(key_names)(key_types)(type) )
FC_REFLECT( eosio::chain::primary_index_def                , (name)(type) )
FC_REFLECT( eosio::chain::secondary_index_def              , (type) )
FC_REFLECT( eosio::chain::kv_table_def                     , (type)(primary_index)(secondary_indices) )
FC_REFLECT( eosio::chain::clause_pair                      , (id)(body) )
FC_REFLECT( eosio::chain::error_message                    , (error_code)(error_msg) )
FC_REFLECT( eosio::chain::variant_def                      , (name)(types) )
FC_REFLECT( eosio::chain::action_result_def                , (name)(result_type) )
FC_REFLECT( eosio::chain::abi_def                          , (version)(types)(structs)(actions)(tables)
                                                             (ricardian_clauses)(error_messages)(abi_extensions)(variants)(action_results)(kv_tables) )
