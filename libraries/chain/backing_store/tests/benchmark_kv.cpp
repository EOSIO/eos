#include <sys/resource.h>

#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/kv_context_rocksdb.hpp>
#include <eosio/chain/backing_store/kv_context_chainbase.hpp>

#include <boost/filesystem.hpp>

using namespace eosio;
using namespace eosio::chain;

namespace kv_benchmark {

// Global test data
constexpr account_name receiver = N(kvrdb);
constexpr uint64_t contract = receiver.to_uint64_t();
constexpr account_name payer = N(payer);
std::string prefix = "prefix";
constexpr uint32_t it_key_size = 10;
constexpr uint64_t default_billable_size = 12;

constexpr uint32_t default_value_size = 1024;
std::string default_operation = "get";
struct cmd_args {
   uint32_t num_keys = 0;
   uint32_t value_size = default_value_size;
   bool is_rocksdb = true; // false means chainbase
   std::string operation = default_operation;
   std::string key_file = "";
};

constexpr uint32_t work_set_size = 10000;
constexpr uint32_t num_iterations = 100;

struct dummy_control {
   fc::logger* get_deep_mind_logger() {
      return nullptr;
   }

   uint32_t get_action_id() {
      return 0;
   }
};

struct dummy_context {
   dummy_control control;

   uint32_t get_action_id() {
      return 0;
   }
};

struct mock_resource_manager {
   uint64_t billable_size = default_billable_size;

   int64_t update_table_usage(account_name payer, int64_t delta, const kv_resource_trace& trace) {
      return 0;
   }

   dummy_context* _context;
};

using wall_clock_t = std::chrono::time_point<std::chrono::steady_clock>;

// resource_manager, receiver, limits in kv_context are 
// initialzed as a reference.  Place them globally to avoid 
// going out of scope
constexpr uint32_t max_key_size   = 100;
constexpr uint32_t max_value_size = 1020*1024;
constexpr uint32_t max_iterators  = 3;
kv_database_config limits { max_key_size, max_value_size, max_iterators };
mock_resource_manager resource_manager;

// Main purpose of this program is to microbenchmark individual
// KV API over RocksDB and ChainKV.

void print_usage(char* program_name) {
   std::cout << program_name << " - Microbenchmark KV APIs over RocksDb and Chainbase\n\n"
      << "   -f=FILENAME, file storing keys one per line \n"
      << "   -o=OPERATION, operation to be benchmarked: get, get_data, set, it_next, it_key_value, Default " << default_operation << std::endl
      << "   -c, use Chainbase. Without this option: use RocksDb\n"
      << "   -h, print this help\n"
      << "   -v=NUMBER, value size. Default " << default_value_size << std::endl
      << std::endl;
}

// As this program is only for internal use,
// we don't use sophisticated library to parse command line
void parse_cmd_line(int argc, char* argv[], cmd_args& args) {
   for (auto i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg == "-h") {
         print_usage(argv[0]);
         exit(0);
      } else if (arg == "-f") {
         if (i + 1 < argc) { 
            args.key_file = argv[++i];
         } else {
            std::cerr << "-f option needs key file name" << std::endl;
            print_usage(argv[0]);
            exit(1);
         }  
      } else if (arg == "-o") {
         if (i + 1 < argc) { 
            args.operation = argv[++i];
         } else {
            std::cerr << "-o option needs operation (get, get_data, set)" << std::endl;
            print_usage(argv[0]);
            exit(1);
         }  
      } else if (arg == "-c") {  // use chainbase
         args.is_rocksdb = false;
      } else {
         std::cout << "Unknown option: " << arg << std::endl;
         print_usage(argv[0]);
         exit(1);
      }
   }
}

std::vector<std::string> get_work_set_keys(const cmd_args& args) {
   std::ifstream key_file(args.key_file);
   if (!key_file.is_open()) {
      std::cout << "Failed to open key file " << args.key_file << std::endl;
      exit(2);
   }

   std::string key;
   auto i = 0;
   std::unordered_set<std::string> unordered_keys;
   while (i < work_set_size && getline(key_file, key)) {
      unordered_keys.insert(key);  // randomize
      ++i;
   }
   key_file.close();

   std::vector<std::string> keys;
   for (auto& k: unordered_keys) {
      keys.push_back(k);
   }
   return keys;
}

