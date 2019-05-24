#include <eosio/chain/chain_snapshot.hpp>
#include <eosio/chain/snapshot.hpp>
#include <eosio/chain/account_object.hpp>
#include <boost/program_options.hpp>

#include <iostream>

namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::positional_options_description;
using bpo::variables_map;

using namespace eosio::chain;

struct fake_account_object {
   account_name         name;
   uint8_t              vm_type;
   uint8_t              vm_version;
   bool                 privileged;

   time_point           last_code_update;
   digest_type          code_version;
   block_timestamp_type creation_date;

   vector<char>         code;
   vector<char>         abi;
};

FC_REFLECT(fake_account_object, (name)(vm_type)(vm_version)(privileged)(last_code_update)(code_version)(creation_date)(code)(abi))


int main(int argc, char** argv) { try {
   options_description named_options("Command Line Options for extract-accounts");
   named_options.add_options()
         ("help,h", "Print this help message and exit.")
         ("input-file,i", bpo::value<std::string>()->default_value( "-" ), "The binary snapshot file to read from (\"-\" for stdin)")
         ("output-file,o", bpo::value<std::string>()->default_value( "-" ), "The text file to write to (\"-\" for stdout)");

   positional_options_description positional_options;
   positional_options.add("input-file", 1);
   positional_options.add("output-file", 1);

   variables_map options;

   bpo::store(
      bpo::command_line_parser(argc, argv)
         .options(named_options)
         .positional(positional_options)
         .run(),
      options);

   bpo::notify(options);

   std::string input_path = options["input-file"].as<std::string>();
   if (input_path == "-") {
      input_path = "/dev/stdin";
   }

   std::string output_path = options["output-file"].as<std::string>();
   if (output_path == "-") {
      output_path = "/dev/stdout";
   }

   auto infile = std::ifstream(input_path, (std::ios::in | std::ios::binary));
   auto outfile = std::ofstream(output_path, (std::ios::out));
   auto reader = std::make_shared<istream_snapshot_reader>(infile);
   reader->validate();
   reader->read_section<account_object>([&]( auto &section ){
      bool more = !section.empty();
      while(more) {
         fake_account_object obj;
         more = section.read_row(obj);

         outfile << obj.name.to_string() << std::endl;
      }
   });
   infile.close();
   outfile.close();

   return 0;

} FC_LOG_AND_RETHROW() }