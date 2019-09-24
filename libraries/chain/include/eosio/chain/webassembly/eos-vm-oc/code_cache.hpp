#pragma once

#include <boost/lockfree/spsc_queue.hpp>

#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/key_extractors.hpp>

#include <boost/interprocess/segment_manager.hpp> //XXX no we just want the rbtree thing
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

using allocator_t = bip::rbtree_best_fit<bip::null_mutex_family, bip::offset_ptr<void>, 16>;

struct config;

class code_cache_base {
   public:
      code_cache_base(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db);
      ~code_cache_base();

      const int& fd() const { return _cache_fd; }

      void free_code(const digest_type& code_id, const uint8_t& vm_version);

   protected:
      struct by_hash;

      typedef boost::multi_index_container<
         code_descriptor,
         indexed_by<
            sequenced<>,
            ordered_unique<tag<by_hash>,
               composite_key< code_descriptor,
                  member<code_descriptor, digest_type, &code_descriptor::code_hash>,
                  member<code_descriptor, uint8_t,     &code_descriptor::vm_version>
               >
            >
         >
      > code_cache_index;
      code_cache_index _cache_index;

      const chainbase::database& _db;

      bfs::path _cache_file_path;
      int _cache_fd;

      io_context _ctx;
      local::datagram_protocol::socket _compile_monitor_write_socket{_ctx};
      local::datagram_protocol::socket _compile_monitor_read_socket{_ctx};

      //these are really only useful to the async code cache, but keep them here so
      //free_code can be shared
      std::unordered_set<code_tuple> _queued_compiles;
      std::unordered_map<code_tuple, bool> _outstanding_compiles_and_poison;

      size_t _free_bytes_eviction_threshold;
      void check_eviction_threshold(size_t free_bytes);

      void set_on_disk_region_dirty(bool);

      template <typename T>
      void serialize_cache_index(fc::datastream<T>& ds);
};

class code_cache_async : public code_cache_base {
   public:
      code_cache_async(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db);
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