/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @brief generates serialization/deserialization functions for contracts types
 **/
#include <string>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <eosio/chain/types.hpp>
#include <eosio/chain/contracts/abi_serializer.hpp>

#include <fc/io/json.hpp>


using namespace std;
using namespace eosio::chain::contracts;

const string tab = "   ";

struct codegen {

   abi_def abi;
   abi_serializer abis;
   codegen(const abi_def& abi)
   :abi(abi) 
   {
      abis.set_abi(abi);
   }

   void generate_ident(ostringstream& output) {
      output << tab << "void print_ident(int n){while(n-->0){print(\"  \");}};" << endl;      
   }
   
   void generate_dump(ostringstream& output, const struct_def& type) {
      output << tab << "void dump(const " << type.name << "& value, int tab=0) {" << endl;
      for(const auto& field : type.fields ) {
         auto field_type = abis.resolve_type(field.type);
         output << tab << tab << "print_ident(tab);";
         output << "print(\"" << field.name << ":[\");";
         if( abis.is_struct(field_type) ) {
            output << "print(\"\\n\"); eosio::dump(value."<< field.name <<", tab+1);";
            output << "print_ident(tab);";
         } else if( field_type == "string" ) {
            output << "prints_l(value."<< field.name <<".get_data(), value."<< field.name <<".get_size());";
         } else if( field_type == "fixed_string32" || field_type == "fixed_string16" ) {
            output << "prints_l(value."<< field.name <<".str, value."<< field.name <<".len);";
         } else if( field_type == "Bytes" ) {
            output << "printhex(value."<< field.name <<".data, value."<< field.name <<".len);";
         } else if( abis.is_integer(field_type) ) {
            auto size = abis.get_integer_size(field_type);
            if(size <= 64) {
               output << "printi(uint64_t(value."<< field.name <<"));";
            } else if (size == 128) {
               output << "printi128(&value."<< field.name <<");";
            } else if (size == 256) {
               output << "printhex((void*)&value."<< field.name <<", sizeof(value."<< field.name <<"));";
            }
            
         } else if( field_type == "bytes" ) {
            output << "printhex(value."<< field.name <<".data, value."<< field.name <<".len);";
         } else if( field_type == "name" || field_type == "account_name" || field_type == "permission_name" ||\
          field_type == "token_name" || field_type == "table_name" || field_type == "func_name" ) {
            output << "printn(value."<< field.name <<");";
         } else if( field_type == "public_key" ) {
            output << "printhex((void*)value."<< field.name <<".data, 33);";
         } else if( field_type == "time" ) {
            output << "printi(value."<< field.name <<");";
         } else if( field_type == "price" ) {
            output << "printi(value."<< field.name <<".base.amount);print(\"/\");printi(value."<< field.name <<".quote.amount);";
         } else {
            
         }
         output << "print(\"]\\n\");" << endl;
      }
      output << tab << "}" << endl;
   }

   void generate_current_message_ex(ostringstream& output) {
      output << tab << "template<typename Type>" << endl;
      output << tab << "Type current_message_ex() {" << endl;
      output << tab << tab << "uint32_t size = message_size();" << endl;
      output << tab << tab << "char* data = (char *)eosio::malloc(size);" << endl;
      output << tab << tab << "assert(data && read_message(data, size) == size, \"error reading message\");" << endl;
      output << tab << tab << "Type value;" << endl;
      output << tab << tab << "eosio::raw::unpack(data, size, value);" << endl;
      output << tab << tab << "eosio::free(data);" << endl;
      output << tab << tab << "return value;" << endl;
      output << tab << "}" << endl;
   }

   void generate_current_message(ostringstream& output, const struct_def& type) {
      output << tab << "template<>" << endl;
      output << tab << type.name << " current_message<" << type.name << ">() {" << endl;
      output << tab << tab << "return current_message_ex<" << type.name << ">();" << endl;
      output << tab << "}" << endl;
   }

   void generate_pack(ostringstream& output, const struct_def& type) {
      output << tab << "template<typename Stream> inline void pack( Stream& s, const " << type.name << "& value ) {" << endl;
      for(const auto& field : type.fields ) {
         output << tab << tab << "raw::pack(s, value." << field.name << ");" << endl;
      }
      if( type.base != type_name() )
         output << tab << tab << "raw::pack(s, static_cast<const " << type.base << "&>(value));" << endl;
      output << tab << "}" << endl;
   }

   void generate_unpack(ostringstream& output, const struct_def& type) {
      output << tab << "template<typename Stream> inline void unpack( Stream& s, " << type.name << "& value ) {" << endl;
      for(const auto& field : type.fields ) {
         output << tab << tab << "raw::unpack(s, value." << field.name << ");" << endl;
      }
      if( type.base != type_name() )
         output << tab << tab << "raw::unpack(s, static_cast<" << type.base << "&>(value));" << endl;
      output << tab << "}" << endl;
   }

   void generate() {
      ostringstream output;
      output << "#pragma once" << endl;
      output << "#include <eoslib/types.hpp>" << endl;
      output << "#include <eoslib/message.hpp>" << endl;
      output << "#include <eoslib/datastream.hpp>" << endl;
      output << "#include <eoslib/raw_fwd.hpp>" << endl;
      
      output << endl;

      //Generate serialization/deserialization for every cotract type
      output << "namespace eosio { namespace raw {" << endl;
      for(const auto& type : abi.structs) {
         generate_pack(output, type);
         generate_unpack(output, type);
      }
      output << "} }" << endl << endl;

      output << "#include <eoslib/raw.hpp>" << endl;

      output << "namespace eosio {" << endl;
      
      generate_ident(output);
      generate_current_message_ex(output);

      for(const auto& type : abi.structs) {
         generate_dump(output, type);
         generate_current_message(output, type);
      }

      output << "} //eosio" << endl;

      cout << output.str() << endl;   
   }

};


//------------------------------------------------------------

int main (int argc, char *argv[]) {

   if( argc < 2 )
      return 1;

   auto abi = fc::json::from_file<abi_def>(argv[1]);
   codegen generator(abi);
   generator.generate();

   return 0;
}
