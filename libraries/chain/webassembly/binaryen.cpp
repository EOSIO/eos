#include <eosio/chain/webassembly/binaryen.hpp>
#include <eosio/chain/apply_context.hpp>

#include <wasm-binary.h>


namespace eosio { namespace chain { namespace webassembly { namespace binaryen {

void entry::call(const string &entry_point, LiteralList& args, apply_context &context)
{
   interpreter_interface local_interface(memory, table, sbrk_bytes);
   interface = &local_interface;
   ModuleInstance instance(*module.get(), interface);
   instance.callExport(Name(entry_point), args);
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
   const uint32_t         prev_num_bytes = sbrk_bytes; //_num_bytes;

   // round the absolute value of num_bytes to an alignment boundary
   num_bytes = (num_bytes + 7) & ~7;

   if ((num_bytes > 0) && (prev_num_bytes > (wasm_constraints::maximum_linear_memory - num_bytes)))  // test if allocating too much memory (overflowed)
      throw eosio::chain::page_memory_error();
   else if ((num_bytes < 0) && (prev_num_bytes < (min_bytes - num_bytes))) // test for underflow
      throw eosio::chain::page_memory_error();

   // update the number of bytes allocated, and compute the number of pages needed
   sbrk_bytes += num_bytes;
   return prev_num_bytes;
}

void entry::reset(const info& base_info) {
   const auto& image = base_info.mem_image;
   char *base = memory.data;
   memcpy(base, image.data(), image.size());
   if (sbrk_bytes > image.size()) {
      memset(memory.data + image.size(), 0, sbrk_bytes - image.size());
   }
   sbrk_bytes = base_info.default_sbrk_bytes;
   interface = nullptr;
}


entry entry::build(const char* wasm_binary, size_t wasm_binary_size) {
   try {
      vector<char> code(wasm_binary, wasm_binary + wasm_binary_size);
      unique_ptr<Module> module(new Module());
      WasmBinaryBuilder builder(*module, code, false);
      builder.read();

      linear_memory_type memory;
      call_indirect_table_type table;

      FC_ASSERT(module->memory.initial * Memory::kPageSize <= wasm_constraints::maximum_linear_memory);

      // create a temporary globals to  use
      TrivialGlobalManager globals;
      for (auto& global : module->globals) {
         globals[global->name] = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(global->init).value;
      }

      // initialize the linear memory
      memset(memory.data, 0, module->memory.initial * Memory::kPageSize);
      for(size_t i = 0; i < module->memory.segments.size(); i++ ) {
         const auto& segment = module->memory.segments.at(i);
         Address offset = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(segment.offset).value.geti32();
         char *base = memory.data + offset;
         FC_ASSERT(offset + segment.data.size() <= wasm_constraints::maximum_linear_memory);
         memcpy(base, segment.data.data(), segment.data.size());
      }

      table.resize(module->table.initial);
      for (auto& segment : module->table.segments) {
         Address offset = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(segment.offset).value.geti32();
         assert(offset + segment.data.size() <= module->table.initial);
         for (size_t i = 0; i != segment.data.size(); ++i) {
            table[offset + i] = segment.data[i];
         }
      }

      uint32_t sbrk_bytes = module->memory.initial * Memory::kPageSize;

      //TODO: validate

      return entry(move(module), std::move(memory), std::move(table), sbrk_bytes);
   } catch (const ParseException &e) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Error building interpreter: ${s}", ("s", e.text));
   }
};

entry::entry(unique_ptr<Module>&& module, linear_memory_type&& memory, call_indirect_table_type&& table, uint32_t sbrk_bytes)
:module(forward<decltype(module)>(module))
,memory(forward<decltype(memory)>(memory))
,table (forward<decltype(table)>(table))
,interface(nullptr)
,sbrk_bytes(sbrk_bytes)
{

}

}}}}