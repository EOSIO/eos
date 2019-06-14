#include <eosio/chain/webassembly/eos-vm-oc/code_cache.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/config.hpp>
#include <eosio/chain/webassembly/common.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/memory.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/intrinsic.hpp>
#include <eosio/chain/exceptions.hpp>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/memfd.h>

#include "IR/Module.h"
#include "IR/Validate.h"
#include "WASM/WASM.h"

using namespace IR;
using namespace Runtime;

namespace eosio { namespace chain { namespace eosvmoc {

static constexpr size_t header_offset = 512u;
static constexpr size_t header_size = 512u;
static constexpr size_t total_header_size = header_offset + header_size;
static constexpr uint64_t header_id = 0x31434f4d56534f45ULL; //"EOSVMOC1" little endian

struct code_cache_header {
   uint64_t id = header_id;
   bool dirty = false;
   uintptr_t serialized_descriptor_index = 0;
} __attribute__ ((packed));
static constexpr size_t header_dirty_bit_offset_from_file_start = header_offset + offsetof(code_cache_header, dirty);
static constexpr size_t descriptor_ptr_from_file_start = header_offset + offsetof(code_cache_header, serialized_descriptor_index);

static_assert(sizeof(code_cache_header) <= header_size, "code_cache_header too big");

code_cache::code_cache(const boost::filesystem::path data_dir, const eosvmoc::config& eosvmoc_config) :
   _cache_file_path(data_dir/"code_cache.bin")
{
   static_assert(sizeof(allocator_t) <= header_offset, "header offset intersects with allocator");

   bfs::create_directories(data_dir);

   if(!bfs::exists(_cache_file_path)) {
      std::ofstream ofs(_cache_file_path.generic_string(), std::ofstream::trunc);
      EOS_ASSERT(ofs.good(), database_exception, "unable to create EOS-VM Optimized Compiler code cache");
      bfs::resize_file(_cache_file_path, eosvmoc_config.cache_size);
      bip::file_mapping creation_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region creation_region(creation_mapping, bip::read_write);
      new (creation_region.get_address()) allocator_t(eosvmoc_config.cache_size, total_header_size);
      new ((char*)creation_region.get_address() + header_offset) code_cache_header;
   }

   code_cache_header* cache_header = nullptr;
   {
      char header[total_header_size];
      std::ifstream hs(_cache_file_path.generic_string(), std::ifstream::binary);
      hs.read(header, sizeof(header));
      EOS_ASSERT(!hs.fail(), bad_database_version_exception, "failed to read code cache header");

      cache_header = reinterpret_cast<code_cache_header*>(header+header_offset);
      EOS_ASSERT(cache_header->id == header_id, bad_database_version_exception, "code cache header magic not as expected");
      EOS_ASSERT(!cache_header->dirty, database_exception, "code cache is dirty");
   }

   set_on_disk_region_dirty(true);

   auto existing_file_size = bfs::file_size(_cache_file_path);
   if(eosvmoc_config.cache_size > existing_file_size) {
      bfs::resize_file(_cache_file_path, eosvmoc_config.cache_size);

      bip::file_mapping resize_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
      bip::mapped_region resize_region(resize_mapping, bip::read_write);

      allocator_t* resize_allocator = reinterpret_cast<allocator_t*>(resize_region.get_address());
      resize_allocator->grow(eosvmoc_config.cache_size - existing_file_size);
   }

   _cache_fd = ::open(_cache_file_path.generic_string().c_str(), O_RDWR | O_CLOEXEC);
   EOS_ASSERT(_cache_fd >= 0, database_exception, "failure to open code cache");
   _code_mapping = (char*)mmap(nullptr, eosvmoc_config.cache_size, PROT_READ|PROT_WRITE, MAP_SHARED, _cache_fd, 0);
   EOS_ASSERT(_code_mapping != MAP_FAILED, database_exception, "failure to mmap code cache");

   allocator = reinterpret_cast<allocator_t*>(_code_mapping);

   if(cache_header->serialized_descriptor_index) {
      fc::datastream<const char*> ds(_code_mapping + cache_header->serialized_descriptor_index, eosvmoc_config.cache_size - cache_header->serialized_descriptor_index);
      unsigned number_entries;
      fc::raw::unpack(ds, number_entries);
      for(unsigned i = 0; i < number_entries; ++i) {
         code_descriptor cd;
         fc::raw::unpack(ds, cd);
         _cache_index.push_back(std::move(cd));
      }
      allocator->deallocate(_code_mapping + cache_header->serialized_descriptor_index);
   }

   _free_bytes_eviction_threshold = eosvmoc_config.cache_size * .1;
}

void code_cache::set_on_disk_region_dirty(bool dirty) {
   bip::file_mapping dirty_mapping(_cache_file_path.generic_string().c_str(), bip::read_write);
   bip::mapped_region dirty_region(dirty_mapping, bip::read_write);

   *((char*)dirty_region.get_address()+header_dirty_bit_offset_from_file_start) = dirty;
   if(dirty_region.flush(0, 0, false) == false)
      elog("Syncing code cache failed");
}

template <typename T>
void code_cache::serialize_cache_index(fc::datastream<T>& ds) {
   unsigned entries = _cache_index.size();
   fc::raw::pack(ds, entries);
   for(const code_descriptor& cd : _cache_index)
      fc::raw::pack(ds, cd);
}

code_cache::~code_cache() {
   fc::datastream<size_t> dssz;
   serialize_cache_index(dssz);
   size_t sz = dssz.tellp();

   char* p = (char*)allocator->allocate(sz);
   fc::datastream<char*> ds(p, sz);
   serialize_cache_index(ds);

   uintptr_t ptr_offset = p-_code_mapping;
   *((uintptr_t*)(_code_mapping+descriptor_ptr_from_file_start)) = ptr_offset;

   msync(_code_mapping, allocator->get_size(), MS_SYNC);
   munmap(_code_mapping, allocator->get_size());
   close(_cache_fd);
   set_on_disk_region_dirty(false);
}

void code_cache::free_code(const digest_type& code_id, const uint8_t& vm_version) {
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end()) {
      allocator->deallocate(_code_mapping + it->code_begin);
      _cache_index.get<by_hash>().erase(it);
   }
}

void code_cache::check_eviction_threshold() {
   while(allocator->get_free_memory() < _free_bytes_eviction_threshold && _cache_index.size() > 1) {
      const code_descriptor& cd = _cache_index.back();
      allocator->deallocate(_code_mapping + cd.code_begin);
      _cache_index.pop_back();
   }
}

const code_descriptor* const code_cache::get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version, const std::vector<uint8_t>& wasm, const std::vector<uint8_t>& initial_mem) {
   code_cache_index::index<by_hash>::type::iterator it = _cache_index.get<by_hash>().find(boost::make_tuple(code_id, vm_version));
   if(it != _cache_index.get<by_hash>().end()) {
      _cache_index.relocate(_cache_index.begin(), _cache_index.project<0>(it));
      return &*it;
   }
   
   Module module;
   try {
      Serialization::MemoryInputStream stream(wasm.data(), wasm.size());
      WASM::serialize(stream, module);
   } catch(const Serialization::FatalSerializationException& e) {
      EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
   } catch(const IR::ValidationException& e) {
      EOS_ASSERT(false, wasm_serialization_error, e.message.c_str());
   }

   instantiated_code code;
   try {
      code = LLVMJIT::instantiateModule(module);
   }
   catch(const Runtime::Exception& e) {
      EOS_THROW(wasm_execution_error, "Failed to stand up WAVM instance: ${m}", ("m", describeExceptionCause(e.cause)));
   }
   catch(const std::runtime_error& e) { ///XXX here to catch WAVM_ASSERTS
      EOS_THROW(wasm_execution_error, "Failed to stand up WAVM instance");
   }

   code_descriptor cd;
   cd.code_hash = code_id;
   cd.vm_version = vm_version;
   cd.codegen_version = 0;

   const std::map<unsigned, uintptr_t>& function_to_offsets = code.function_offsets;
   const std::vector<uint8_t>& pic = code.code;

   void* allocated_code = allocator->allocate(pic.size());
   memcpy(allocated_code, pic.data(), pic.size());
   uintptr_t placed_code_in = (char*)allocated_code-_code_mapping;
   cd.code_begin = placed_code_in;
   check_eviction_threshold();

   if(module.startFunctionIndex == UINTPTR_MAX)
      cd.start = no_offset{};
   else if(module.startFunctionIndex < module.functions.imports.size()) {
      const auto& f = module.functions.imports[module.startFunctionIndex];
      const intrinsic_entry& ie = get_intrinsic_map().at(f.moduleName + "." + f.exportName);
      cd.start = intrinsic_ordinal{ie.ordinal};
   }
   else
      cd.start = code_offset{function_to_offsets.at(module.startFunctionIndex-module.functions.imports.size())};

   for(const Export& exprt : module.exports) {
      if(exprt.name == "apply")
         cd.apply_offset = function_to_offsets.at(exprt.index-module.functions.imports.size());
   }

   cd.starting_memory_pages = -1;
   if(module.memories.size())
      cd.starting_memory_pages = module.memories.defs.at(0).type.size.min;

   std::vector<uint8_t> prolouge(memory::cb_offset); ///XXX not the best use of variable
   std::vector<uint8_t>::iterator prolouge_it = prolouge.end();

   //set up mutable globals
   union global_union {
      int64_t i64;
      int32_t i32;
      float f32;
      double f64;
   };

   for(const GlobalDef& global : module.globals.defs) {
      if(!global.type.isMutable)
         continue;
      prolouge_it -= 8;
      global_union* const u = (global_union* const)&*prolouge_it;

      switch(global.initializer.type) {
         case InitializerExpression::Type::i32_const: u->i32 = global.initializer.i32; break;
         case InitializerExpression::Type::i64_const: u->i64 = global.initializer.i64; break;
         case InitializerExpression::Type::f32_const: u->f32 = global.initializer.f32; break;
         case InitializerExpression::Type::f64_const: u->f64 = global.initializer.f64; break;
      }
   }

   struct table_entry {
      uintptr_t type;
      int64_t func;    //>= 0 means offset to code in wasm; < 0 means intrinsic call at offset address
   };

   if(module.tables.size())
      prolouge_it -= sizeof(table_entry) * module.tables.defs[0].type.size.min;

   for(const TableSegment& table_segment : module.tableSegments) {
      struct table_entry* table_index_0 = (struct table_entry*)&*prolouge_it;

      for(Uptr i = 0; i < table_segment.indices.size(); ++i) {
         const Uptr function_index = table_segment.indices[i];
         const long int effective_table_index = table_segment.baseOffset.i32 + i;
         EOS_ASSERT(effective_table_index < module.tables.defs[0].type.size.min, wasm_execution_error, "table init out of bounds");

         if(function_index < module.functions.imports.size()) {
            const auto& f = module.functions.imports[function_index];
            const intrinsic_entry& ie = get_intrinsic_map().at(f.moduleName + "." + f.exportName);
            table_index_0[effective_table_index].func = ie.ordinal*-8;
            table_index_0[effective_table_index].type = (uintptr_t)module.types[module.functions.imports[function_index].type.index];
         }
         else {
            table_index_0[effective_table_index].func = function_to_offsets.at(function_index - module.functions.imports.size());
            table_index_0[effective_table_index].type = (uintptr_t)module.types[module.functions.defs[function_index - module.functions.imports.size()].type.index];
         }
      }
   }

   cd.initdata_pre_memory_size = prolouge.end() - prolouge_it;
   std::move(prolouge_it, prolouge.end(), std::back_inserter(cd.initdata));
   std::move(initial_mem.begin(), initial_mem.end(), std::back_inserter(cd.initdata));

   return &*_cache_index.push_front(std::move(cd)).first;
}

}}}