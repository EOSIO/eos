/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 *  @brief generates serialization/deserialization functions for contracts types
 **/
#include <string>
#include <sstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <eos/types/types.hpp>
#include <eos/types/AbiSerializer.hpp>

#include <fc/io/json.hpp>


using namespace std;
using namespace eosio;

const string tab = "   ";

struct CodeGen {

   types::Abi abi;
   types::AbiSerializer abis;
   CodeGen(const types::Abi& abi) : abi(abi) {
      abis.setAbi(abi);
   }

   void generate_ident(ostringstream& output) {
      output << tab << "inline void print_ident(int n){while(n-->0){print(\"  \");}};" << endl;      
   }
   
   void generate_from_bytes(ostringstream& output) {
      output << tab << "template<typename Type> void from_bytes(const Bytes& bytes, Type& value) {" << endl;
      output << tab << tab << "datastream<char *> ds((char*)bytes.data, bytes.len);" << endl;
      output << tab << tab << "raw::unpack(ds, value);" << endl;
      output << tab << "}" << endl;
   }

   void generate_to_bytes(ostringstream& output) {
      output << tab << "template<typename Type> Bytes to_bytes(const Type& value) {" << endl;
      output << tab << tab << "uint32_t maxsize = packed_size(value);" << endl;
      output << tab << tab << "char* buffer = (char *)eos::malloc(maxsize);" << endl;
      output << tab << tab << "datastream<char *> ds(buffer, maxsize);" << endl;
      output << tab << tab << "pack(ds, value);" << endl;
      output << tab << tab << "Bytes bytes;" << endl;
      output << tab << tab << "bytes.len = ds.tellp();" << endl;
      output << tab << tab << "bytes.data = (uint8_t*)buffer;" << endl;
      output << tab << tab << "return bytes;" << endl;
      output << tab << "}" << endl;
   }

   void generate_dump(ostringstream& output, const types::Struct& type) {
      output << tab << "void dump(const " << type.name << "& value, int tab=0) {" << endl;
      for(const auto& field : type.fields ) {
         auto field_type = abis.resolveType(field.type);
         output << tab << tab << "print_ident(tab);";
         output << "print(\"" << field.name << ":[\");";
         if( abis.isStruct(field_type) ) {
            output << "print(\"\\n\"); eos::dump(value."<< field.name <<", tab+1);";
            output << "print_ident(tab);";
         } else if( field_type == "String" ) {
            output << "prints_l(value."<< field.name <<".get_data(), value."<< field.name <<".get_size());";
         } else if( field_type == "FixedString32" || field_type == "FixedString16" ) {
            output << "prints_l(value."<< field.name <<".str, value."<< field.name <<".len);";
         } else if( field_type == "Bytes" ) {
            output << "printhex(value."<< field.name <<".data, value."<< field.name <<".len);";
         } else if( abis.isInteger(field_type) ) {
            auto size = abis.getIntegerSize(field_type);
            if(size <= 64) {
               output << "printi(uint64_t(value."<< field.name <<"));";
            } else if (size == 128) {
               output << "printi128(&value."<< field.name <<");";
            } else if (size == 256) {
               output << "printhex(&value."<< field.name <<", sizeof(value."<< field.name <<"));";
            }
            
         } else if( field_type == "Bytes" ) {
            output << "printhex(value."<< field.name <<".data, value."<< field.name <<".len));";
         } else if( field_type == "Name" || field_type == "AccountName" || field_type == "PermissionName" ||\
          field_type == "TokenName" || field_type == "TableName" || field_type == "FuncName" ) {
            output << "printn(value."<< field.name <<");";
         } else if( field_type == "PublicKey" ) {
            output << "printhex((void*)value."<< field.name <<".data, 33);";
         } else if( field_type == "Time" ) {
            output << "printi(value."<< field.name <<");";
         } else {
            
         }
         output << "print(\"]\\n\");" << endl;
      }
      output << tab << "}" << endl;
   }

   void generate_current_message_ex(ostringstream& output) {
      output << tab << "template<typename Type>" << endl;
      output << tab << "Type current_message_ex() {" << endl;
      output << tab << tab << "uint32_t msgsize = messageSize();" << endl;
      output << tab << tab << "Bytes bytes;" << endl;
      output << tab << tab << "bytes.data = (uint8_t *)eos::malloc(msgsize);" << endl;
      output << tab << tab << "bytes.len  = msgsize;" << endl;
      output << tab << tab << "assert(bytes.data && readMessage(bytes.data, bytes.len) == msgsize, \"error reading message\");" << endl;
      output << tab << tab << "Type value;" << endl;
      output << tab << tab << "eos::raw::from_bytes(bytes, value);" << endl;
      output << tab << tab << "eos::free(bytes.data);" << endl;
      output << tab << tab << "return value;" << endl;
      output << tab << "}" << endl;
   }

   void generate_pack(ostringstream& output, const types::Struct& type) {
      output << tab << "template<typename Stream> inline void pack( Stream& s, const " << type.name << "& value ) {" << endl;
      for(const auto& field : type.fields ) {
         output << tab << tab << "raw::pack(s, value." << field.name << ");" << endl;
      }
      if( type.base != types::TypeName() )
         output << tab << tab << "raw::pack(s, static_cast<const " << type.base << "&>(value));" << endl;
      output << tab << "}" << endl;
   }

   void generate_unpack(ostringstream& output, const types::Struct& type) {
      output << tab << "template<typename Stream> inline void unpack( Stream& s, " << type.name << "& value ) {" << endl;
      for(const auto& field : type.fields ) {
         output << tab << tab << "raw::unpack(s, value." << field.name << ");" << endl;
      }
      if( type.base != types::TypeName() )
         output << tab << tab << "raw::unpack(s, static_cast<" << type.base << "&>(value));" << endl;
      output << tab << "}" << endl;
   }

   void generate_packed_size(ostringstream& output, const types::Struct& type) {
      output << tab << "inline uint32_t packed_size(const " << type.name << "& c) {" << endl;
      output << tab << tab << "uint32_t size = 0;" << endl;
      for(const auto& field : type.fields ) {
         output << tab << tab << "size += packed_size(c."<<field.name<<");" << endl;
      }
      if( type.base != types::TypeName() )
         output << tab << tab << "size += packed_size(static_cast<" << type.base << "&>(c));" << endl;
      output << tab << tab << "return size;" << endl;
      output << tab << "}" << endl;
   }

   void generate() {
      ostringstream output;
      output << "#include <eoslib/types.hpp>" << endl;
      output << "#include <eoslib/datastream.hpp>" << endl;
      output << "#include <eoslib/raw.hpp>" << endl;
      output << endl;

      //Generate serialization/deserialization for every cotract type
      
      output << "namespace eos { namespace raw {" << endl;
      for(const auto& type : abi.structs) {
         generate_packed_size(output, type);
         generate_pack(output, type);
         generate_unpack(output, type);
      }
      
      generate_from_bytes(output);
      generate_to_bytes(output);
      output << "} }" << endl << endl;

      output << "namespace eos {" << endl;
      
      generate_ident(output);

      for(const auto& type : abi.structs) {
         generate_dump(output, type);
      }

      generate_current_message_ex(output);

      output << "} //eos" << endl;

      cout << output.str() << endl;   
   }

};


//------------------------------------------------------------

int main (int argc, char *argv[]) {

   if( argc < 2 )
      return 1;

   auto abi = fc::json::from_file<types::Abi>(argv[1]);
   CodeGen codegen(abi);
   codegen.generate();

   return 0;
}