void create_key_values(cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr) {
   std::ifstream key_file(args.key_file);

   if (!key_file.is_open()) {
      std::cout << "Failed to open key file " << args.key_file << std::endl;
      exit(2);
   }

   std::string value(args.value_size, 'a');
   std::string key;
   while (getline(key_file, key)) {
      kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
      ++args.num_keys;
   }

   key_file.close();
}

void benchmark_get(cmd_args& args, const std::unique_ptr<kv_context> kv_context_ptr, std::vector<std::string> work_set, wall_clock_t& start, wall_clock_t& end, rusage& usage_start, rusage& usage_end) {
   start = std::chrono::steady_clock::now();
   getrusage(RUSAGE_SELF, &usage_start);

   for (auto i = 0; i < num_iterations; ++i) {
      for (auto& key: work_set) {
         uint32_t actual_value_size;
         kv_context_ptr->kv_get(contract, key.c_str(), key.size(),actual_value_size);
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
   end = std::chrono::steady_clock::now();
}

void benchmark_get_data(cmd_args& args, const std::unique_ptr<kv_context> kv_context_ptr, std::vector<std::string> work_set, wall_clock_t& start, wall_clock_t& end, rusage& usage_start, rusage& usage_end) {
   char* data = new char[args.value_size];

   start = std::chrono::steady_clock::now();
   getrusage(RUSAGE_SELF, &usage_start);

   for (auto i = 0; i < num_iterations; ++i) {
      for (auto& key: work_set) {
         kv_context_ptr->kv_get_data(0, data, default_value_size);
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
   end = std::chrono::steady_clock::now();
}

void benchmark_set(cmd_args& args, const std::unique_ptr<kv_context> kv_context_ptr, std::vector<std::string> work_set, wall_clock_t& start, wall_clock_t& end, rusage& usage_start, rusage& usage_end) {
   std::string value(args.value_size, 'b');

   start = std::chrono::steady_clock::now();
   getrusage(RUSAGE_SELF, &usage_start);

   for (auto i = 0; i < num_iterations; ++i) {
      for (auto& key: work_set) {
         kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
   end = std::chrono::steady_clock::now();
}

void benchmark_it_next(cmd_args& args, const std::unique_ptr<kv_context> kv_context_ptr, wall_clock_t& start, wall_clock_t& end, rusage& usage_start, rusage& usage_end) {
   start = std::chrono::steady_clock::now();
   getrusage(RUSAGE_SELF, &usage_start);

   uint32_t found_key_size, found_value_size;
   auto it = kv_context_ptr->kv_it_create(contract, "", 0);
   it->kv_it_next(&found_key_size, &found_value_size);

   while (it->kv_it_status() != kv_it_stat::iterator_end) {
      it->kv_it_next(&found_key_size, &found_value_size);
   }

   getrusage(RUSAGE_SELF, &usage_end);
   end = std::chrono::steady_clock::now();
}

void benchmark_it_key_value(cmd_args& args, const std::unique_ptr<kv_context> kv_context_ptr, wall_clock_t& start, wall_clock_t& end, rusage& usage_start, rusage& usage_end) {
   uint32_t offset = 0;
   char* dest = new char[args.value_size];
   uint32_t actual_size;
   uint32_t found_key_size, found_value_size;

   start = std::chrono::steady_clock::now();
   getrusage(RUSAGE_SELF, &usage_start);

   auto it = kv_context_ptr->kv_it_create(contract, "", 0);
   it->kv_it_next(&found_key_size, &found_value_size);

   while (it->kv_it_status() != kv_it_stat::iterator_end) {
      it->kv_it_key(offset, dest, found_key_size, actual_size);
      it->kv_it_value(offset, dest, found_value_size, actual_size);

      it->kv_it_next(&found_key_size, &found_value_size);
   }

   getrusage(RUSAGE_SELF, &usage_end);
   end = std::chrono::steady_clock::now();
}

void benchmark_operation(cmd_args& args, std::unique_ptr<kv_context> kv_context_ptr, wall_clock_t& start, wall_clock_t& end, rusage& usage_start, rusage& usage_end) {
   create_key_values(args, kv_context_ptr);
   std::vector<std::string> work_set = get_work_set_keys(args);

   if (args.operation == "get" ) {
      benchmark_get(args, std::move(kv_context_ptr), work_set, start, end, usage_start, usage_end);
   } else if (args.operation == "get_data" ) {
      benchmark_get_data(args, std::move(kv_context_ptr), work_set, start, end, usage_start, usage_end);
   } else if (args.operation == "set" ) {
      benchmark_set(args, std::move(kv_context_ptr), work_set, start, end, usage_start, usage_end);
   } else if (args.operation == "it_next" ) {
      benchmark_it_next(args, std::move(kv_context_ptr), start, end, usage_start, usage_end);
   } else if (args.operation == "it_key_value" ) {
      benchmark_it_key_value(args, std::move(kv_context_ptr), start, end, usage_start, usage_end);
   };
}

void print_results(cmd_args& args, wall_clock_t& start, wall_clock_t& end, const rusage& usage_start, const rusage& usage_end) {
   std::string title = "Benchmarked \"" + args.operation + "\" for ";

   if (args.operation == "it_next" || args.operation == "it_key_value") {
      title += " one time over ";
   } else {
      title += std::to_string(num_iterations);
      title += " times on ";
      title += std::to_string(work_set_size);
      title += " keys from ";
   }

   title += std::to_string(args.num_keys);
   title += " total keys ";

   if (args.is_rocksdb) {
      title += "on RocksDb:";
   } else {
      title += "on Chainbase:";
   }

   uint32_t clock_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
   uint32_t user_duration_us = 1000000 * (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) + usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec;;
   uint32_t system_duration_us = 1000000 * (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) + usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec;;
   uint32_t minor_faults = uint32_t(usage_end.ru_minflt  - usage_start.ru_minflt);
   uint32_t major_faults = uint32_t(usage_end.ru_majflt  - usage_start.ru_majflt);
   uint32_t blocks_in = uint32_t(usage_end.ru_inblock - usage_start.ru_inblock);
   uint32_t blocks_out = uint32_t(usage_end.ru_oublock - usage_start.ru_oublock);

   std::cout << title << std::endl 
      << "   clock_duration_us: " << clock_duration_us
      << ", user_duration_us: " << user_duration_us 
      << ", system_duration_us: " << system_duration_us 
      << ", minor_faults: " << minor_faults 
      << ", major_faults: " << major_faults 
      << ", blocks_in: " << blocks_in  
      << ", blocks_out: " << blocks_out 
      << std::endl << std::endl;
}

void benchmark(cmd_args& args) {
   uint32_t clock_duration_us;
   wall_clock_t start, end;
   struct rusage usage_start, usage_end;

   if (args.is_rocksdb) {
      boost::filesystem::remove_all("kvrdb-tmp");  // Use a clean RocksDB
      boost::filesystem::remove_all(chain::config::default_state_dir_name);
      b1::chain_kv::database      kv_db{"kvrdb-tmp", true};
      b1::chain_kv::undo_stack    kv_undo_stack{kv_db, vector<char>{make_rocksdb_undo_prefix()}};

      std::unique_ptr<kv_context> kv_context_ptr = create_kv_rocksdb_context<b1::chain_kv::view, b1::chain_kv::write_session, mock_resource_manager>(kv_db, kv_undo_stack, receiver, resource_manager, limits); 
      benchmark_operation(args, std::move(kv_context_ptr), start, end, usage_start, usage_end); // kv_context_ptr must be in the same scope as kv_db and usage_start, since they are references in create_kv_rocksdb_context
   } else {
      boost::filesystem::remove_all(chain::config::default_state_dir_name);  // Use a clean Chainbase
      chainbase::database chainbase_db(chain::config::default_state_dir_name, database::read_write, chain::config::default_state_size); // 1024*1024*1024ll == 1073741824
      chainbase_db.add_index<kv_index >();

      std::unique_ptr<kv_context> kv_context_ptr = create_kv_chainbase_context<mock_resource_manager>(chainbase_db, receiver, resource_manager, limits);
      benchmark_operation(args, std::move(kv_context_ptr), start, end, usage_start, usage_end);
   }
   
   print_results(args, start, end, usage_start, usage_end);
}
} // namespace kv_benchmark

int main(int argc, char* argv[]) {
   kv_benchmark::cmd_args args;
   kv_benchmark::parse_cmd_line(argc, argv, args);
   kv_benchmark::benchmark(args);

   return 0;
}
