/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/types.hpp>

namespace eosio { namespace chain {

using type_name      = string;
using field_name     = string;
using hash_type      = uint64_t;

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

struct event_def {
    event_def() = default;
    event_def(const event_name& name, const type_name& type)
        :name(name), type(type)
    {}

    event_name name;
    type_name   type;
};

struct order_def {
   order_def() = default;
   order_def(const field_name& field, const type_name& order)
   : field(field), order(order)
   { }

   field_name         field;
   type_name          order; // asc/desc
   vector<field_name> path;
   type_name          type;
};

struct index_def {
   index_def() = default;
   index_def(const index_name& name, const bool unique, const vector<order_def>& orders)
   : name(name), unique(unique), orders(orders)
   { }

   index_def(const index_name& name, const hash_type hash, const bool unique, const vector<order_def>& orders)
   : name(name), hash(hash), unique(unique), orders(orders)
   { }

   index_name         name;
   hash_type          hash = 0;
   bool               unique = false;
   vector<order_def>  orders;
};

struct table_def {
   table_def() = default;
   table_def(const table_name& name, const type_name& type, const vector<index_def>& indexes)
   :name(name), type(type), indexes(indexes)
   {}

   table_def(const table_name& name, const hash_type hash, const type_name& type, const vector<index_def>& indexes)
   :name(name), hash(hash), type(type), indexes(indexes)
   {}

   table_name         name;        // the name of the table
   hash_type          hash = 0;
   int64_t            row_count = 0;
   type_name          type;        // type of binary data stored in this table
   vector<index_def>  indexes;     //
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

template<typename T>
struct may_not_exist {
   T value{};
};

struct abi_def {
   abi_def() = default;
   abi_def(const vector<type_def>& types, const vector<struct_def>& structs, const vector<action_def>& actions, const vector<event_def>& events, const vector<table_def>& tables, const vector<clause_pair>& clauses, const vector<error_message>& error_msgs)
   :types(types)
   ,structs(structs)
   ,actions(actions)
   ,events(events)
   ,tables(tables)
   ,ricardian_clauses(clauses)
   ,error_messages(error_msgs)
   {}

   string                              version = "cyberway::abi/1.1";
   vector<type_def>                    types;
   vector<struct_def>                  structs;
   vector<action_def>                  actions;
   vector<event_def>                   events;
   vector<table_def>                   tables;
   vector<clause_pair>                 ricardian_clauses;
   vector<error_message>               error_messages;
   extensions_type                     abi_extensions;
   may_not_exist<vector<variant_def>>  variants;
};

abi_def eosio_contract_abi(const abi_def& eosio_system_abi);
vector<type_def> common_type_defs();

} } /// namespace eosio::chain

namespace fc {

template<typename ST, typename T>
datastream<ST>& operator << (datastream<ST>& s, const eosio::chain::may_not_exist<T>& v) {
   raw::pack(s, v.value);
   return s;
}

template<typename ST, typename T>
datastream<ST>& operator >> (datastream<ST>& s, eosio::chain::may_not_exist<T>& v) {
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

} // namespace fc

FC_REFLECT( eosio::chain::type_def                         , (new_type_name)(type) )
FC_REFLECT( eosio::chain::field_def                        , (name)(type) )
FC_REFLECT( eosio::chain::struct_def                       , (name)(base)(fields) )
FC_REFLECT( eosio::chain::action_def                       , (name)(type)(ricardian_contract) )
FC_REFLECT( eosio::chain::event_def                        , (name)(type) )
FC_REFLECT( eosio::chain::order_def                        , (field)(order) )
FC_REFLECT( eosio::chain::index_def                        , (name)(unique)(orders) )
FC_REFLECT( eosio::chain::table_def                        , (name)(type)(indexes) )
FC_REFLECT( eosio::chain::clause_pair                      , (id)(body) )
FC_REFLECT( eosio::chain::error_message                    , (error_code)(error_msg) )
FC_REFLECT( eosio::chain::variant_def                      , (name)(types) )
FC_REFLECT( eosio::chain::abi_def                          , (version)(types)(structs)(actions)(events)(tables)
                                                             (ricardian_clauses)(error_messages)(abi_extensions)(variants) )
