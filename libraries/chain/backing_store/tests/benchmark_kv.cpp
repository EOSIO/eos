#include <sys/resource.h>
#include <sys/time.h>

#include <eosio/chain/kv_chainbase_objects.hpp>
#include <eosio/chain/backing_store/kv_context_rocksdb.hpp>
#include <eosio/chain/backing_store/kv_context_chainbase.hpp>

#include <b1/session/rocks_session.hpp>
#include <b1/session/session.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace eosio;
using namespace eosio::chain;

namespace kv_benchmark {

// Global test data
constexpr account_name receiver = "kvrdb"_n;
constexpr uint64_t contract = receiver.to_uint64_t();
constexpr account_name payer = "payer"_n;
constexpr uint64_t default_billable_size = 12;

struct cmd_args {
   std::string operation = "";
   std::string backing_store = "";
   std::string key_file = "";
   std::string workset_file = "";
   uint32_t value_size = 1024;
   uint64_t num_runs = 1000000;
   uint32_t state_size_multiples = 1; // For Chainbase. Multiples of 1GB 
};

struct measurement_t {
   double user_duration_us_avg;
   double system_duration_us_avg;
   uint64_t actual_num_runs;
   uint32_t minor_faults;
   uint32_t major_faults;
   uint32_t blocks_in;
   uint32_t blocks_out;
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

// Loads keys and returns number of keys as an argument.
// This is mainly used to benchmark key creation and erase.
std::vector<std::string> load_keys(const cmd_args& args, uint32_t& num_keys) {
   std::vector<std::string> keys; 
   std::ifstream key_file(args.key_file);

   if (!key_file.is_open()) {
      std::cerr << "Failed to open key file " << args.key_file << std::endl;
      exit(2);
   }

   std::string key;
   num_keys = 0;
   while (getline(key_file, key)) {
      keys.push_back(key);
      ++num_keys;
   }

   key_file.close();

   return keys;
}

// Load workset. Benchmarking "get", "get_data", and "set" 
// requires it.
std::vector<std::string> load_workset(const cmd_args& args, uint32_t& workset_size) {
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

// Read keys from key file and create key-values pairs 
// on backing store.
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

// Returns the number of loops required to perform total_runs
// for runs_per_loop
uint32_t get_num_loops(const uint64_t total_runs, const uint32_t runs_per_loop) {
   if (total_runs <= runs_per_loop) {
      return 1; // one loop is sufficient
   } else {
      return total_runs/runs_per_loop + 1 ;
   }
}

// Returns difference between two timevals
double time_diff_us(const timeval& start, const timeval& end) {
   timeval diff;
   timersub(&end, &start, &diff);
   return (1000000 * diff.tv_sec + diff.tv_usec);
}

// Returns calculated measurement based on raw data
measurement_t calculated_measurement(const uint64_t actual_num_runs, const rusage& usage_start, const rusage& usage_end) {
   measurement_t m;

   m.actual_num_runs = actual_num_runs;
   m.user_duration_us_avg = time_diff_us(usage_start.ru_utime, usage_end.ru_utime)/actual_num_runs;
   m.system_duration_us_avg = time_diff_us(usage_start.ru_stime, usage_end.ru_stime)/actual_num_runs;
   m.minor_faults = uint64_t(usage_end.ru_minflt  - usage_start.ru_minflt);
   m.major_faults = uint64_t(usage_end.ru_majflt  - usage_start.ru_majflt);
   m.blocks_in = uint64_t(usage_end.ru_inblock - usage_start.ru_inblock);
   m.blocks_out = uint64_t(usage_end.ru_oublock - usage_start.ru_oublock);

   return m;
}

// Benchmark "get" operation
measurement_t benchmark_get(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, const std::vector<std::string>& workset) {
   uint32_t num_loops = get_num_loops(args.num_runs, workset.size()); 
   rusage usage_start, usage_end;

   getrusage(RUSAGE_SELF, &usage_start);
   for (auto i = 0; i < num_loops; ++i) {
      for (auto& key: workset) {
         uint32_t actual_value_size;
         kv_context_ptr->kv_get(contract, key.c_str(), key.size(),actual_value_size);
      }
   }
   getrusage(RUSAGE_SELF, &usage_end);

   return calculated_measurement(num_loops*workset.size(), usage_start, usage_end);
}

// Benchmark "get_data" operation
measurement_t benchmark_get_data(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, const std::vector<std::string>& workset) {
   char* data = new char[args.value_size];
   uint32_t num_loops = get_num_loops(args.num_runs, workset.size()); 
   rusage usage_start, usage_end;

   getrusage(RUSAGE_SELF, &usage_start);
   for (auto i = 0; i < num_loops; ++i) {
      for (auto& key: workset) {
         kv_context_ptr->kv_get_data(0, data, args.value_size);
      }
   }
   getrusage(RUSAGE_SELF, &usage_end);

   return calculated_measurement(num_loops*workset.size(), usage_start, usage_end);
}

// Benchmark "set" operation
measurement_t benchmark_set(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, const std::vector<std::string>& workset) {
   std::string value(args.value_size, 'b');
   uint32_t num_loops = get_num_loops(args.num_runs, workset.size()); 
   rusage usage_start, usage_end;

   getrusage(RUSAGE_SELF, &usage_start);
   for (auto i = 0; i < num_loops; ++i) {
      for (auto& key: workset) {
         kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
      }
   }
   getrusage(RUSAGE_SELF, &usage_end);

   return calculated_measurement(num_loops*workset.size(), usage_start, usage_end);
}

// Benchmark "create" operation
measurement_t benchmark_create(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, uint32_t& num_keys) {
   // read keys from the key file
   std::vector<std::string> keys = load_keys(args, num_keys); 
   std::string value(args.value_size, 'a');
   rusage usage_start, usage_end;

   getrusage(RUSAGE_SELF, &usage_start);
   for (auto& key: keys) {
      kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
   }
   getrusage(RUSAGE_SELF, &usage_end);

   return calculated_measurement(keys.size(), usage_start, usage_end);
}

// Benchmark "erase" operation
measurement_t benchmark_erase(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr, uint32_t& num_keys) {
   // create keys in backing store first
   std::vector<std::string> keys = load_keys(args, num_keys); 
   std::string value(args.value_size, 'a');
   for (auto& key: keys) {
      kv_context_ptr->kv_set(contract, key.c_str(), key.size(), value.c_str(), value.size(), payer);
   }

   rusage usage_start, usage_end;
   getrusage(RUSAGE_SELF, &usage_start);
   for (auto& key: keys) {
      kv_context_ptr->kv_erase(contract, key.c_str(), key.size());
   }
   getrusage(RUSAGE_SELF, &usage_end);

   return calculated_measurement(keys.size(), usage_start, usage_end);
}

// Benchmark "it_create" operation
measurement_t benchmark_it_create(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr) {
   rusage usage_start, usage_end;
   getrusage(RUSAGE_SELF, &usage_start);

   auto i = 0;
   std::string prefix = "a";
   while (i < args.num_runs) {
      kv_context_ptr->kv_it_create(contract, prefix.c_str(), 0); // kv_it_create creates a unique pointer. Will be destoryed at the end of the scope.
      ++i;
   }

   getrusage(RUSAGE_SELF, &usage_end);
   return calculated_measurement(args.num_runs, usage_start, usage_end);
}

// Benchmark "it_next" operation
measurement_t benchmark_it_next(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr) {
   rusage usage_start, usage_end;
   getrusage(RUSAGE_SELF, &usage_start);

   uint32_t found_key_size, found_value_size;
   auto it = kv_context_ptr->kv_it_create(contract, "", 0);
   it->kv_it_next(&found_key_size, &found_value_size);

   uint32_t num_runs = 0;
   while (it->kv_it_status() != kv_it_stat::iterator_end) {
      it->kv_it_next(&found_key_size, &found_value_size);
      ++num_runs;
   }

   getrusage(RUSAGE_SELF, &usage_end);
   return calculated_measurement(num_runs, usage_start, usage_end);
}

// Benchmark "it_key_value" operation
measurement_t benchmark_it_key_value(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr) {
   uint32_t offset = 0;
   char* dest = new char[args.value_size];
   uint32_t actual_size;
   uint32_t found_key_size, found_value_size;

   rusage usage_start, usage_end;
   getrusage(RUSAGE_SELF, &usage_start);

   auto it = kv_context_ptr->kv_it_create(contract, "", 0);
   it->kv_it_next(&found_key_size, &found_value_size);  // move to the first position

   uint64_t num_runs = 0;
   while (it->kv_it_status() != kv_it_stat::iterator_end) {
      it->kv_it_key(offset, dest, found_key_size, actual_size);
      it->kv_it_value(offset, dest, found_value_size, actual_size);

      it->kv_it_next(&found_key_size, &found_value_size);
      ++num_runs;
   }

   getrusage(RUSAGE_SELF, &usage_end);
   return calculated_measurement(num_runs, usage_start, usage_end);
}

// Print out benchmarking results
void print_results(const cmd_args& args, const uint32_t num_keys, const uint32_t workset_size, const measurement_t& m) {
   std::cout 
      << "backing_store: " << args.backing_store
      << ", operation: " << args.operation
      << ", key_file: " << args.key_file
      << ", num_keys: " << num_keys
      << ", workset_file: " << args.workset_file
      << ", workset_size: " << workset_size
      << ", value_size: " << args.value_size
      << ", num_runs: " << m.actual_num_runs
      << ", user_cpu_us_avg: " << m.user_duration_us_avg
      << ", system_cpu_us_avg: " << m.system_duration_us_avg
      << ", minor_faults_total: " << m.minor_faults 
      << ", major_faults_total: " << m.major_faults 
      << ", blocks_in_total: " << m.blocks_in  
      << ", blocks_out_total: " << m.blocks_out 
      << std::endl;
}

// Dispatcher to benchmark individual operation
void benchmark_operation(const cmd_args& args, const std::unique_ptr<kv_context>& kv_context_ptr) {
   measurement_t m;
   uint32_t num_keys {0}, workset_size {0};

   // Benchmarking "create" and "erase" creates key values
   // on their own
   if (args.operation != "create" && args.operation != "erase") {
      create_key_values(args, kv_context_ptr, num_keys);
   }

   if (args.operation == "get" || args.operation == "get_data" || args.operation == "set") {
      std::vector<std::string> workset = load_workset(args, workset_size);
      
      if (args.operation == "get" ) {
         m = benchmark_get(args, std::move(kv_context_ptr), workset);
      } else if (args.operation == "get_data" ) {
         m = benchmark_get_data(args, std::move(kv_context_ptr), workset);
      } else if (args.operation == "set" ) {
         m = benchmark_set(args, std::move(kv_context_ptr), workset);
      }
   } else {
      // workset is not used.
      if (args.operation == "it_create" ) {
         m = benchmark_it_create(args, std::move(kv_context_ptr));
      } else if (args.operation == "it_next" ) {
         m = benchmark_it_next(args, std::move(kv_context_ptr));
      } else if (args.operation == "it_key_value" ) {
         m = benchmark_it_key_value(args, std::move(kv_context_ptr));
      } else if (args.operation == "create" ) {
         m = benchmark_create(args, std::move(kv_context_ptr), num_keys);
      } else if (args.operation == "erase" ) {
         m = benchmark_erase(args, std::move(kv_context_ptr), num_keys);
      } else {
         std::cerr << "Unknown operation: " << args.operation << std::endl;
         exit(3);
      }
   };
   
   print_results(args, num_keys, workset_size, m);
}

inline std::shared_ptr<rocksdb::DB> make_rocks_db(const std::string& name) {
   rocksdb::DB* cache_ptr{ nullptr };
   auto         cache = std::shared_ptr<rocksdb::DB>{};

   auto options                                 = rocksdb::Options{};
   options.create_if_missing                    = true;
   options.level_compaction_dynamic_level_bytes = true;
   options.bytes_per_sync                       = 1048576;
   options.OptimizeLevelStyleCompaction(256ull << 20);

   auto status = rocksdb::DB::Open(options, name.c_str(), &cache_ptr);
   cache.reset(cache_ptr);

   return cache;
}

// The driver
void benchmark(const cmd_args& args) {
   if (args.backing_store == "rocksdb") {
      boost::filesystem::remove_all("kvrdb-tmp");  // Use a clean RocksDB
      boost::filesystem::remove_all(chain::config::default_state_dir_name);

      auto rocks_session = eosio::session::make_session(make_rocks_db("kvrdb-tmp"));
      auto session = eosio::session::session<decltype(rocks_session)>{rocks_session};

      std::unique_ptr<kv_context> kv_context_ptr = create_kv_rocksdb_context<decltype(session), mock_resource_manager>(session, receiver, resource_manager, limits); 
      benchmark_operation(args, std::move(kv_context_ptr)); // kv_context_ptr must be in the same scope as kv_db and usage_start, since they are references in create_kv_rocksdb_context
   } else {
      boost::filesystem::remove_all(chain::config::default_state_dir_name);  // Use a clean Chainbase
      chainbase::database chainbase_db(chain::config::default_state_dir_name, database::read_write, args.state_size_multiples * chain::config::default_state_size); // Default is 1024*1024*1024ll == 1073741824
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
     ("key-file,k", bpo::value<string>()->required(), "the file storing all the keys")
     ("workset,w", bpo::value<string>(), "the file storing workset keys, required for get, get_data, set")
     ("operation,o", bpo::value<string>()->required(), "operation to be benchmarked: get, get_data, set, create, erase, it_create, it_next, it_key_value")
     ("backing-store,b", bpo::value<string>()->required(), "the database where kay vlaues are stored, rocksdb or chainbase .")
     ("value-size,v", bpo::value<uint32_t>(), "value size for the keys")
     ("state-size-mul,s", bpo::value<uint32_t>(), "multiples of 1GB for Chainbase state")
     ("num-runs,n", bpo::value<uint64_t>(), "minimum number of runs over the operation to be benchmarked")
     ("help,h","print this list");

   try {
      bpo::store(bpo::parse_command_line(argc, argv, cli), vmap);
      bpo::notify(vmap);

      if (vmap.count("help") > 0) {
         cli.print(std::cerr);
         return 0;
      }

      if (vmap.count("key-file") > 0) {
         args.key_file = vmap["key-file"].as<std::string>();
      }
      if (vmap.count("workset") > 0) {
         args.workset_file = vmap["workset"].as<std::string>();
      }
      if (vmap.count("operation") > 0) {
         args.operation = vmap["operation"].as<std::string>();
         if (args.operation != "get" && args.operation != "get_data" && args.operation != "set" && args.operation != "create" && args.operation != "erase" && args.operation != "it_create" && args.operation != "it_next" && args.operation != "it_key_value") {
            std::cerr << "\'--operation\' must be get, get_data, set, create, erase, it_create, it_next, or it_key_value" << std::endl;
            return 1;
         }
      }
      if (vmap.count("value-size") > 0) {
         args.value_size = vmap["value-size"].as<uint32_t>();
      }
      if (vmap.count("state-size-mul") > 0) {
         args.state_size_multiples = vmap["state-size-mul"].as<uint32_t>();
      }
      if (vmap.count("num-runs") > 0) {
         args.num_runs = vmap["num-runs"].as<uint64_t>();
      }
      if (vmap.count("backing-store") > 0) {
         args.backing_store = vmap["backing-store"].as<std::string>();

         if (args.backing_store != "rocksdb" && args.backing_store != "chainbase") {
            std::cerr << "\'--backing-store\' must be rocksdb or chainbase" << std::endl;
            return 1;
         }
      }
   } catch (bpo::unknown_option &ex) {
      std::cerr << ex.what() << std::endl;
      cli.print (std::cerr);
      return 1;
   }

   if ((args.operation == "get" || args.operation == "get_data" || args.operation == "set") && args.workset_file.empty()) {
      std::cerr << "\'workset\' is required for get, get_data, or set" << std::endl;
      cli.print(std::cerr);
      return 1;
   }

   kv_benchmark::benchmark(args);

   return 0;
}
