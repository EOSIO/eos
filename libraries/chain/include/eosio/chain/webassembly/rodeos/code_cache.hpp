#pragma once

#include <eosio/chain/webassembly/rodeos/rodeos.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/key_extractors.hpp>

//phase1 headers:
#include <boost/interprocess/segment_manager.hpp>

namespace eosio { namespace chain { namespace rodeos {

using namespace boost::multi_index;

namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

struct config;

class code_cache {
   public:
      code_cache(const bfs::path data_dir, const rodeos::config& rodeos_config);
      ~code_cache();

      const int& fd() const { return _cache_fd; }

      //If code is in cache: returns pointer & bumps to front of MRU list
      //If code is not in cache, and not blacklisted, and not currently compiling: return nullptr and kick off compile
      //otherwise: return nullptr
      const code_descriptor* const get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version, const std::vector<uint8_t>& wasm, const std::vector<uint8_t>& initial_mem);

   private:
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

      bfs::path _cache_file_path;
      int _cache_fd;
      char* _code_mapping;

      using allocator_t = bip::rbtree_best_fit<bip::null_mutex_family, bip::offset_ptr<void>, 16>;
      allocator_t* allocator;

      void set_on_disk_region_dirty(bool);

      template <typename T>
      void serialize_cache_index(fc::datastream<T>& ds);
};

}}}