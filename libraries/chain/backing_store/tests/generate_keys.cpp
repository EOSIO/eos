#include<iostream>
#include<fstream>

namespace gen_keys {

struct cmd_args {
   uint32_t num_keys = 0;
   std::string first_key;
   std::string key_file;
};

// Generates "num_keys" keys starting with "first_key" alphabetically
// and saves them into "key_file".
// One may use this program to generate multiple files of keys,
// concatenate them, and use Linux shuf command to randomize the
// resulting file.

void print_usage(char* program_name) {
   std::cout << program_name << " - Generates \"num_keys\" keys starting with \"first_key\" alphabetically and saves them into \"key_file\"\n\n"
      << "   -f=FILENAME, file storing the resultingkeys\n"
      << "   -n=NUMBER, number of keys to be generated\n"
      << "   -k=KEY, the first key to be generated\n"
      << "   -h, print this help\n"
      << std::endl;
}

// As this program is only for internal use,
// we don't use sophisticated library to parse command line
void parse_cmd_line(int argc, char* argv[], cmd_args& args) {
   if (argc != 7) {
      std::cout << "Require 3 arguments" << std::endl;
      print_usage(argv[0]);
      exit(1);
   }

   for (auto i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-h") {
         print_usage(argv[0]);
         exit(0);
      } else if (arg == "-f") {
         if (i + 1 < argc) { 
            args.num_keys = 0;
            args.key_file = argv[++i];
         } else {
            std::cerr << "-f option needs key file name" << std::endl;
            print_usage(argv[0]);
            exit(1);
         }  
      } else if (arg == "-n") {
         if (i + 1 < argc) { 
            args.num_keys = std::stoi(argv[++i]);
         } else {
            std::cerr << "-n option needs number of keys" << std::endl;
            print_usage(argv[0]);
            exit(1);
         }  
      } else if (arg == "-k") {
         if (i + 1 < argc) { 
            args.first_key = argv[++i];
         } else {
            std::cerr << "-k option needs first key" << std::endl;
            print_usage(argv[0]);
            exit(1);
         }  
      } else {
         std::cout << "Unknown option: " << arg << std::endl;
         print_usage(argv[0]);
         exit(1);
      }
   }
}

// Generate next key alphabetically.
// Lower case only without loss of generality.
std::string generate_next_key(const std::string& current_key) {
   std::string next_key = current_key;
   uint32_t last_possible_index = current_key.size() - 1;
   while (last_possible_index >= 0) {
      if (next_key[last_possible_index] != 'z') {
         next_key[last_possible_index] = static_cast<char>(next_key[last_possible_index] + 1);
         return next_key;
      } else {
        next_key[last_possible_index] = 'a';
        --last_possible_index;
      }
   }

   std::cout << "Number of keys requested too big. No next key available" << std::endl;
   return "";
}

void generate_keys(const cmd_args& args) {
   std::ofstream key_file(args.key_file, std::ios::out | std::ios::trunc);
   if (!key_file.is_open()) {
      std::cout << "Failed to open key file " << args.key_file << std::endl;
      exit(2);
   }
   
   std::string curr_key = args.first_key;
   for (auto i = 0; i < args.num_keys; ++i) {
      key_file << curr_key << std::endl;
      curr_key = generate_next_key(curr_key);
   }

   key_file.close();
}

} // namespace

int main(int argc, char* argv[]) {
   gen_keys::cmd_args args;
   gen_keys::parse_cmd_line(argc, argv, args);
   gen_keys::generate_keys(args);

   return 0;
}
