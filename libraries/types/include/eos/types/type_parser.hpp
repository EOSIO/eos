/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once
#include <map>
#include <string>
#include <vector>
#include <istream>

#include <fc/exception/exception.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/reflect/variant.hpp>

#include <eos/types/native.hpp>
#include <eos/types/exceptions.hpp>

namespace eosio { namespace types {
using std::map;
using std::set;
using std::string;

class abstract_symbol_table {
public:
   virtual ~abstract_symbol_table(){}
   virtual void add_type(const struct_t& s) =0;
   virtual void add_type_def(const type_name& from, const type_name& to) = 0;
   virtual bool is_known_type(const type_name& name) const = 0;
   virtual bool is_valid_type_name(const type_name& name, bool allowArray = false) const;
   virtual bool is_valid_field_name(const field_name& name) const;

   virtual type_name resolve_type_def(type_name name) const = 0;
   virtual struct_t get_type(type_name name) const = 0;

   /**
    * Parses the input stream and populatse the symbol table, the table may be pre-populated
    */
   void parse(std::istream& in);

};


class simple_symbol_table : public abstract_symbol_table {
public:
   simple_symbol_table():
      known({ "field", "struct_t", "asset", "share_type", "name", "field_name", "fixed_string16", "fixed_string32",
            "uint8", "uint16", "uint32", "uint64",
            "uint128", "checksum", "uint256", "UInt512",
            "int8", "int16", "int32", "int64",
            "int128", "Int224", "int256", "Int512",
            "bytes", "public_key", "signature", "string", "time" })
   {}

   virtual void add_type(const struct_t& s) override;
   virtual void add_type_def(const type_name& from, const type_name& to) override;
   virtual bool is_known_type(const type_name& n) const override;
   virtual struct_t get_type(type_name name) const override;
   virtual type_name resolve_type_def(type_name name) const override;

   //      private:
   const set<type_name>      known;
   vector<type_name>         order;
   map<type_name, type_name> typedefs;
   map<type_name, struct_t>  structs;
};



}} // namespace eosio::types

FC_REFLECT(eosio::types::simple_symbol_table, (typedefs)(structs))
