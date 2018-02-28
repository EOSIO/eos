#include <eosio/chain/webassembly/binaryen.hpp>
#include <eosio/chain/apply_context.hpp>

#include <wasm-binary.h>


namespace eosio { namespace chain { namespace webassembly { namespace binaryen {

void entry::call(const string &entry_point, LiteralList& args, apply_context &context)
{
   interpreter_interface local_interface(memory, table, import_lut, sbrk_bytes);
   interface = &local_interface;
   interpreter_instance local_instance(*module.get(), interface);
   instance = &local_instance;
   local_instance.callExport(Name(entry_point), args);
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
   const uint32_t         prev_num_bytes = sbrk_bytes; //_num_bytes;

   // round the absolute value of num_bytes to an alignment boundary
   num_bytes = (num_bytes + 7) & ~7;

   if ((num_bytes > 0) && (prev_num_bytes > (wasm_constraints::maximum_linear_memory - num_bytes)))  // test if allocating too much memory (overflowed)
      return -1;

   // update the number of bytes allocated, and compute the number of pages needed
   
   auto num_accessible_pages = (sbrk_bytes + num_bytes + Memory::kPageSize - 1) >> NBPPL2;
   if(num_accessible_pages > module->memory.max)
      return -1;
   instance->set_accessible_pages(num_accessible_pages);
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

void entry::prepare( const info& base_info) {
}

static Expression* create_call_checktime(Import *checktime, Module &module) {
   // create a calling expression
   CallImport *call = module.allocator.alloc<CallImport>();
   Const *value = module.allocator.alloc<Const>();
   value->finalize();

   value->set(Literal(11));
   call->target = checktime->name;
   call->operands.resize(1);
   call->operands[0] = value;
   call->type = WasmType::none;
   call->finalize();
   return call;
}

static Block* convert_to_injected_block(Expression *expr, Import *checktime, Module &module) {
   Block *block = module.allocator.alloc<Block>();
   block->list.push_back(create_call_checktime(checktime, module));
   block->list.push_back(expr);
   block->finalize();
   return block;
}

static void inject_checktime_block(Block *block, Import *checktime, Module &module)
{
   auto old_size = block->list.size();
   block->list.resize(block->list.size() + 1);
   if (old_size > 0) {
      memmove(&block->list[1], &block->list[0], old_size * sizeof(Expression *));
   }

   block->list[0] = create_call_checktime(checktime, module);

   for (int i = 1; i < block->list.size(); i++) {
      auto* expr = block->list[i];
      if (expr->is<Block>()) {
         inject_checktime_block(expr->cast<Block>(), checktime, module);
      } else if (expr->is<Loop>()) {
         Loop *loop = expr->cast<Loop>();
         if (loop->body->is<Block>()) {
            inject_checktime_block(loop->body->cast<Block>(), checktime, module);
         } else {
            loop->body = convert_to_injected_block(loop->body, checktime, module);
         }
      }
   }
}

static void inject_checktime(Module& module)
{
   // find an appropriate function type
   FunctionType* type = nullptr;
   for (auto &t: module.functionTypes) {
      if (t->result == WasmType::none && t->params.size() == 1 && t->params.at(0) == WasmType::i32) {
         type = t.get();
      }
   }

   if (!type) {
      // create the type
      type = new FunctionType();
      type->name = Name(string("checktime-injected"));
      type->params.push_back(WasmType::i32);
      type->result = WasmType::none;
      module.addFunctionType(type);
   }

   // inject the import
   Import *import = new Import();
   import->name = Name(string("checktime"));
   import->module = Name(string("env"));
   import->base = Name(string("checktime"));
   import->kind = ExternalKind::Function;
   import->functionType = type->name;
   module.addImport(import);

   // walk the ops and inject the callImport on function bodies, blocks and loops
   for(auto &fn: module.functions) {
      if (fn->body->is<Block>()) {
         inject_checktime_block(fn->body->cast<Block>(), import, module);
      } else {
         fn->body = convert_to_injected_block(fn->body, import, module);
      }
   }

}

entry entry::build(const char* wasm_binary, size_t wasm_binary_size) {
   try {
      vector<char> code(wasm_binary, wasm_binary + wasm_binary_size);
      unique_ptr<Module> module(new Module());
      WasmBinaryBuilder builder(*module, code, false);
      builder.read();

      inject_checktime(*module.get());


      FC_ASSERT(module->memory.initial * Memory::kPageSize <= wasm_constraints::maximum_linear_memory);

      // create a temporary globals to  use
      TrivialGlobalManager globals;
      for (auto& global : module->globals) {
         globals[global->name] = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(global->init).value;
      }

      // initialize the linear memory
      linear_memory_type memory;
      memset(memory.data, 0, module->memory.initial * Memory::kPageSize);
      for(size_t i = 0; i < module->memory.segments.size(); i++ ) {
         const auto& segment = module->memory.segments.at(i);
         Address offset = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(segment.offset).value.geti32();
         char *base = memory.data + offset;
         FC_ASSERT(offset + segment.data.size() <= wasm_constraints::maximum_linear_memory);
         memcpy(base, segment.data.data(), segment.data.size());
      }

      call_indirect_table_type table;
      table.resize(module->table.initial);
      for (auto& segment : module->table.segments) {
         Address offset = ConstantExpressionRunner<TrivialGlobalManager>(globals).visit(segment.offset).value.geti32();
         assert(offset + segment.data.size() <= module->table.initial);
         for (size_t i = 0; i != segment.data.size(); ++i) {
            table[offset + i] = segment.data[i];
         }
      }

      // initialize the import lut
      import_lut_type import_lut;
      import_lut.reserve(module->imports.size());
      for (auto& import : module->imports) {
         if (import->module == "env") {
            auto& intrinsic_map = intrinsic_registrator::get_map();
            auto intrinsic_itr = intrinsic_map.find(string(import->base.c_str()));
            if (intrinsic_itr != intrinsic_map.end()) {
               import_lut.emplace(make_pair((uintptr_t)import.get(), intrinsic_itr->second));
               continue;
            }
         }

         FC_ASSERT( !"unresolvable", "${module}.${export}", ("module",import->module.c_str())("export",import->base.c_str()) );
      }

      uint32_t sbrk_bytes = module->memory.initial * Memory::kPageSize;

      return entry(move(module), std::move(memory), std::move(table), std::move(import_lut), sbrk_bytes);
   } catch (const ParseException &e) {
      FC_THROW_EXCEPTION(wasm_execution_error, "Error building interpreter: ${s}", ("s", e.text));
   }
};

entry::entry(unique_ptr<Module>&& module, linear_memory_type&& memory, call_indirect_table_type&& table, import_lut_type&& import_lut, uint32_t sbrk_bytes)
:module(forward<decltype(module)>(module))
,memory(forward<decltype(memory)>(memory))
,table (forward<decltype(table)>(table))
,import_lut(forward<decltype(import_lut)>(import_lut))
,interface(nullptr)
,sbrk_bytes(sbrk_bytes)
{

}

}}}}