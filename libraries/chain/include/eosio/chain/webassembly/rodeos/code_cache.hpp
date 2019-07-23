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

class code_cache {
   public:
      ///XXX in future this must take path, size, mapping type etc
      code_cache();
      ~code_cache();

      const int& fd() const { return _cache_fd; }

      //If code is in cache: returns pointer & bumps to front of MRU list
      //If code is not in cache, and not blacklisted, and not currently compiling: return nullptr and kick off compile
      //otherwise: return nullptr
      const code_descriptor* const get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version);

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

      int _cache_fd;

      //XXX a bunch of junk below here just for phase 1. ultimately this stuff gets moved out to the sandbox
      typedef typename bip::segment_manager<char, bip::rbtree_best_fit<bip::null_mutex_family>, bip::iset_index> segment_manager;
      uint8_t* _code_mapping;
      size_t _code_mapping_size;
      segment_manager* _segment_manager;

};

}}}