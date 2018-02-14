#include <eosio/chain/webassembly/binaryen.hpp>
#include <eosio/chain/apply_context.hpp>

#include <wasm-binary.h>


namespace eosio { namespace chain { namespace webassembly { namespace binaryen {

void entry::call(const string &entry_point, LiteralList& args, apply_context &context)
{
   instance->callExport(Name(entry_point), args);
}

void entry::call_apply(apply_context& context)
{
   LiteralList args = {Literal(uint64_t(context.act.account)),
                       Literal(uint64_t(context.act.name))};

   call("apply", args, context);
}

void entry::call_error(apply_context& context)
{
   LiteralList args = {/* */};
   call("error", args, context);
}

int entry::sbrk(int num_bytes) {
   // TODO: omitted checktime function from previous version of sbrk, may need to be put back in at some point
   constexpr uint32_t NBPPL2  = 16;
   const uint32_t         num_pages      = module->memory.initial;
   const uint32_t         min_bytes      = (num_pages << NBPPL2) > UINT32_MAX ? UINT32_MAX : num_pages << NBPPL2;
   const uint32_t         prev_num_bytes = interface->sbrk_bytes; //_num_bytes;

   // round the absolute value of num_bytes to an alignment boundary
   num_bytes = (num_bytes + 7) & ~7;

   if ((num_bytes > 0) && (prev_num_bytes > (wasm_constraints::maximum_linear_memory - num_bytes)))  // test if allocating too much memory (overflowed)
      throw eosio::chain::page_memory_error();
   else if ((num_bytes < 0) && (prev_num_bytes < (min_bytes - num_bytes))) // test for underflow
      throw eosio::chain::page_memory_error();

   // update the number of bytes allocated, and compute the number of pages needed
   interface->sbrk_bytes += num_bytes;
   return prev_num_bytes;
}

void entry::reset(const info& base_info) {
   for (size_t i = 0; i < base_info.mem_images.size(); i++) {
      const auto& image = base_info.mem_images.at(i);
      char *base = interface->memory.data + (i * Memory::kPageSize);
      memcpy(base, image.data(), image.size());
   }

   if (interface->sbrk_bytes > base_info.default_sbrk_bytes) {
      memset(interface->memory.data + base_info.default_sbrk_bytes, 0, interface->sbrk_bytes - base_info.default_sbrk_bytes);
   }
   interface->sbrk_bytes = base_info.default_sbrk_bytes;
}


entry entry::build(const char* wasm_binary, size_t wasm_binary_size) {
   vector<char> code(wasm_binary, wasm_binary + wasm_binary_size);
   unique_ptr<Module> module;
   WasmBinaryBuilder builder(*module, code, false);
   builder.read();

   unique_ptr<interpreter_interface> interface;
   unique_ptr<ModuleInstance> instance(new ModuleInstance(*module.get(), interface.get()));

   //TODO: validate

   return entry(move(module), move(interface), move(instance));
};

entry::entry(unique_ptr<Module>&& module, unique_ptr<interpreter_interface>&& interface, unique_ptr<ModuleInstance>&& instance)
:module(forward<decltype(module)>(module))
,interface(forward<decltype(interface)>(interface))
,instance (forward<decltype(instance)>(instance))
{

}

}}}}