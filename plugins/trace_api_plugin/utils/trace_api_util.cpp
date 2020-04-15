#include <eosio/trace_api/cmd_registration.hpp>

#include <iostream>

#include <boost/program_options.hpp>

using namespace eosio::trace_api;
namespace bpo = boost::program_options;

command_registration* command_registration::_list = nullptr;

namespace {
   auto create_command_map() {
      auto result = std::map<std::string, command_registration*>();
      command_registration* cur = command_registration::_list;
      while (cur != nullptr) {
         if (result.count(cur->name) > 0) {
            std::cerr << "Illformed Program, duplicate subcommand: " << cur->name << "\n";
            exit(1);
         }
         result[cur->name] = cur;
         cur = cur->_next;
      }

      return result;
   }
}

int main(int argc, char** argv) {
   auto command_map = create_command_map();

   bpo::options_description vis_desc("Options");
   auto vis_opts = vis_desc.add_options();
   vis_opts("help,h", "show usage help message");

   bpo::options_description hidden_desc;
   auto hidden_opts = hidden_desc.add_options();
   hidden_opts("subargs", bpo::value<std::vector<std::string>>(), "args");

   bpo::positional_options_description pos_desc;
   pos_desc.add("subargs", -1);

   bpo::options_description cmdline_options;
   cmdline_options.add(vis_desc).add(hidden_desc);

   bpo::variables_map vm;
   auto parsed_args = bpo::command_line_parser(argc, argv).options(cmdline_options).positional(pos_desc).allow_unregistered().run();
   bpo::store(parsed_args, vm);

   std::vector<std::string> args = bpo::collect_unrecognized(parsed_args.options, bpo::include_positional);

   auto show_help = [&](std::ostream& os) {
      os <<
         "Usage: trace_api_util <options> command ...\n"
         "\n"
         "Commands:\n";

      for (const auto& e: command_map) {
         os << "  " << e.second->name << "              " << e.second->slug << "\n";
      }

      os << "\n" << vis_desc << "\n";
   };

   if (args.size() < 1) {
      if (vm.count("help") > 0) {
         show_help(std::cout);
         return 0;
      }

      std::cerr << "Error: No command provided\n\n";
      show_help(std::cerr);
      return 1;
   } else if (command_map.count(args.at(0)) == 0) {
      std::cerr << "Error: unknown command \"" << args.at(0) << "\"\n\n";
      show_help(std::cerr);
      return 1;
   }

   // trim the command name and pass the rest of the args to the defined command
   return command_map.at(args.at(0))->func(vm, std::vector<std::string>(args.begin() + 1, args.end()));
}
