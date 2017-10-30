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

#include <fc/io/json.hpp>


using namespace std;
using namespace eos;

const string tab = "   ";

void generateValueToBytes(ostringstream& output, const types::Table& table) {
   output << "Bytes valueToBytes(const " << table.type << "& kv) {" << endl;
   output << tab << "uint32_t maxsize = estimatePackedSize(kv.value);" << endl;
   output << tab << "char* buffer = (char *)eos::malloc(maxsize);" << endl;
   output << tab << "datastream<char *> ds(buffer, maxsize);" << endl;
   output << tab << "raw::pack(ds, kv.value);" << endl;
   output << tab << "Bytes bytes;" << endl;
   output << tab << "bytes.len = ds.tellp();" << endl;
   output << tab << "bytes.data = (uint8_t*)buffer;" << endl;
   output << tab << "return bytes;" << endl;
   output << "}" << endl;
}

void generateBytesToValue(ostringstream& output, const types::Table& table) {

}

void generateCurrentMessage(ostringstream& output, const types::Action& action) {
   output << tab << "template<>" << endl;
   output << tab << action.type << " currentMessage<" << action.type << ">() {" << endl;
   output << tab << tab << "uint32_t msgsize = messageSize();" << endl;
   output << tab << tab << "char* buffer = (char *)eos::malloc(msgsize);" << endl;
   output << tab << tab << "assert(readMessage(buffer, msgsize) == msgsize, \"error reading " << action.type << "\");" << endl;
   output << tab << tab << "datastream<char *> ds(buffer, msgsize);" << endl;
   output << tab << tab << action.type << " value;" << endl;
   output << tab << tab << "raw::unpack(ds, value);" << endl;
   output << tab << tab << "eos::free(buffer);" << endl;
   output << tab << tab << "return value;" << endl;
   output << tab << "}" << endl;
}

void generatePack(ostringstream& output, const types::Struct& type) {
   output << tab << "template<typename Stream> inline void pack( Stream& s, const " << type.name << "& value ) {" << endl;
   for(const auto& field : type.fields ) {
      output << tab << tab << "raw::pack(s, value." << field.name << ");" << endl;
   }
   if( type.base != types::TypeName() )
      output << tab << tab << "raw::pack(s, static_cast<const " << type.base << "&>(value));" << endl;
   output << tab << "}" << endl;
}

void generateUnpack(ostringstream& output, const types::Struct& type) {
   output << tab << "template<typename Stream> inline void unpack( Stream& s, " << type.name << "& value ) {" << endl;
   for(const auto& field : type.fields ) {
      output << tab << tab << "raw::unpack(s, value." << field.name << ");" << endl;
   }
   if( type.base != types::TypeName() )
      output << tab << tab << "raw::unpack(s, static_cast<" << type.base << "&>(value));" << endl;
   output << tab << "}" << endl;
}

void generateEstimatePackedSize(ostringstream& output, const types::Struct& type) {
   output << "uint32_t estimatePackedSize(const " << type.name << "& c) {" << endl;
   output << tab << "uint32_t size = 0;" << endl;
   for(const auto& field : type.fields ) {
      output << tab;
      if( field.type == "String" )
         output << "size += c."<<field.name<<".get_size() + 4;" << endl;
      else if( field.type == "Bytes" ) {
         output << "size += c."<<field.name<<".len + 4;" << endl;
      } else {
         output << "size += sizeof(c."<<field.name<<");" << endl;
      }
   }
   if( type.base != types::TypeName() )
      output << tab << "size += estimatePackedSize(static_cast<" << type.base << "&>(c));" << endl;
   output << tab << "return size;" << endl;
   output << "}" << endl;
}

//------------------------------------------------------------

int main (int argc, char *argv[]) {

   if( argc < 2 )
      return 1;

   auto abi = fc::json::from_file<types::Abi>(argv[1]);

   ostringstream output;
   output << "#include <eoslib/datastream.hpp>" << endl;
   output << "#include <eoslib/raw.hpp>" << endl;
   output << endl;

   for(const auto& type : abi.structs) {
      generateEstimatePackedSize(output, type);
   }

   //Generate valueToBytes/bytesToValue functions for every str index table type
   for(const auto& table : abi.tables) {
      if(table.indextype != "str") continue;
      generateValueToBytes(output, table);
   }

   //Generate serialization/deserialization for every cotract type
   output << "namespace eos { namespace raw {" << endl;
   for(const auto& type : abi.structs) {
      generatePack(output, type);
      generateUnpack(output, type);
   }
   output << "} }" << endl << endl;

   //Generate currentMessage specialization for every action
   output << "namespace eos {" << endl;
   vector<types::TypeName> types_seen;
   for(const auto& action : abi.actions) {
      if(find(types_seen.begin(), types_seen.end(), action.type) != types_seen.end()) continue;
      generateCurrentMessage(output, action);
      types_seen.emplace_back(action.type);
   }
   output << "} //eos" << endl;

   cout << output.str() << endl;

   return 0;
}