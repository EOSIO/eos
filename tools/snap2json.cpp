/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio/chain/asset.hpp>
#include <eosio/chain/block_log.hpp>
#include <eosio/chain/block_header_state.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/authority.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/optional.hpp>
#include <fc/reflect/reflect.hpp>
#include <fc/filesystem.hpp>

// TODO: find a better way to instantiate chainbase objects
#undef OBJECT_CTOR
#define OBJECT_CTOR(...) public:
#define shared_string vector<char>
#define shared_authority authority
namespace eosio { namespace chain { namespace config {
   template<>
   constexpr uint64_t billable_size_v<authority> = 1;
}}}

#include <eosio/chain/account_object.hpp>
#include <eosio/chain/permission_object.hpp>
#include <eosio/chain/contract_table_objects.hpp>

using namespace eosio;
using namespace eosio::chain;
using namespace chainbase;
using namespace fc;
using namespace std;

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, const char **argv) {

   try {

   po::options_description desc("Convert a snapshot taken with the snapshot_plugin to json");
   desc.add_options()
      ("help,h", "Print this help message and exit")
      ("in", po::value<string>(), "Pathname of the input binary snapshot")
      ("out", po::value<string>(), "Pathname of the output JSON file")
   ;

   po::variables_map vm;
   po::store(po::parse_command_line(argc, argv, desc), vm);
   po::notify(vm);

   string in_file;
   if(vm.count("in")) {
      in_file = vm.at("in").as<string>();
   }

   if( vm.count("help") || !in_file.size() ) {
      std::cout << desc << std::endl;
      return 1;
   }

   if(!fc::exists(in_file)) {
      cout << "Input file does not exists: " << in_file << endl;
      return 1;
   }

   std::ifstream ifs(in_file);

   std::streambuf *buf;
   std::ofstream ofs;

   if(vm.count("out")) {
       ofs.open(vm.at("out").as<string>());
       buf = ofs.rdbuf();
   } else {
       buf = std::cout.rdbuf();
   }

   std::ostream out(buf);

   uint16_t version;
   fc::raw::unpack(ifs, version);
   out << "{\"version\":" << fc::json::to_string(version);

   chain_id_type chain_id(sha256(""));
   fc::raw::unpack(ifs, chain_id);
   out << ",\"chain_id\":" << fc::json::to_string(chain_id);

   genesis_state gs;
   fc::raw::unpack(ifs, gs);
   out << ",\"genesis_state\":" << fc::json::to_string(gs);

   block_header_state bhs;
   fc::raw::unpack(ifs, bhs);
   out << ",\"block_header_state\":" << fc::json::to_string(bhs);

   uint32_t total_accounts;
   fc::raw::unpack(ifs, total_accounts);

   map<account_name, abi_def> contract_abi;

   out << ",\"accounts\":[";
   for(uint32_t i=0; i<total_accounts; ++i) {
      if(i>0) out << ",";
      account_object accnt;
      fc::raw::unpack(ifs, accnt);

      fc::variant v;
      fc::to_variant(accnt, v);
      fc::mutable_variant_object mvo(v);

      abi_def abi;
      if( abi_serializer::to_abi(accnt.abi, abi) ) {
         contract_abi[accnt.name] = abi;
         fc::variant vabi;
         fc::to_variant(abi, vabi);
         mvo["abi"] = vabi;
      }

      out << fc::json::to_string(mvo);
   }
   out << "]";

   out << ",\"permissions\":[";
   uint32_t total_perms;
   fc::raw::unpack(ifs, total_perms);
   for(uint32_t i=0; i<total_perms; ++i) {
      if(i>0) out << ",";
      permission_object perm;
      fc::raw::unpack(ifs, perm);
      out << fc::json::to_string(perm);
   }
   out << "]";

   out << ",\"tables\":[";
   uint32_t total_tables;
   fc::raw::unpack(ifs, total_tables);
   for(uint32_t i=0; i<total_tables; ++i) {
      if(i>0) out << ",";
      table_id_object t_id;
      fc::raw::unpack(ifs, t_id);
      out << "{";
      out << "\"tid\":" << fc::json::to_string(t_id);
      out << ",\"rows\":{";

      const auto& abi = contract_abi[t_id.code];
      abi_serializer abis(abi);
      string table_type = abis.get_table_type(t_id.table);
      vector<char> data;

      for(int j=0; j<t_id.count; ++j) {
         if(j>0) out << ",";
         key_value_object row;
         fc::raw::unpack(ifs, row);
         out << "\"" << row.primary_key << "\":";
         if(!table_type.size()) {
            out << "{\"hex_data\":\"" << fc::to_hex(row.value.data(), row.value.size()) << "\"}";
         } else {
            data.resize( row.value.size() );
            memcpy(data.data(), row.value.data(), row.value.size());
            out << "{\"data\":"
                << fc::json::to_string( abis.binary_to_variant(table_type, data) )
                << "}";
         }
      }
      out << "}}"; //table
   }
   out << "]"; //tables

   out << "}"; //main object

   ifs.close();
   ofs.close();

   return 0;

   } FC_CAPTURE_AND_LOG(());
   return 1;
}
