#include <sys/resource.h>

#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/kv_context_rocksdb.hpp>
#include <eosio/chain/backing_store/kv_context_chainbase.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace eosio;
using namespace eosio::chain;

namespace kv_benchmark {

// Global test data
constexpr account_name receiver = N(kvrdb);
constexpr uint64_t contract = receiver.to_uint64_t();
constexpr account_name payer = N(payer);
constexpr uint32_t it_key_size = 10;
constexpr uint64_t default_billable_size = 12;

constexpr uint32_t default_value_size = 1024;
struct cmd_args {
   uint32_t value_size = default_value_size;
   bool is_rocksdb = false; // false means chainbase
   std::string operation = "get";
   std::string key_file = "";
   std::string workset_file = "";
   std::string creation_mode = "random";
   uint32_t num_repeats = 10000;
};

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

// resource_manager, receiver, limits in kv_context are 
// initialzed as a reference.  Place them globally to avoid 
// going out of scope
constexpr uint32_t max_key_size   = 100;
constexpr uint32_t max_value_size = 1020*1024;
constexpr uint32_t max_iterators  = 100;
kv_database_config limits { max_key_size, max_value_size, max_iterators };
mock_resource_manager resource_manager;

// Main purpose of this program is to microbenchmark individual
// KV API over RocksDB and ChainKV.

std::vector<std::string> read_workset(const cmd_args& args, uint32_t& workset_size) {
   std::ifstream workset_ifs(args.workset_file);

   if (!workset_ifs.is_open()) {
      std::cerr << "Failed to open workset file " << args.workset_file << std::endl;
      exit(2);
   }

   std::string key;
   std::vector<std::string> workset;
   workset_size = 0;
   while (getline(workset_ifs, key)) {
      ++workset_size;
      workset.push_back(key);
   }

   workset_ifs.close();
   return workset;
}

void create_key_values(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, uint32_t& num_keys) {
   std::ifstream key_file(args.key_file);

   if (!key_file.is_open()) {
      std::cerr << "Failed to open key file " << args.key_file << std::endl;
      exit(2);
   }

   std::string value(args.value_size, 'a');
   std::string key;
   num_keys = 0;
   while (getline(key_file, key)) {
      kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
      ++num_keys;
   }

   key_file.close();
}

void benchmark_get(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, const std::vector<std::string>& workset, rusage& usage_start, rusage& usage_end) {
   getrusage(RUSAGE_SELF, &usage_start);

   for (auto i = 0; i < args.num_repeats; ++i) {
      for (auto& key: workset) {
         uint32_t actual_value_size;
         kv_context_ptr->kv_get(contract, key.c_str(), key.size(),actual_value_size);
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
}

void benchmark_get_data(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, const std::vector<std::string>& workset, rusage& usage_start, rusage& usage_end) {
   char* data = new char[args.value_size];

   getrusage(RUSAGE_SELF, &usage_start);

   for (auto i = 0; i < args.num_repeats; ++i) {
      for (auto& key: workset) {
         kv_context_ptr->kv_get_data(0, data, default_value_size);
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
}

void benchmark_set(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, const std::vector<std::string>& workset, rusage& usage_start, rusage& usage_end) {
   std::string value(args.value_size, 'b');

   getrusage(RUSAGE_SELF, &usage_start);

   for (auto i = 0; i < args.num_repeats; ++i) {
      for (auto& key: workset) {
         kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
}

void benchmark_it_create(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, rusage& usage_start, rusage& usage_end) {
   getrusage(RUSAGE_SELF, &usage_start);

   auto i = 0;
   std::string prefix = "a";
   while (i < args.num_repeats) {
      kv_context_ptr->kv_it_create(contract, prefix.c_str(), 0);
      ++i;
      if (prefix[0] != 'z') {
         prefix[0] = static_cast<char>(prefix[0] + 1);
      } else {
         prefix[0] = 'a';
      }
   }

   getrusage(RUSAGE_SELF, &usage_end);
}

void benchmark_it_next(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, rusage& usage_start, rusage& usage_end) {
   getrusage(RUSAGE_SELF, &usage_start);

   uint32_t found_key_size, found_value_size;
   auto it = kv_context_ptr->kv_it_create(contract, "", 0);
   it->kv_it_next(&found_key_size, &found_value_size);

   while (it->kv_it_status() != kv_it_stat::iterator_end) {
      it->kv_it_next(&found_key_size, &found_value_size);
   }

   getrusage(RUSAGE_SELF, &usage_end);
}

void benchmark_it_key_value(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, rusage& usage_start, rusage& usage_end) {
   uint32_t offset = 0;
   char* dest = new char[args.value_size];
   uint32_t actual_size;
   uint32_t found_key_size, found_value_size;

   getrusage(RUSAGE_SELF, &usage_start);

   auto it = kv_context_ptr->kv_it_create(contract, "", 0);
   it->kv_it_next(&found_key_size, &found_value_size);

   while (it->kv_it_status() != kv_it_stat::iterator_end) {
      it->kv_it_key(offset, dest, found_key_size, actual_size);
      it->kv_it_value(offset, dest, found_value_size, actual_size);

      it->kv_it_next(&found_key_size, &found_value_size);
   }

   getrusage(RUSAGE_SELF, &usage_end);
}

void print_results(const cmd_args& args, const uint32_t num_keys, const uint32_t workset_size, const rusage& usage_start, const rusage& usage_end) {
   uint32_t user_duration_us = 1000000 * (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) + usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec;;
   uint32_t system_duration_us = 1000000 * (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) + usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec;;
   uint32_t minor_faults = uint32_t(usage_end.ru_minflt  - usage_start.ru_minflt);
   uint32_t major_faults = uint32_t(usage_end.ru_majflt  - usage_start.ru_majflt);
   uint32_t blocks_in = uint32_t(usage_end.ru_inblock - usage_start.ru_inblock);
   uint32_t blocks_out = uint32_t(usage_end.ru_oublock - usage_start.ru_oublock);

   std::cout 
      << "operation: " << args.operation
      << ", backing_store: " << (args.is_rocksdb ? "rocksdb" : "chainbase")
      << ", creation_mode: " << args.creation_mode
      << ", num_keys: " << num_keys
      << ", value_size: " << args.value_size;

   if (args.operation == "get" || args.operation == "get_data" || args.operation == "set" || args.operation == "it_create") {
      std::cout
      << ", num_repeats: " << args.num_repeats;
   }

   if (args.operation == "it_create" || args.operation == "it_next" || args.operation == "it_key_value") {
      std::cout
      << ", user_duration_us_total: " << user_duration_us
      << ", system_duration_us_total: " << system_duration_us;
   } else {
      std::cout
      << ", workset_size: " << workset_size
      << ", user_duration_us_avg: " << (double)user_duration_us/(args.num_repeats*workset_size)
      << ", system_duration_us_avg: " << (double)system_duration_us/(args.num_repeats*workset_size);
   };

   std::cout
      << ", minor_faults_total: " << minor_faults 
      << ", major_faults_total: " << major_faults 
      << ", blocks_in_total: " << blocks_in  
      << ", blocks_out_total: " << blocks_out 
      << std::endl << std::endl;
}

void benchmark_operation(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr) {
   uint32_t clock_duration_us;
   struct rusage usage_start, usage_end;
   uint32_t num_keys, workset_size;

   create_key_values(args, kv_context_ptr, num_keys);
   std::vector<std::string> workset = read_workset(args, workset_size);

   if (args.operation == "get" ) {
      benchmark_get(args, std::move(kv_context_ptr), workset, usage_start, usage_end);
   } else if (args.operation == "get_data" ) {
      benchmark_get_data(args, std::move(kv_context_ptr), workset, usage_start, usage_end);
   } else if (args.operation == "set" ) {
      benchmark_set(args, std::move(kv_context_ptr), workset, usage_start, usage_end);
   } else if (args.operation == "it_create" ) {
      benchmark_it_create(args, std::move(kv_context_ptr), usage_start, usage_end);
   } else if (args.operation == "it_next" ) {
      benchmark_it_next(args, std::move(kv_context_ptr), usage_start, usage_end);
   } else if (args.operation == "it_key_value" ) {
      benchmark_it_key_value(args, std::move(kv_context_ptr), usage_start, usage_end);
   };
   
   print_results(args, num_keys, workset_size, usage_start, usage_end);
}


void benchmark(const cmd_args& args) {
   if (args.is_rocksdb) {
      boost::filesystem::remove_all("kvrdb-tmp");  // Use a clean RocksDB
      boost::filesystem::remove_all(chain::config::default_state_dir_name);
      b1::chain_kv::database      kv_db{"kvrdb-tmp", true};
      b1::chain_kv::undo_stack    kv_undo_stack{kv_db, vector<char>{make_rocksdb_undo_prefix()}};

      std::unique_ptr<kv_context> kv_context_ptr = create_kv_rocksdb_context<b1::chain_kv::view, b1::chain_kv::write_session, mock_resource_manager>(kv_db, kv_undo_stack, receiver, resource_manager, limits); 
      benchmark_operation(args, std::move(kv_context_ptr)); // kv_context_ptr must be in the same scope as kv_db and usage_start, since they are references in create_kv_rocksdb_context
   } else {
      boost::filesystem::remove_all(chain::config::default_state_dir_name);  // Use a clean Chainbase
      chainbase::database chainbase_db(chain::config::default_state_dir_name, database::read_write, chain::config::default_state_size); // 1024*1024*1024ll == 1073741824
      chainbase_db.add_index<kv_index >();

      std::unique_ptr<kv_context> kv_context_ptr = create_kv_chainbase_context<mock_resource_manager>(chainbase_db, receiver, resource_manager, limits);
      benchmark_operation(args, std::move(kv_context_ptr));
   }
}
} // namespace kv_benchmark

namespace bpo = boost::program_options;
using bpo::options_description;
using bpo::variables_map;

int main(int argc, char* argv[]) {
   kv_benchmark::cmd_args args;

   variables_map vmap;
   options_description cli ("kv_benchmark command line options");
   
   std::string gts;
   cli.add_options()
     ("keyfile,k", bpo::value<string>()->required(), "the file storing all the keys")
     ("workset,w", bpo::value<string>(), "the file storing workset keys")
     ("operation,o", bpo::value<string>(), "operation to be benchmarked: get, get_data, set, it_create, it_next, it_key_value")
     ("valuesize,v", bpo::value<uint32_t>(), "value size for the keys")
     ("numrepeats,n", bpo::value<uint32_t>(), "number of repeats of test run over workset and number of iterator creations in it_create")
     ("rocksdb,r", "rocksdb to be backing store.")
     ("sorted,s", "keys in keyfile are sorted.")
     ("help,h","print this list");

   try {
      bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
      bpo::notify(vmap);

      if (vmap.count("help") > 0) {
         cli.print(std::cerr);
         return 0;
      }

      if (vmap.count("keyfile") > 0) {
         args.key_file = vmap["keyfile"].as<std::string>();
      }
      if (vmap.count("workset") > 0) {
         args.workset_file = vmap["workset"].as<std::string>();
      }
      if (vmap.count("operation") > 0) {
         args.operation = vmap["operation"].as<std::string>();
      }
      if (vmap.count("valuesize") > 0) {
         args.value_size = vmap["valuesize"].as<uint32_t>();
      }
      if (vmap.count("numrepeats") > 0) {
         args.num_repeats = vmap["numrepeats"].as<uint32_t>();
      }
      if (vmap.count("rocksdb") > 0) {
         args.is_rocksdb = true;
      }
      if (vmap.count("sorted") > 0) {
         args.creation_mode = "sorted";
      }
   } catch (bpo::unknown_option &ex) {
      std::cerr << ex.what() << std::endl;
      cli.print (std::cerr);
      return 1;
   }

   if ((args.operation == "get" || args.operation == "get_data" || args.operation == "set") && args.workset_file.empty()) {
      std::cerr << "workset argument is required for get, get_data, or set" << std::endl;
      cli.print(std::cerr);
      return 1;
   }

   kv_benchmark::benchmark(args);

   return 0;
}
