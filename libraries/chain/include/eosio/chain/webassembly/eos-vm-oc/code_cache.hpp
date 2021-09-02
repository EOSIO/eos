#pragma once

#include <boost/lockfree/spsc_queue.hpp>

#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/key_extractors.hpp>

#include <boost/interprocess/mem_algo/rbtree_best_fit.hpp>
#include <boost/asio/local/datagram_protocol.hpp>


#include <thread>

namespace std {
    template<> struct hash<eosio::chain::eosvmoc::code_tuple> {
        size_t operator()(const eosio::chain::eosvmoc::code_tuple& ct) const noexcept {
            return ct.code_id._hash[0];
        }
    };
}

namespace eosio { namespace chain { namespace eosvmoc {

using namespace boost::multi_index;
using namespace boost::asio;

namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

using allocator_t = bip::rbtree_best_fit<bip::null_mutex_family, bip::offset_ptr<void>, alignof(std::max_align_t)>;

struct config;

class code_cache_base {
   public:
      using code_finder = std::function<std::string_view(const eosio::chain::digest_type&, uint8_t)>;
      code_cache_base(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, code_finder db);
      ~code_cache_base();

      //fd of code cache suitable for sending to out of process code manager
      const int& cache_fd() const { return _cache_fd; }
      //return a mmap()ing of the code cache (and the mapping size) suitable for execution
      void cache_mapping_for_execution(const int prot_flags, uint8_t*& addr, size_t& map_size) const;

      size_t number_entries() const {return _cache_index.size();}
      //only known after completion of a compile
      std::optional<size_t> free_bytes() const {return _last_known_free_bytes;}

      void free_code(const digest_type& code_id, const uint8_t& vm_version);

   protected:
      struct by_hash;

      typedef boost::multi_index_container<
         code_descriptor,
         indexed_by<
            sequenced<>,
            hashed_unique<tag<by_hash>,
               composite_key< code_descriptor,
                  member<code_descriptor, digest_type, &code_descriptor::code_hash>,
                  member<code_descriptor, uint8_t,     &code_descriptor::vm_version>
               >
            >
         >
      > code_cache_index;
      code_cache_index _cache_index;

      code_finder _db;

      //handle to the on-disk file
      int _cache_file_fd;

      //the file to map in, and properties of that mapping
      int _cache_fd = -1;
      size_t _mapped_size = 0;
      bool _populate_on_map = false;
      bool _mlock_map = false;

      int _extra_mmap_flags = 0;

      io_context _ctx;
      local::datagram_protocol::socket _compile_monitor_write_socket{_ctx};
      local::datagram_protocol::socket _compile_monitor_read_socket{_ctx};

      //these are really only useful to the async code cache, but keep them here so
      //free_code can be shared
      std::unordered_set<code_tuple> _queued_compiles;
      std::unordered_map<code_tuple, bool> _outstanding_compiles_and_poison;

      size_t _free_bytes_eviction_threshold;
      void check_eviction_threshold(size_t free_bytes);
      void run_eviction_round();
      std::optional<size_t> _last_known_free_bytes;

      void set_on_disk_region_dirty(bool);

      int get_huge_memfd(size_t map_size, int memfd_flags) const;

      template <typename T>
      void serialize_cache_index(fc::datastream<T>& ds);
};

class code_cache_async : public code_cache_base {
   public:
      code_cache_async(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, code_finder db);
      ~code_cache_async();

      //If code is in cache: returns pointer & bumps to front of MRU list
      //If code is not in cache, and not blacklisted, and not currently compiling: return nullptr and kick off compile
      //otherwise: return nullptr
      const code_descriptor* const get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version);

   private:
      std::thread _monitor_reply_thread;
      boost::lockfree::spsc_queue<wasm_compilation_result_message> _result_queue;
      void wait_on_compile_monitor_message();
      std::tuple<size_t, size_t> consume_compile_thread_queue();
      std::unordered_set<code_tuple> _blacklist;
      size_t _threads;
};

class code_cache_sync : public code_cache_base {
   public:
      using code_cache_base::code_cache_base;
      ~code_cache_sync();

      //Can still fail and return nullptr if, for example, there is an expected instantiation failure
      const code_descriptor* const get_descriptor_for_code_sync(const digest_type& code_id, const uint8_t& vm_version);
};

}}}
